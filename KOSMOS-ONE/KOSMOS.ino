/* KOSMOS + PRA32-U (All-in-One:  Waveshare Pico-Audio version) */
#include <Arduino.h>

// Optional: give Core1 8KB stack if needed
bool core1_separate_stack = true;

float g_sub_gain = 1.0f;   // 0.0〜1.0

// ----------------------- PRA32-U on Core1 (I2S + Waveshare Pico-Audio) ----------------------
#include <I2S.h>

I2S g_i2s_output(OUTPUT);

#define PRA32_U_VERSION "vOne"
#define PRA32_U_MIDI_CH (0)

// I2S / DAC for Pimoroni Pico Audio Pack (PIM544)
#define PRA32_U_I2S_SWAP_BCLK_AND_LRCLK_PINS  (false)
#define PRA32_U_I2S_SWAP_LEFT_AND_RIGHT       (false)

#define PRA32_U_I2S_DATA_PIN  (9)
#define PRA32_U_I2S_BCLK_PIN  (10)   // LRCLK = 11

#define PRA32_U_I2S_BUFFERS                   (4)
#define PRA32_U_I2S_BUFFER_WORDS              (64)

// 内部 MIDI ブリッジ（すでにあなたのコードにあるやつ）
enum MidiEvType : uint8_t { EV_NOTE_ON=0, EV_NOTE_OFF=1, EV_CC=2 };
struct MidiEvent { uint8_t type, d1, d2, ch; };
namespace MidiQ {
  constexpr size_t QSIZE=256; static volatile uint32_t head=0, tail=0; static MidiEvent q[QSIZE];
  inline bool push(const MidiEvent& ev){uint32_t h=head,n=(h+1)%QSIZE; if(n==tail) return false; q[h]=ev; head=n; return true;}
  inline bool pop(MidiEvent& out){uint32_t t=tail; if(t==head) return false; out=q[t]; tail=(t+1)%QSIZE; return true;}
}

inline void synth_note_on_core1(uint8_t note,uint8_t vel,uint8_t ch=0){
    MidiEvent ev{EV_NOTE_ON,note,vel,ch};

    // ★ 最大 100 回だけリトライ（数十マイクロ秒程度）
    for (int i = 0; i < 100; i++) {
        if (MidiQ::push(ev)) return;
        tight_loop_contents();
    }
    // ここまで来たら諦めて捨てる（アルペジオの時間軸を優先）
}

inline void synth_note_off_core1(uint8_t note,uint8_t ch=0){
    MidiEvent ev{EV_NOTE_OFF,note,0,ch};
    for (int i = 0; i < 100; i++) {
        if (MidiQ::push(ev)) return;
        tight_loop_contents();
    }
}

inline void midi_bridge_send_cc(uint8_t cc,uint8_t val,uint8_t ch=0){
    MidiEvent ev{EV_CC,cc,val,ch};
    for (int i = 0; i < 50; i++) {   // CC は優先度低めでリトライ回数も少なく
        if (MidiQ::push(ev)) return;
        tight_loop_contents();
    }
}

// PRA32-U synth 用のグローバル
uint8_t g_midi_ch = PRA32_U_MIDI_CH;
#include "pra32-u-common.h"
#include "pra32-u-synth.h"

PRA32_U_Synth g_synth;
PRA32_U_Synth<true> g_sub_synth;

// 標準 I2S インスタンス
I2S i2s(OUTPUT);

#include "pico/multicore.h"

extern I2S g_i2s_output;

// ------------------------------------------------------
// Core1: メイン処理（I2S + シンセ + MIDI受信）
// ------------------------------------------------------
void __not_in_flash_func(core1_main)() {

    // ==== PRA32-U と同じ PIO I2S 初期化 ====
    g_i2s_output.setSysClk(48000);
    g_i2s_output.setFrequency(48000);

    g_i2s_output.setDATA(9);
    g_i2s_output.setBCLK(10);

    // ★ これが抜けていた（PRA32-U と完全一致させる）
    if (PRA32_U_I2S_SWAP_BCLK_AND_LRCLK_PINS) {
        g_i2s_output.swapClocks();
    }

    g_i2s_output.setBitsPerSample(16);
    g_i2s_output.setBuffers(PRA32_U_I2S_BUFFERS, PRA32_U_I2S_BUFFER_WORDS);
    g_i2s_output.begin();

    // ==== シンセ初期化 ====
    g_synth.initialize();
    g_sub_synth.initialize();
    g_synth.program_change(11);
    g_sub_synth.program_change(8);

    const int N = PRA32_U_I2S_BUFFER_WORDS;
    int16_t Lbuf[N], Rbuf[N];

    while (true) {

        // ==== MIDI 受信 ====
        MidiEvent ev;
        while (MidiQ::pop(ev)) {
            uint8_t ch = ev.ch;
            if (ev.type == EV_NOTE_ON) {
                if (ch == 0) g_synth.note_on(ev.d1, ev.d2);
                else if (ch == 1) g_sub_synth.note_on(ev.d1, ev.d2);
            }
            else if (ev.type == EV_NOTE_OFF) {
                if (ch == 0) g_synth.note_off(ev.d1);
                else if (ch == 1) g_sub_synth.note_off(ev.d1);
            }
            else if (ev.type == EV_CC) {
                if (ev.d1 == 100) {
                    uint8_t prog = ev.d2;
                    if (ch == 0) g_synth.program_change(prog);
                    else if (ch == 1) g_sub_synth.program_change(prog);
                }
                // ★ Bパート Volume（CC200）
                else if (ev.d1 == 200 && ev.ch == 1) {
                    g_sub_gain = ev.d2 / 127.0f;
                }
                // ★ それ以外の CC は全部シンセにパススルー
                else {
                    if (ch == 0) g_synth.control_change(ev.d1, ev.d2);
                    else if (ch == 1) g_sub_synth.control_change(ev.d1, ev.d2);
                }
            }
        }

        // ==== I2S バッファ生成 ====
        for (int i = 0; i < N; i++) {

            int16_t subR;
            int16_t subL = g_sub_synth.process(0, subR);

            // ★ ここでゲインを掛ける
            subL = subL * g_sub_gain;
            subR = subR * g_sub_gain;

            int16_t mainR;
            int16_t mainL = g_synth.process(subL, mainR);

            Lbuf[i] = mainL * 0.7f;
            Rbuf[i] = mainR * 0.7f;
        }

        // ==== PRA32-U と同じ PIO I2S 出力 ====
        for (int i = 0; i < N; i++) {
            g_i2s_output.write16(Lbuf[i], Rbuf[i]);
        }
    }
}

// ----------------------- Core0: KOSMOS (patched) -----------------------
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_TinyUSB.h>

Adafruit_USBD_MIDI usb_midi;

// ==== RGB565 カラー定義 ====
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_DARK_GRAY 0x2104
#define COLOR_GRAY 0x8410   // 中間グレー（RGB565）
#define COLOR_ORANGE 0xFD20   // 明るいオレンジ

// ==== Waveshare Pico-LCD-1.3 ピン定義 ====
#define LCD_DC   8
#define LCD_CS   9
#define LCD_RST 12
#define LCD_BL  13
#define LCD_SCK 10
#define LCD_MOSI 11
#define LCD_SPI SPI1

// ---- A/B/X/Y ----
#define KEY_A_PIN    15
#define KEY_B_PIN    17
#define KEY_X_PIN    19
#define KEY_Y_PIN    21

// ---- 十字キー（固定）----
#define LEFT_PIN   14
#define DOWN_PIN   16
#define UP_PIN     2
#define RIGHT_PIN  3

// ---- Joystick ----
#define JOY_X_PIN 6
#define JOY_Y_PIN 7
#define JOY_SW_PIN 0   // ★ ここを 8 から変更（最重要）

// 5x7 ASCII フォント (32〜127)
const uint8_t font5x7[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // 32 ' '
  {0x00,0x00,0x5F,0x00,0x00}, // 33 '!'
  {0x00,0x07,0x00,0x07,0x00}, // 34 '"'
  {0x14,0x7F,0x14,0x7F,0x14}, // 35 '#'
  {0x24,0x2A,0x7F,0x2A,0x12}, // 36 '$'
  {0x23,0x13,0x08,0x64,0x62}, // 37 '%'
  {0x36,0x49,0x55,0x22,0x50}, // 38 '&'
  {0x00,0x05,0x03,0x00,0x00}, // 39 '''
  {0x00,0x1C,0x22,0x41,0x00}, // 40 '('
  {0x00,0x41,0x22,0x1C,0x00}, // 41 ')'
  {0x14,0x08,0x3E,0x08,0x14}, // 42 '*'
  {0x08,0x08,0x3E,0x08,0x08}, // 43 '+'
  {0x00,0x50,0x30,0x00,0x00}, // 44 ','
  {0x08,0x08,0x08,0x08,0x08}, // 45 '-'
  {0x00,0x60,0x60,0x00,0x00}, // 46 '.'
  {0x20,0x10,0x08,0x04,0x02}, // 47 '/'
  {0x3E,0x51,0x49,0x45,0x3E}, // 48 '0'
  {0x00,0x42,0x7F,0x40,0x00}, // 49 '1'
  {0x42,0x61,0x51,0x49,0x46}, // 50 '2'
  {0x21,0x41,0x45,0x4B,0x31}, // 51 '3'
  {0x18,0x14,0x12,0x7F,0x10}, // 52 '4'
  {0x27,0x45,0x45,0x45,0x39}, // 53 '5'
  {0x3C,0x4A,0x49,0x49,0x30}, // 54 '6'
  {0x01,0x71,0x09,0x05,0x03}, // 55 '7'
  {0x36,0x49,0x49,0x49,0x36}, // 56 '8'
  {0x06,0x49,0x49,0x29,0x1E}, // 57 '9'
  {0x00,0x36,0x36,0x00,0x00}, // 58 ':'
  {0x00,0x56,0x36,0x00,0x00}, // 59 ';'
  {0x08,0x14,0x22,0x41,0x00}, // 60 '<'
  {0x14,0x14,0x14,0x14,0x14}, // 61 '='
  {0x00,0x41,0x22,0x14,0x08}, // 62 '>'
  {0x02,0x01,0x51,0x09,0x06}, // 63 '?'
  {0x32,0x49,0x79,0x41,0x3E}, // 64 '@'
  {0x7E,0x11,0x11,0x11,0x7E}, // 65 'A'
  {0x7F,0x49,0x49,0x49,0x36}, // 66 'B'
  {0x3E,0x41,0x41,0x41,0x22}, // 67 'C'
  {0x7F,0x41,0x41,0x22,0x1C}, // 68 'D'
  {0x7F,0x49,0x49,0x49,0x41}, // 69 'E'
  {0x7F,0x09,0x09,0x09,0x01}, // 70 'F'
  {0x3E,0x41,0x49,0x49,0x7A}, // 71 'G'
  {0x7F,0x08,0x08,0x08,0x7F}, // 72 'H'
  {0x00,0x41,0x7F,0x41,0x00}, // 73 'I'
  {0x20,0x40,0x41,0x3F,0x01}, // 74 'J'
  {0x7F,0x08,0x14,0x22,0x41}, // 75 'K'
  {0x7F,0x40,0x40,0x40,0x40}, // 76 'L'
  {0x7F,0x02,0x0C,0x02,0x7F}, // 77 'M'
  {0x7F,0x04,0x08,0x10,0x7F}, // 78 'N'
  {0x3E,0x41,0x41,0x41,0x3E}, // 79 'O'
  {0x7F,0x09,0x09,0x09,0x06}, // 80 'P'
  {0x3E,0x41,0x51,0x21,0x5E}, // 81 'Q'
  {0x7F,0x09,0x19,0x29,0x46}, // 82 'R'
  {0x46,0x49,0x49,0x49,0x31}, // 83 'S'
  {0x01,0x01,0x7F,0x01,0x01}, // 84 'T'
  {0x3F,0x40,0x40,0x40,0x3F}, // 85 'U'
  {0x1F,0x20,0x40,0x20,0x1F}, // 86 'V'
  {0x7F,0x20,0x18,0x20,0x7F}, // 87 'W'
  {0x63,0x14,0x08,0x14,0x63}, // 88 'X'
  {0x07,0x08,0x70,0x08,0x07}, // 89 'Y'
  {0x61,0x51,0x49,0x45,0x43}, // 90 'Z'
  {0x00,0x7F,0x41,0x41,0x00}, // 91 '['
  {0x02,0x04,0x08,0x10,0x20}, // 92 '\'
  {0x00,0x41,0x41,0x7F,0x00}, // 93 ']'
  {0x04,0x02,0x01,0x02,0x04}, // 94 '^'
  {0x40,0x40,0x40,0x40,0x40}, // 95 '_'
  {0x00,0x01,0x02,0x04,0x00}, // 96 '`'
  {0x20,0x54,0x54,0x54,0x78}, // 97 'a'
  {0x7F,0x48,0x44,0x44,0x38}, // 98 'b'
  {0x38,0x44,0x44,0x44,0x20}, // 99 'c'
  {0x38,0x44,0x44,0x48,0x7F}, // 100 'd'
  {0x38,0x54,0x54,0x54,0x18}, // 101 'e'
  {0x08,0x7E,0x09,0x01,0x02}, // 102 'f'
  {0x0C,0x52,0x52,0x52,0x3E}, // 103 'g'
  {0x7F,0x08,0x04,0x04,0x78}, // 104 'h'
  {0x00,0x44,0x7D,0x40,0x00}, // 105 'i'
  {0x20,0x40,0x44,0x3D,0x00}, // 106 'j'
  {0x7F,0x10,0x28,0x44,0x00}, // 107 'k'
  {0x00,0x41,0x7F,0x40,0x00}, // 108 'l'
  {0x7C,0x04,0x18,0x04,0x78}, // 109 'm'
  {0x7C,0x08,0x04,0x04,0x78}, // 110 'n'
  {0x38,0x44,0x44,0x44,0x38}, // 111 'o'
  {0x7C,0x14,0x14,0x14,0x08}, // 112 'p'
  {0x08,0x14,0x14,0x14,0x7C}, // 113 'q'
  {0x7C,0x08,0x04,0x04,0x08}, // 114 'r'
  {0x48,0x54,0x54,0x54,0x20}, // 115 's'
  {0x04,0x3F,0x44,0x40,0x20}, // 116 't'
  {0x3C,0x40,0x40,0x20,0x7C}, // 117 'u'
  {0x1C,0x20,0x40,0x20,0x1C}, // 118 'v'
  {0x3C,0x40,0x30,0x40,0x3C}, // 119 'w'
  {0x44,0x28,0x10,0x28,0x44}, // 120 'x'
  {0x0C,0x50,0x50,0x50,0x3C}, // 121 'y'
  {0x44,0x64,0x54,0x4C,0x44}, // 122 'z'
  {0x00,0x08,0x36,0x41,0x00}, // 123 '{'
  {0x00,0x00,0x7F,0x00,0x00}, // 124 '|'
  {0x00,0x41,0x36,0x08,0x00}, // 125 '}'
  {0x08,0x04,0x08,0x10,0x08}, // 126 '~'
};

// ============================================================
// ★ BPM テーブル（X ボタン用）
// ============================================================
const int BPM_TABLE[] = {20, 80, 120, 140, 240};
const int BPM_COUNT = 5;

// ---- テンポ（内部 BPM のみ）----
int baseBPM  = 80;   // 基本 BPM
int stepBPM  = 80;   // ステップ進行用 BPM

int bpmIndex = 1; // 初期値 80BPM（TABLE[1])

int transpose = 3;   // -24〜+24 くらいまで対応（2オクターブ）

const int TRANSPOSE_LIST[] = { -10, -5, -4, 0, +4, +5, +10 };
const int TRANSPOSE_COUNT = 7;

// 8分 × 16 のリズムパターン
int rhythmPatterns[6][16] = {
    // パターン0：交互（基礎）
    {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},

    // パターン1：前打ち（前打ち）
    {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},

    // パターン2：余白多め（呼吸）
    {1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0},

    // パターン3：連打入り（勢い）
    {1,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0},

    // パターン4：後半寄り（タメ・尺八的）
    {0,0,1,0,0,1,0,1,1,0,1,1,0,1,0,1},

    // パターン5：3連（伸び）
    {1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0}
};

bool mainNoteExtended = false;

int currentPattern = 0;   // 現在のパターン
int mainDensity = 100;    // 発音率（0〜100%）

bool btnA = false;
bool btnB = false;
bool btnX = false;
bool btnY = false;

bool btnLeft = false;
bool btnRight = false;
bool btnUp = false;
bool btnDown = false;

bool btnSW = false;

// ★ 起動時アルペジオ再生済みフラグ
bool startupArpDone = false;

unsigned long autoModeTimer = 0;

unsigned long noteOffTime = 0;   // ノートを止める時刻
int noteLengthMin = 50;          // 最短音長（ミリ秒）
int noteLengthMax = 400;         // 最長音長（ミリ秒）

int noteDots[240] = { -1 };     // x=0〜239 にノートの高さを保存

bool noteIsOnB = false;
uint8_t lastNoteB = 0;
unsigned long noteOffTimeB = 0;

bool noteIsOnMain = false;
uint8_t lastNoteMain = 0;
unsigned long noteOffTimeMain = 0;

// ★ 8分ステップ管理
unsigned long lastStep = 0;
// ★ 8分の長さ（BPM から計算）
unsigned long interval = 0;

unsigned long nextMainSilenceTime = 0;
unsigned long mainSilenceDuration = 0;
bool mainSilenceActive = false;

unsigned long lastMainStepTime = 0;
unsigned long nextSilenceTime = 0;
int mainPattern[16];   // 0 = 休符, 1 = 鳴く
bool pendingPatternChange = false;

uint16_t bpmColor = COLOR_GREEN;   // Start時は白、Stop時は赤

unsigned long lastClockMicros = 0;

// ---- スケール定義（度数） ----
const uint8_t SCALE_HEI[]   = { 0, 2, 4, 7, 9 };   // 平調子（ヨナ抜き長音階）
const uint8_t SCALE_MIYA[]  = { 0, 1, 5, 7, 10 };   // 都節（C/Db/F/G/Bb）
const uint8_t SCALE_INSEN[] = { 0, 1, 5, 7, 8 };    // 陰旋法（C/Db/F/G/Ab）

const int SCALE_HEI_SIZE   = 5;
const int SCALE_MIYA_SIZE  = 5;
const int SCALE_INSEN_SIZE = 5;

int scaleMode = 0;  
// 0 = 平調子
// 1 = 都節
// 2 = 陰旋法

// ---- スケールごとの mainPattern 密度（鳴く確率 %） ----
int mainPatternDensity[3] = {
    80,
    60,
    70
};

// ---- スケールごとのアルペジオ速度（ミリ秒） ----
// 小さいほど速い
int arpSpeedTable[3] = {
    20,   // 平調子 → 明るく速い
    105,  // 都節   → 哀愁、少しゆっくり
    90    // 陰旋法 → 渋い、やや速め
};

// ---- スケールごとのアルペジオ幅（degree の増減幅） ----
int arpWidthTable[3] = {
    3,   // 平調子 → 跳ねる
    1,   // 都節   → 哀愁、狭い動き
    3    // 陰旋法 → 渋い、広い跳躍
};

// ---- スケールごとのアルペジオ方向 ----
//  1 = 上昇, -1 = 下降, 0 = ランダム
int arpDirTable[3] = {
     1   // 平調子 → 上昇
    -1,  // 都節   → 下降
    0    // 陰旋法 → ランダム
};

// ---- スケールごとのアルペジオ持続時間（方向反転回数） ----
int arpLengthTable[3] = {
    8,  // 平調子 → 長めに鳴く（明るく広がる）
    4,   // 都節   → 短め（哀愁、余韻を残す）
    30   // 陰旋法 → 長く粘る（尺八的）
};

// ---- スケールごとのアルペジオ間隔時間（ミリ秒） ----
// 小さいほど速い、値が大きいほどゆっくり
int arpTimeTable[3] = {
    40,   // 平調子（HEI） → 明るく速い
    70,   // 都節（MIYA） → 哀愁、ゆっくり
    50    // 陰旋法（INSEN） → 渋い、中速
};

// =====================================================
// ★ B パート専用リズムパターン（8分 × 16）
// =====================================================
int rhythmBPatterns[2][16] = {
    // パターン0：基本
    //{1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},

    // パターン1：4分の地鳴り（ドン……ドン……）
    {1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},
};

int currentBPattern = 0;

int noteToY(uint8_t note) {
    return map(note, 36, 84, 240, 150);  // 下が低音、上が高音
}

int muteState = 0;  
// 0 = A mute
// 1 = B mute
// 2 = ALL mute
// 3 = A unmute
// 4 = B unmute
bool muteA = false;
bool muteB = false;

// ---- 音色番号（LCD表示用）----
int programA = 11;   // ch1 初期音色
int programB = 8;    // ch2 初期音色

// ---- ボタン状態管理 ----
bool lastAState = false;
bool lastBState = false;

unsigned long pressStartA = 0;
unsigned long pressStartB = 0;

unsigned long lastStepA = 0;   // ← 外に出す（重要）
unsigned long lastStepB = 0;   // ← 外に出す（重要）

// ---- X ボタン状態管理 ----
bool lastX = false;
unsigned long pressStartX = 0;
unsigned long lastStepX = 0;

bool isPlaying = false;

void usb_send_note_on(uint8_t note, uint8_t vel, uint8_t ch) {
    uint8_t msg[3] = { uint8_t(0x90 | (ch & 0x0F)), note, vel };
    usb_midi.write(msg, 3);
}

void usb_send_note_off(uint8_t note, uint8_t ch) {
    uint8_t msg[3] = { uint8_t(0x80 | (ch & 0x0F)), note, 0 };
    usb_midi.write(msg, 3);
}

inline void midi_bridge_send_note_on(uint8_t note, uint8_t vel, uint8_t ch=0){
    // 内部シンセへ送る（Core1）
    MidiEvent ev{EV_NOTE_ON,note,vel,ch};
    MidiQ::push(ev);

    // USB MIDI へ送る（Core0）
    usb_send_note_on(note, vel, ch);
}

inline void midi_bridge_send_note_off(uint8_t note, uint8_t ch=0){
    MidiEvent ev{EV_NOTE_OFF,note,0,ch};
    MidiQ::push(ev);

    usb_send_note_off(note, ch);
}

// ============================================================
// ★ Yボタン（リズムパターン切替）
// ============================================================
bool lastY = false;
unsigned long pressStartY = 0;
unsigned long lastStepY = 0;

const int RHYTHM_PATTERN_COUNT = 5;

void readButtons() {
}

void drawPlayIndicator(uint16_t color) {
}

void drawMuteStatus() {
}

void drawScaleName() {
}

void drawProgramInfo() {
}

// 中心値
int centerX = 0;
int centerY = 0;

// 移動平均用バッファ
const int FILTER_N = 20;
int bufX[FILTER_N];
int bufY[FILTER_N];
int bufIndex = 0;

// 現在の方向
int joyXState = 0;  // -1=左, 0=中心, 1=右
int joyYState = 0;

// 1回だけ反応
bool joyLeftOnce = false;
bool joyRightOnce = false;
bool joyUpOnce = false;
bool joyDownOnce = false;

void calibrateJoystick() {
}

int joyX = 0;
int joyY = 0;

void readJoystick() {
}

// ==== LCD コマンド送信 ====
void lcdCmd(uint8_t cmd) {
  digitalWrite(LCD_DC, LOW);
  digitalWrite(LCD_CS, LOW);
  LCD_SPI.transfer(cmd);
  digitalWrite(LCD_CS, HIGH);
}

void lcdData(uint8_t dat) {
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  LCD_SPI.transfer(dat);
  digitalWrite(LCD_CS, HIGH);
}

// ==== LCD 初期化（Waveshare 純正） ====
void lcdInit() {
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_BL, OUTPUT);

  digitalWrite(LCD_BL, LOW);
  digitalWrite(LCD_CS, HIGH);

  LCD_SPI.setSCK(LCD_SCK);
  LCD_SPI.setTX(LCD_MOSI);
  LCD_SPI.begin();
  LCD_SPI.beginTransaction(SPISettings(24000000, MSBFIRST, SPI_MODE0));

  // ハードウェアリセット
  digitalWrite(LCD_RST, LOW); delay(10);
  digitalWrite(LCD_RST, HIGH); delay(120);

  // ==== Waveshare ST7789 初期化コマンド ====
  lcdCmd(0x36); lcdData(0x70);
  lcdCmd(0x3A); lcdData(0x05);
  lcdCmd(0xB2); lcdData(0x0C); lcdData(0x0C); lcdData(0x00); lcdData(0x33); lcdData(0x33);
  lcdCmd(0xB7); lcdData(0x35);
  lcdCmd(0xBB); lcdData(0x19);
  lcdCmd(0xC0); lcdData(0x2C);
  lcdCmd(0xC2); lcdData(0x01);
  lcdCmd(0xC3); lcdData(0x12);
  lcdCmd(0xC4); lcdData(0x20);
  lcdCmd(0xC6); lcdData(0x0F);
  lcdCmd(0xD0); lcdData(0xA4); lcdData(0xA1);
  lcdCmd(0xE0); for (int i = 0; i < 14; i++) lcdData(0x00);
  lcdCmd(0xE1); for (int i = 0; i < 14; i++) lcdData(0x00);
  lcdCmd(0x21);
  lcdCmd(0x11); delay(120);
  lcdCmd(0x29); delay(20);

  digitalWrite(LCD_BL, HIGH); // バックライト ON
}

// ==== 描画ユーティリティ ====
void lcdSetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  lcdCmd(0x2A);
  lcdData(x0 >> 8); lcdData(x0 & 0xFF);
  lcdData(x1 >> 8); lcdData(x1 & 0xFF);

  lcdCmd(0x2B);
  lcdData(y0 >> 8); lcdData(y0 & 0xFF);
  lcdData(y1 >> 8); lcdData(y1 & 0xFF);

  lcdCmd(0x2C);
}

void lcdFill(uint16_t color) {
  lcdSetWindow(0, 0, 239, 239);
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  for (uint32_t i = 0; i < 240UL * 240UL; i++) {
    LCD_SPI.transfer(color >> 8);
    LCD_SPI.transfer(color & 0xFF);
  }
  digitalWrite(LCD_CS, HIGH);
}

void lcdDrawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= 240 || y < 0 || y >= 240) return;
  lcdSetWindow(x, y, x, y);
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  LCD_SPI.transfer(color >> 8);
  LCD_SPI.transfer(color & 0xFF);
  digitalWrite(LCD_CS, HIGH);
}

void lcdDrawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
  if (c < 32 || c > 126) return;
  const uint8_t *bitmap = font5x7[c - 32];

  for (int col = 0; col < 5; col++) {
    uint8_t line = bitmap[col];
    for (int row = 0; row < 7; row++) {
      uint16_t drawColor = (line & 0x01) ? color : bg;
      lcdFillRect(x + col * size, y + row * size, size, size, drawColor);
      line >>= 1;
    }
  }
}

void lcdFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (w <= 0 || h <= 0) return;
  lcdSetWindow(x, y, x + w - 1, y + h - 1);
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_CS, LOW);
  for (int i = 0; i < w * h; i++) {
    LCD_SPI.transfer(color >> 8);
    LCD_SPI.transfer(color & 0xFF);
  }
  digitalWrite(LCD_CS, HIGH);
}

void lcdPrint(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
  while (*str) {
    lcdDrawChar(x, y, *str, color, bg, size);
    x += 6 * size;
    str++;
  }
}

// =====================================================
//  UI パラメータ
// =====================================================
int steps = 16;
int hits = 5;
int rotation = 0;

int pattern[16];
int prob[16];

int currentStep = 0;
int selectedStep = 0;

// =====================================================
//  Euclid パターン生成
// =====================================================
void makeEuclid(int steps, int hits, int rot, int *pattern) {
  for (int i = 0; i < steps; i++) pattern[i] = 0;

  int bucket = 0;
  for (int i = 0; i < steps; i++) {
    bucket += hits;
    if (bucket >= steps) {
      bucket -= steps;
      int idx = (i + rot) % steps;
      pattern[idx] = 1;   // ★ int で 1 を入れる
    }
  }
}

// =====================================================
//  UI 全体描画
// =====================================================
void drawUI() {
}

void drawOneProbabilityBar(int i) {
}

void updateProbabilityBars() {
}

void updateStepBars() {
    static int lastStep = -1;
    static int lastPattern[16];

    int x0 = 5;
    int y0 = 120;
    int barWidth = 12;
    int barHeight = 20;
    int gap = 3;

    // ★ パターン変化チェック
    bool patternChanged = false;
    for (int i = 0; i < 16; i++) {
        if (mainPattern[i] != lastPattern[i]) {
            patternChanged = true;
            break;
        }
    }

    // ★ パターンが変わったら全バーを描き直す
    if (patternChanged) {
        for (int i = 0; i < 16; i++) {
            int x = x0 + i * (barWidth + gap);

            uint16_t color;
            if (mainPattern[i] == 1)
                color = COLOR_GREEN;   // パターンON → 緑色
            else
                color = COLOR_GRAY;     // パターンOFF → 灰色

            lcdFillRect(x, y0, barWidth, barHeight, color);
            lastPattern[i] = mainPattern[i];
        }
    }

    // ★ ステップ移動時
    if (currentStep != lastStep) {

        // 前ステップを元の色に戻す
        if (lastStep >= 0) {
            int x = x0 + lastStep * (barWidth + gap);

            uint16_t color;
            if (mainPattern[lastStep] == 1)
                color = COLOR_GREEN;
            else
                color = COLOR_GRAY;

            lcdFillRect(x, y0, barWidth, barHeight, color);
        }

        // ★ 現在ステップを赤 or オレンジで塗る
        int x = x0 + currentStep * (barWidth + gap);

        if (mainNoteExtended) {
            lcdFillRect(x, y0, barWidth, barHeight, COLOR_ORANGE);  // 伸びてるとき
        } else {
            lcdFillRect(x, y0, barWidth, barHeight, COLOR_RED);     // 通常
        }

        lastStep = currentStep;
    }
}

void updateStepDots() {
    static int lastStep = -1;

    if (currentStep != lastStep) {
        int x0 = 5;
        int y0 = 160;
        int gap = 14;

        // 前のステップを灰色に戻す
        if (lastStep >= 0) {
            lcdPrint(x0 + lastStep * gap, y0, "・", COLOR_DARK_GRAY, COLOR_BLACK, 1);
        }

        // 現在のステップを赤に
        lcdPrint(x0 + currentStep * gap, y0, "●", COLOR_RED, COLOR_BLACK, 1);

        lastStep = currentStep;
    }
}

// 長押し判定用
unsigned long holdStartHits = 0;
unsigned long holdStartRot  = 0;

unsigned long swPressStart = 0;

// ランダムモード
// 0 = Euclidランダム
// 1 = ステップランダム
// 2 = 両方ランダム
int randomMode = 1;


bool updateEuclidEdit() {

  bool changed = false;  // ★ 変更があったかどうか

  // --- まず同時押しを最優先で判定する ---
  if (btnLeft && btnRight) {
    executeRandom();
    changed = true;   // ★ ランダム実行は確実に変更
    return changed;
  }

  if (btnUp && btnDown) {
    randomMode = (randomMode + 1) % 3;
    drawRandomMode();
    changed = true;   // ★ モード変更も変更扱い
    return changed;
  }

  // --- ここから単押し処理（hits / rotation） ---
  static unsigned long holdStartHits = 0;
  static unsigned long holdStartRot  = 0;

  int oldHits = hits;
  int oldRot  = rotation;

  // hits 編集（左右）
  if (btnRight) {
    if (holdStartHits == 0) {
      holdStartHits = millis();
      hits++;
    } else if (millis() - holdStartHits > 300) {
      if ((millis() - holdStartHits) % 50 == 0) hits++;
    }
  }
  else if (btnLeft) {
    if (holdStartHits == 0) {
      holdStartHits = millis();
      hits--;
    } else if (millis() - holdStartHits > 300) {
      if ((millis() - holdStartHits) % 50 == 0) hits--;
    }
  }
  else {
    holdStartHits = 0;
  }

  // rotation 編集（上下）
  if (btnUp) {
    if (holdStartRot == 0) {
      holdStartRot = millis();
      rotation++;
    } else if (millis() - holdStartRot > 300) {
      if ((millis() - holdStartRot) % 50 == 0) rotation++;
    }
  }
  else if (btnDown) {
    if (holdStartRot == 0) {
      holdStartRot = millis();
      rotation--;
    } else if (millis() - holdStartRot > 300) {
      if ((millis() - holdStartRot) % 50 == 0) rotation--;
    }
  }
  else {
    holdStartRot = 0;
  }

  // ★ hits または rotation が変わったら changed = true
  if (hits != oldHits || rotation != oldRot) {
    changed = true;
  }

  // Euclid パターンを再生成
  makeEuclid(steps, hits, rotation, pattern);

  return changed;
}

// =====================================================
// ★ ランダムモード実行
//    randomMode:
//      0 = Euclid ランダム（steps/hits/rotation）
//      1 = Step ランダム（prob[]）
//      2 = 両方ランダム
// =====================================================
void executeRandom() {

    // -----------------------------------------
    // ★ Euclid（リズム）→ 全モードで鳴く密度を統一
    // -----------------------------------------
    if (randomMode == 0 || randomMode == 2) {

        steps = 16;                 // ★ 全モード8分固定
        hits  = random(3, 6);       // ★ 鳴く密度を低めに統一（最重要）
        rotation = random(0, steps);

        makeEuclid(steps, hits, rotation, pattern);
    }

    // -----------------------------------------
    // ★ Step（音程揺らぎ）→ 全モードで揺らぎを弱める
    // -----------------------------------------
    if (randomMode == 1 || randomMode == 2) {

        for (int i = 0; i < 16; i++) {

            // ★ 全モードで揺らぎ弱め（pattern が 1 になりすぎない）
            pattern[i] = random(-1, 1);   // -1,0
        }
    }

    // -----------------------------------------
    // ★ Probability（鳴く確率）→ 全モードで統一
    // -----------------------------------------
    for (int i = 0; i < 16; i++) {

        if (pattern[i] != 0) {
            prob[i] = random(50, 80);   // ★ 鳴く確率を中程度に統一
        } else {
            prob[i] = random(10, 40);   // ★ 休符ステップは低め
        }
    }

    // -----------------------------------------
    // ★ MainPattern（8分の ON/OFF）ランダム生成
    // -----------------------------------------
    int density = mainPatternDensity[scaleMode];  // ★ スケールごとの密度

    for (int i = 0; i < 16; i++) {
      int r = random(0, 100);
      mainPattern[i] = (r < density ? 1 : 0);
    }
}

// ---- 音名テーブル（シャープ表記）----
const char* noteNames[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// ---- 音名を返す関数 ----
const char* getNoteName(uint8_t note) {
    int idx = note % 12;
    return noteNames[idx];
}

void drawRandomMode() {
}

void drawGenerativeBackground() {
}

void drawNoteDots() {
}

void sendNoteOnCh(uint8_t note, uint8_t velocity, uint8_t ch) {
    uint8_t msg[3] = { (uint8_t)(0x90 | (ch & 0x0F)), note, velocity };
    usb_midi.write(msg, 3);
    midi_bridge_send_note_on(note, velocity, ch);
}

void sendNoteOffCh(uint8_t note, uint8_t ch) {
    uint8_t msg[3] = { (uint8_t)(0x80 | (ch & 0x0F)), note, 0 };
    usb_midi.write(msg, 3);
    midi_bridge_send_note_off(note, ch);
}

const uint8_t scale[] = { 0, 2, 4, 7, 9 };  
// Cメジャーペンタの度数（0=ルート）
const int scaleSize = 5;

uint8_t generateNote() {

    // 80%でスケール音、20%でランダム
    bool useScale = (rand() % 100) < 80;

    if (useScale) {
        // スケール音
        int degree = rand() % scaleSize;  // 0〜4
        int octave = 48;                  // C3を基準にする
        return octave + scale[degree];
    } else {
        // ランダム音（味付け）
        return 48 + (rand() % 24);  // C3〜B4
    }
}

uint8_t lastNote = 0;
bool noteIsOn = false;

unsigned long clockInterval = (60000UL / baseBPM) / 24;
unsigned long lastClockTime = 0;

void pushNoteDot(uint8_t note) {
  int y = noteToY(note);

  for (int i = 0; i < 239; i++) {
    noteDots[i] = noteDots[i+1];
  }
  noteDots[239] = y;   // ★ Y座標を入れる
}

void drawTopText() {
}

// -----------------------------------------------------
// ★ スケール配列の中で note が何番目かを返す
//    見つからない場合は最も近い音を返す（安全）
// -----------------------------------------------------
int findScaleIndex(uint8_t note) {
    int bestIndex = 0;
    int bestDiff  = 9999;

    for (int i = 0; i < scaleSize; i++) {
        int diff = abs((int)note - (int)scale[i]);
        if (diff < bestDiff) {
            bestDiff  = diff;
            bestIndex = i;
        }
    }
    return bestIndex;
}

// ---- グローバルに置く（重要）----
int degreeA = 0;   // スケール内の現在位置
int dirA    = 1;   // 上昇(+1) / 下降(-1)
int degreeArp = 0;
int dirArp    = 1;

// =====================================================
// ★ メインメロディ生成（陰音階＋パターン揺らぎ＋トリルフラグ）
// =====================================================
uint8_t generateNoteA(bool &trillFlag, int &degreeOut) {

    // ---- スケール選択 ----
    const uint8_t* scale;
    int scaleSize;

    if (scaleMode == 0) { 
        scale = SCALE_HEI;  
        scaleSize = SCALE_HEI_SIZE; 
    }
    else if (scaleMode == 1) { 
        scale = SCALE_MIYA; 
        scaleSize = SCALE_MIYA_SIZE; 
    }
    else { 
        scale = SCALE_INSEN; 
        scaleSize = SCALE_INSEN_SIZE; 
    }

    // ---- ランダム方向転換（自然な揺れ）----
    if (random(0, 100) < 20) {
        dirA = -dirA;
    }

    // ---- degree 更新 ----
    degreeA += dirA;

    // ---- 範囲外なら反転して戻す（暴走防止の最重要ポイント）----
    if (degreeA < 0) {
        degreeA = 1;
        dirA = 1;
    }
    if (degreeA >= scaleSize) {
        degreeA = scaleSize - 2;
        dirA = -1;
    }

    degreeOut = degreeA;

    // ---- MIDI ノート生成（C3=48 基準）----
    uint8_t note = 48 + scale[degreeA];
    note += transpose;

    return note;
}


int downBias = 50;
int upBias = 50;

// B の刻み間隔（4分 or 8分）
int noteBDiv = 2;  
// 2 = 4分（8分×2）
// 1 = 8分（8分×1）

uint8_t generateNoteB() {

    const uint8_t* sc;

    if (scaleMode == 0) sc = SCALE_HEI;
    else if (scaleMode == 1) sc = SCALE_MIYA;
    else sc = SCALE_INSEN;

    int baseOct = 48;  // C3（低音）
    return baseOct + transpose + sc[0];  // ルートのみ
}


// =====================================================
// ★ モード切替インターバル
// =====================================================
unsigned long lastModeChange = 0;
unsigned long modeInterval   = 10000;  // 初期値（あとでランダムに更新）

// =====================================================
// ★ モード切替インターバル
// =====================================================
unsigned long ccDisplayTimer = 0;

void drawCCValue(byte cc, byte value) {
}

void drawEuclidParams() {
}

// ★ 小休止用
unsigned long lastRest = 0;
bool inRest = false;
int restStepsRemaining = 0; // ★ 休止ステップ数

int sustainBias = 125;

void handleCC(byte channel, byte number, byte value) {
    MidiEvent ev{EV_CC, number, value, channel};
    MidiQ::push(ev);
}

bool ghost = false;

// ★ キーチェンジ用
unsigned long lastKeyChange = 0;
unsigned long nextKeyChangeInterval = 20000;

unsigned long nextRestInterval = 20000; // 初期値

// ★ 微細揺らぎ
void microBpmNudge() {
    int n = random(-3, 4);  // -3〜+3
    baseBPM = constrain(baseBPM + n, 40, 260);
}

int snapToHeichoshi(int raw, int transpose) {

    const int heichoshi[5] = {0, 1, 5, 7, 8};

    int pitch = (raw - transpose) % 12;
    if (pitch < 0) pitch += 12;

    int best = heichoshi[0];
    int bestDiff = abs(pitch - heichoshi[0]);

    for (int i = 1; i < 5; i++) {
        int diff = abs(pitch - heichoshi[i]);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = heichoshi[i];
        }
    }

    return (raw - pitch) + best;
}

unsigned long silenceLength = 0;

void randomizeMainPattern() {
    for (int i = 0; i < 16; i++) {

        int r = random(0, 100);

        if (i > 0 && mainPattern[i-1] == 0) {
            // 連続休符を避ける → 休符後は鳴く確率を上げる
            mainPattern[i] = (r < 20 ? 0 : 1);
        } else {
            // 通常
            mainPattern[i] = (r < 40 ? 0 : 1);
        }
    }
}

void drawAllStepBars() {
}

int phraseDir = 1;   // +1 = 上行, -1 = 下降

void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel = 0) {
    midi_bridge_send_cc(cc, value, channel);
}

void sendAllNotesOff() {
    for (int ch = 0; ch < 16; ch++) {
        sendControlChange(123, 0, ch);  // CC123 = All Notes Off
    }
}

void safeNoteOffA() {
    if (noteIsOnMain) {
        sendNoteOffCh(lastNoteMain, 0);
        noteIsOnMain = false;
    }
}

int mainDegree = 8;
bool mainGoingDown = true;

uint8_t mapToScale(uint8_t note, const uint8_t* sc, int scSize) {
    int base = (note / 12) * 12;
    int best = base + sc[0];
    int bestDist = abs(note - best);

    for (int i = 1; i < scSize; i++) {
        int cand = base + sc[i];
        int dist = abs(note - cand);
        if (dist < bestDist) {
            best = cand;
            bestDist = dist;
        }
    }
    return best;
}

int findNearestDegree(uint8_t note, const uint8_t* sc, int scSize, int transpose) {
    int root = 60 + transpose;
    int rel = note - root;  // 半音差

    // スケールの中で最も近い度数を探す
    int bestDeg = 0;
    int bestDist = abs(rel - sc[0]);

    for (int i = 1; i < scSize; i++) {
        int dist = abs(rel - sc[i]);
        if (dist < bestDist) {
            bestDist = dist;
            bestDeg = i;
        }
    }
    return bestDeg;
}

void drawSplash() {
}

void playStartupArp() {
    uint8_t notes[3] = { 52, 60, 69 }; // E3, C4, A4
    for (int i = 0; i < 3; i++) {
        midi_bridge_send_note_on(notes[i], 100, 0);
        delay(180);
        midi_bridge_send_note_off(notes[i], 0);
    }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Core0: setup start");
  
  pinMode(25, OUTPUT);      // Pico 本体 LED
  digitalWrite(25, HIGH);   // 起動後ずっと点灯

  usb_midi.begin();  
  
  for (int i = 0; i < 16; i++) prob[i] = 100;

  executeRandom();
  transpose += 5;
  
  // ---- 起動時アルペジオ初期化（美しい低音スタート） ----
  int baseDegree = SCALE_HEI[0];   // 平調子の最も低い度数（D）
  int baseOct = 6;                 // C6

  lastNoteMain = baseOct * 12 + baseDegree + transpose;

  nextSilenceTime = millis() + 2500;

  randomizeMainPattern();

  // ---- 起動時は必ず平調子でスタート ----
  scaleMode = 0;        // 平調子

  // ---- autoMode の暴走防止（起動直後に切り替わらないように）----
  autoModeTimer = millis() + 10000;  // 10秒後に初回切替

  degreeA = 0;
  dirA = 1;

  executeRandom();      // 平調子の密度・確率でパターン生成

  nextMainSilenceTime = millis() + random(8000, 15000);  // 8〜15秒後
  
  for (int i = 0; i < 240; i++) noteDots[i] = -1;

  // ★★★ Core0 の初期化が全部終わってから Core1 を起動する ★★★
  multicore_launch_core1(core1_main);

  Serial.println("Core0: setup done, Core1 launched");
}

// ★ アルペジオ終了後に即メインへ吸着させるフラグ
bool forceMainStep = false;
bool arpWantsToEnd = false;   // ★ 終了希望フラグ（実際にはまだ終わらない）

struct ArpEngine {
    bool active = false;

    uint8_t baseNote = 60;
    int scaleMode = 0;

    int currentDegree = 0;
    int octave = 0;

    bool arpGoingUp = true;     // ★ 上昇フェーズか下降フェーズか
    int remainingUpSteps = 0;   // ★ 上昇の残りステップ
    int remainingDownSteps = 0; // ★ 下降の残りステップ

    uint32_t nextStepMs = 0;

    // ★ 上昇・下降でスピードを変える
    uint32_t upIntervalMs = 80;     // 上昇スピード
    uint32_t downIntervalMs = 120;  // 下降スピード

    bool noteOn = false;
    uint8_t lastNote = 0;

    bool stopOnUp = false;
    bool stopOnDown = false;
};

ArpEngine g_arp;

const uint8_t* getScale(int mode, int &size) {
    if (mode == 0) { size = SCALE_HEI_SIZE;   return SCALE_HEI; }
    if (mode == 1) { size = SCALE_MIYA_SIZE;  return SCALE_MIYA; }
    size = SCALE_INSEN_SIZE; return SCALE_INSEN;
}

void arp_start(uint8_t baseNote, int scaleMode, int bpm) {
    g_arp.active = true;
    g_arp.baseNote = baseNote;
    g_arp.scaleMode = scaleMode;

    int scSize;
    getScale(scaleMode, scSize);

    // ★ 開始は必ず上昇
    g_arp.arpGoingUp = true;

    // ★ 上昇は低めから
    g_arp.currentDegree = 0;
    g_arp.octave = 0;

    // ★ 長さ候補
    const int lengthChoices[] = {4, 8};
    const int lengthCount = 2;

    // ★ 上昇・下降の長さを候補からランダム選択
    g_arp.remainingUpSteps   = lengthChoices[random(0, lengthCount)];
    //g_arp.remainingDownSteps = lengthChoices[random(0, lengthCount)];
    g_arp.remainingDownSteps = random(24, 48);


    // ★ 停止は「下降が終わったときだけ」
    g_arp.stopOnUp = false;                 // 上昇では絶対止まらない
    g_arp.stopOnDown = true;

    // 基本スピード
    uint32_t baseUp   = 60000UL / bpm / 8;  // 上昇
    uint32_t baseDown = 60000UL / bpm / 8;  // 下降

    // ランダム倍率（0.7〜1.3倍）
    float upMul   = random(70, 130) / 100.0f;
    float downMul = random(70, 130) / 100.0f;

    g_arp.upIntervalMs   = baseUp   * upMul;
    g_arp.downIntervalMs = baseDown * downMul;
}

void arp_stop() {
    if (g_arp.noteOn) {
        midi_bridge_send_note_off(g_arp.lastNote, 0);
        g_arp.noteOn = false;
    }
    g_arp.active = false;
}

void arp_update(uint32_t nowMs) {
    if (!g_arp.active) return;
    if (nowMs < g_arp.nextStepMs) return;

    // 前のノートを切る
    if (g_arp.noteOn) {
        midi_bridge_send_note_off(g_arp.lastNote, 0);
        g_arp.noteOn = false;
    }

    int scSize;
    const uint8_t* sc = getScale(g_arp.scaleMode, scSize);

    // ★ ノート決定（共通）
    uint8_t note = g_arp.baseNote + g_arp.octave * 12 + sc[g_arp.currentDegree];
    midi_bridge_send_note_on(note, 90, 0);

    g_arp.lastNote = note;
    g_arp.noteOn = true;

    // =====================================================
    // ★ 上昇フェーズ
    // =====================================================
    if (g_arp.arpGoingUp) {

        int up = random(1, 3);  // 1〜2音上昇
        g_arp.currentDegree += up;

        // スケール上端処理
        if (g_arp.currentDegree >= scSize) {
            g_arp.currentDegree -= scSize;
            g_arp.octave++;
        }

        g_arp.remainingUpSteps--;
        if (g_arp.remainingUpSteps <= 0) {

            // ★ 上昇では絶対に止めない
            // → 下降へ切り替え
            g_arp.arpGoingUp = false;
            g_arp.currentDegree = scSize - 1;

            g_arp.nextStepMs = nowMs + g_arp.downIntervalMs;
            return;
        }

        g_arp.nextStepMs = nowMs + g_arp.upIntervalMs;
        return;
    }

    // =====================================================
    // ★ 下降フェーズ
    // =====================================================
    //int drop = random(1, 3);  // 1〜2音下降（安定・自然・音数が増える）
    int drop = random(1, 3);  // 1〜2音下降（安定・自然・音数が増える）

    g_arp.currentDegree -= drop;

    // スケール下端処理
    if (g_arp.currentDegree < 0) {
        g_arp.currentDegree = scSize - 1;
        g_arp.octave--;

        if (g_arp.octave < 0) {
            arp_stop();
            return;
        }
    }

    g_arp.remainingDownSteps--;
    if (g_arp.remainingDownSteps <= 0) {

        // ★ 下降後だけ停止判定
        if (g_arp.stopOnDown) {
            arp_stop();
            return;
        }

        // ★ 停止しない → 上昇へ戻る
        g_arp.arpGoingUp = true;
        g_arp.currentDegree = 0;
        g_arp.octave = random(0, 2);
        g_arp.remainingUpSteps = random(6, 20);

        g_arp.nextStepMs = nowMs + g_arp.upIntervalMs;
        return;
    }

    // 通常下降の次ステップ
    g_arp.nextStepMs = nowMs + g_arp.downIntervalMs;
}


// ★ アルペジオ頻度アップ用
unsigned long nextArpChanceTime = 0;

void loop() {
    // =====================================================
    // ★ USB MIDI 受信（1バイトずつ読み取り → 3バイト組み立て）
    // =====================================================
    static uint8_t msg[3];
    static uint8_t idx = 0;

    int b;
    while ((b = usb_midi.read()) != -1) {

        if (b & 0x80) {
            idx = 0;
            msg[idx++] = b;
        } else {
            if (idx < 3) msg[idx++] = b;
        }

        if (idx == 3) {

            uint8_t status = msg[0] & 0xF0;
            uint8_t ch     = msg[0] & 0x0F;
            uint8_t d1     = msg[1];
            uint8_t d2     = msg[2];

            // ---- Note On ----
            if (status == 0x90 && d2 > 0) {
                MidiEvent ev{EV_NOTE_ON, d1, d2, ch};
                MidiQ::push(ev);
            }

            // ---- Note Off ----
            else if (status == 0x80 || (status == 0x90 && d2 == 0)) {
                MidiEvent ev{EV_NOTE_OFF, d1, 0, ch};
                MidiQ::push(ev);
            }

            else if (status == 0xB0) {

                uint8_t cc  = d1;
                uint8_t val = d2;

                bool handled = false;

                // ★ CC82 → Bパート Volume（Core1 CC200)
                if (cc == 82) {
                    midi_bridge_send_cc(200, val, 1);   // ch=1
                    handled = true;
                }

                // ★ CC81 → スケール切替
                if (cc == 81) {
                    scaleMode = val % 3;
                    executeRandom();
                    handled = true;
                }

                // ★ CC20 = Density
                if (cc == 20) {
                    mainDensity = map(val, 0, 127, 0, 100);
                    handled = true;
                }

                // ★ CC22 = Speed
                if (cc == 22) {
                    baseBPM = map(val, 0, 127, 20, 240);
                    interval = (60000 / baseBPM) / 2;
                    handled = true;
                }

                // ★ CC106 = パラメータ書き込み
                if (cc == 106) {
                    if (val > 0) {
                        // 保存処理
                    }
                    handled = true;
                }

                // ★ handled されていない CC だけ Core1 に送る
                if (!handled) {
                    MidiEvent ev{EV_CC, cc, val, ch};
                    MidiQ::push(ev);
                }
            }

            idx = 0;
        }
    }

    // ★ 超安定 MIDI Clock（24ppqn）
    unsigned long nowMicros = micros();
    unsigned long clockIntervalMicros = (60000000UL / baseBPM) / 24;

    if (nowMicros - lastClockMicros >= clockIntervalMicros) {
        lastClockMicros += clockIntervalMicros;  // ← これが最重要（揺れゼロ）
        usb_midi.write(0xF8);  // MIDI Clock
    }
    
    static unsigned long lastFrame = 0;
    unsigned long now_us = micros();
    if (now_us - lastFrame < 300) return;
    lastFrame = now_us;

    uint32_t now = millis();

    if (interval < 10) interval = 10;

    // =====================================================
    // NoteOff（メイン / B）
    // =====================================================
    if (noteIsOnMain && now >= noteOffTimeMain) {
        midi_bridge_send_note_off(lastNoteMain, 0);
        noteIsOnMain = false;
    }
    if (noteIsOnB && now >= noteOffTimeB) {
        midi_bridge_send_note_off(lastNoteB, 1);
        noteIsOnB = false;
    }

    // =====================================================
    // メインサイレンス制御（開始）
    // =====================================================
    if (!mainSilenceActive && now >= nextMainSilenceTime) {
        mainSilenceActive = true;

        // メインを無音化
        for (int i = 0; i < 16; i++) mainPattern[i] = 0;

        // メイン音を強制停止
        if (noteIsOnMain) {
            midi_bridge_send_note_off(lastNoteMain, 0);
            noteIsOnMain = false;
        }

        // B パートも強制停止
        if (noteIsOnB) {
            midi_bridge_send_note_off(lastNoteB, 1);
            noteIsOnB = false;
        }

        // アルペジオも強制停止
        if (g_arp.active) {
            arp_stop();
        }

        mainSilenceDuration = now + random(5000, 8000);
    }

    // =====================================================
    // メインサイレンス制御（終了）
    // =====================================================
    if (mainSilenceActive && now >= mainSilenceDuration) {
        mainSilenceActive = false;
        
        // ★ ここでパターン更新を必ず行う
        currentPattern = random(0, 6);
        memcpy(mainPattern, rhythmPatterns[currentPattern], sizeof(mainPattern));

        // B パターンを復活
        currentBPattern = 0;

        arp_start(60 + transpose, scaleMode, stepBPM);
        executeRandom();
        nextMainSilenceTime = now + random(30000, 50000);
    }

    // =====================================================
    // 呼吸 BPM / 自動トランスポーズ
    // =====================================================
    stepBPM = baseBPM;

    static unsigned long nextTransposeTime = millis() + random(5000, 15000);
    if (now >= nextTransposeTime) {

        // ★ 7つの候補からランダムに選ぶ
        int idx = random(0, TRANSPOSE_COUNT);
        transpose = TRANSPOSE_LIST[idx];

        nextTransposeTime = now + random(5000, 15000);
    }

    // =====================================================
    // ランダムでアルペジオを挿入
    // =====================================================
    if (!g_arp.active && now >= nextArpChanceTime) {
        nextArpChanceTime = now + random(8000, 12000);
        if (!g_arp.active) {
            uint8_t base = 60 + transpose;
            arp_start(base, scaleMode, stepBPM);
        }
    }

    // =====================================================
    // モード切替
    // =====================================================
    static int pendingScale = -1;
    static int pendingRandom = -1;

    if (now >= autoModeTimer) {

        autoModeTimer = now + random(80000, 120000);

        // ★ ランダムモード切替（既存）
        pendingRandom = (randomMode + 1) % 3;

        // ★ スケール切替（0→1→2→0…）
        pendingScale = (scaleMode + 1) % 3;

    }

    if (!g_arp.active) {

        if (pendingRandom != -1) {
            randomMode = pendingRandom;
            pendingRandom = -1;
        }

        if (pendingScale != -1) {
            scaleMode = pendingScale;
            pendingScale = -1;

            // ★ スケールが変わったらパターン再生成
            executeRandom();
        }
    }

    // =====================================================
    // メインステップ（8分 × 16）
    // =====================================================
    interval = 60000UL / max(stepBPM, 30) / 4;
 
    if (now - lastMainStepTime >= interval) {
        lastMainStepTime = now;
        currentStep = (currentStep + 1) % 16;

        // B パターン更新（無音中は変えない）
        if (!mainSilenceActive && currentStep == 0) {
            currentBPattern = 0;
        }

        // メインパターン更新（無音中は上書きしない）
        if (currentStep == 0 && !mainSilenceActive) {
             currentPattern = random(0, 6);
            memcpy(mainPattern, rhythmPatterns[currentPattern], sizeof(mainPattern));
        }

        // ★ アルペジオ中はメインを鳴らさない
        if (!g_arp.active) {

            // NoteOff
            if (noteIsOnMain && now >= noteOffTimeMain) {
                midi_bridge_send_note_off(lastNoteMain, 0);
                noteIsOnMain = false;
            }

            // ★ パターン × 発音率（density）
            bool shouldPlay =
                (!mainSilenceActive) &&
                (!muteA) &&
                (mainPattern[currentStep] == 1) &&
                (random(0, 100) < mainDensity);

            if (shouldPlay) {

                // 上昇下降の動き
                if (mainGoingDown) {
                    mainDegree--;
                    if (mainDegree <= 0) {
                        mainDegree = 0;
                        mainGoingDown = false;
                    }
                } else {
                    mainDegree++;
                    if (mainDegree >= 8) {
                        mainDegree = 8;
                        mainGoingDown = true;
                    }
                }

                // スケール取得
                const uint8_t* sc;
                int scSize;
                if (scaleMode == 0) { sc = SCALE_HEI;   scSize = SCALE_HEI_SIZE; }
                else if (scaleMode == 1) { sc = SCALE_MIYA;  scSize = SCALE_MIYA_SIZE; }
                else { sc = SCALE_INSEN; scSize = SCALE_INSEN_SIZE; }

                int idx = mainDegree % scSize;
                uint8_t noteA = 60 + transpose + sc[idx];

                int velA = map(stepBPM, 30, 140, 70, 120) + random(-10, 10);
                midi_bridge_send_note_on(noteA, velA, 0);

                lastNoteMain = noteA;
                noteIsOnMain = true;
                noteOffTimeMain = now + (interval * 0.85);

            } else {
                // 鳴らさないときは noteOff
                if (noteIsOnMain) {
                    midi_bridge_send_note_off(lastNoteMain, 0);
                    noteIsOnMain = false;
                }
            }
        }


        // =================================================
        // B パート（無音中は鳴らさない）
        // =================================================
        if (!mainSilenceActive) {
            if (!muteB && rhythmBPatterns[currentBPattern][currentStep] == 1) {
                uint8_t noteB = generateNoteB();
                if (noteIsOnB) midi_bridge_send_note_off(lastNoteB, 1);
                midi_bridge_send_note_on(noteB, 90, 1);
                lastNoteB = noteB;
                noteIsOnB = true;
                noteOffTimeB = now + (interval * 1.8);
            }
        }

    }

    // =====================================================
    // アルペジオ更新
    // =====================================================
    arp_update(now);
}
