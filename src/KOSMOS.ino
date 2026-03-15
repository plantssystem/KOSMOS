/* KOSMOS + PRA32-U (All-in-One: Pico Audio Pack version) */
#include <Arduino.h>

// Optional: give Core1 8KB stack if needed
bool core1_separate_stack = true;

// ----------------- Internal MIDI Bridge (SPSC) -----------------

enum MidiEvType : uint8_t { EV_NOTE_ON=0, EV_NOTE_OFF=1, EV_CC=2 };
struct MidiEvent { uint8_t type, d1, d2, ch; };
namespace MidiQ {
  constexpr size_t QSIZE=64; static volatile uint32_t head=0, tail=0; static MidiEvent q[QSIZE];
  inline bool push(const MidiEvent& ev){uint32_t h=head,n=(h+1)%QSIZE; if(n==tail) return false; q[h]=ev; head=n; return true;}
  inline bool pop(MidiEvent& out){uint32_t t=tail; if(t==head) return false; out=q[t]; tail=(t+1)%QSIZE; return true;}
}
inline void midi_bridge_send_note_on(uint8_t note,uint8_t vel,uint8_t ch=0){MidiEvent ev{EV_NOTE_ON,note,vel,ch};MidiQ::push(ev);}  
inline void midi_bridge_send_note_off(uint8_t note,uint8_t ch=0){MidiEvent ev{EV_NOTE_OFF,note,0,ch};MidiQ::push(ev);}  
inline void midi_bridge_send_cc(uint8_t cc,uint8_t val,uint8_t ch=0){MidiEvent ev{EV_CC,cc,val,ch};MidiQ::push(ev);}  

// ----------------------- PRA32-U on Core1 (I2S 9/10/11) ----------------------
#include <I2S.h>
#define PRA32_U_VERSION "v3.3.0 "
#define PRA32_U_MIDI_CH (0)
#define PRA32_U_I2S_DAC_MUTE_OFF_PIN (22)
#define PRA32_U_I2S_DATA_PIN  (9)
#define PRA32_U_I2S_BCLK_PIN  (10)  // LRCLK = 11（= BCLK+1）
#define PRA32_U_I2S_SWAP_BCLK_AND_LRCLK_PINS (false)
#define PRA32_U_I2S_SWAP_LEFT_AND_RIGHT     (false)
#define PRA32_U_I2S_BUFFERS      (4)
#define PRA32_U_I2S_BUFFER_WORDS (64)

// Define g_midi_ch BEFORE including the synth headers
uint8_t g_midi_ch = PRA32_U_MIDI_CH;
#include "pra32-u-common.h"
#include "pra32-u-synth.h"

PRA32_U_Synth g_synth; I2S g_i2s_output(OUTPUT);

void __not_in_flash_func(setup1)(){
  g_i2s_output.setSysClk(SAMPLING_RATE);
  g_i2s_output.setFrequency(SAMPLING_RATE);
  g_i2s_output.setDATA(PRA32_U_I2S_DATA_PIN);
  g_i2s_output.setBCLK(PRA32_U_I2S_BCLK_PIN); // LRCLK implicit = 11
  if (PRA32_U_I2S_SWAP_BCLK_AND_LRCLK_PINS) g_i2s_output.swapClocks();
  g_i2s_output.setBitsPerSample(16);
  g_i2s_output.setBuffers(PRA32_U_I2S_BUFFERS, PRA32_U_I2S_BUFFER_WORDS);
  g_i2s_output.begin();
  pinMode(PRA32_U_I2S_DAC_MUTE_OFF_PIN, OUTPUT);
  digitalWrite(PRA32_U_I2S_DAC_MUTE_OFF_PIN, HIGH); // unmute
  g_synth.initialize();
}

void __not_in_flash_func(loop1)(){
  MidiEvent ev; while(MidiQ::pop(ev)){
    if(ev.type==EV_NOTE_ON)  g_synth.note_on(ev.d1, ev.d2);
    else if(ev.type==EV_NOTE_OFF) g_synth.note_off(ev.d1);
    else {}
  }
  int16_t L[PRA32_U_I2S_BUFFER_WORDS], R[PRA32_U_I2S_BUFFER_WORDS];
  for(uint32_t i=0;i<PRA32_U_I2S_BUFFER_WORDS;i++) L[i]=g_synth.process(R[i]);
  for(uint32_t i=0;i<PRA32_U_I2S_BUFFER_WORDS;i++) g_i2s_output.write16(L[i], R[i]);
}


// ----------------------- Core0: KOSMOS (patched) -----------------------
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_TinyUSB.h>

Adafruit_USBD_MIDI usb_midi;

#define DISABLE_SILENCE 1

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

// ==== Waveshare Pico-LCD-1.3 ピン定義 ====
/*
#define LCD_DC   8
#define LCD_CS   9
#define LCD_RST 12
#define LCD_BL  13
#define LCD_SCK 10
#define LCD_MOSI 11
#define LCD_SPI SPI1
*/
// ==== Waveshare Pico-LCD-1.3 ピン定義（SPI0版・衝突ゼロ） ====
#define LCD_DC    8
#define LCD_CS    5
#define LCD_RST  12
#define LCD_BL   13

#define LCD_SCK  18
#define LCD_MOSI 19
#define LCD_SPI  SPI   // SPI0 を使用



#define KEY_A_PIN    15
#define KEY_B_PIN    17
#define KEY_X_PIN    19
#define KEY_Y_PIN    21

#define LEFT_PIN   21
#define DOWN_PIN   19
#define UP_PIN     17
#define RIGHT_PIN  15

#define JOY_X_PIN 27   // 左右
#define JOY_Y_PIN 26   // 上下
#define JOY_SW_PIN 3  // 押し込み

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

int transpose = 3;   // -24〜+24 くらいまで対応（2オクターブ）

bool btnA = false;
bool btnB = false;
bool btnX = false;
bool btnY = false;

bool btnLeft = false;
bool btnRight = false;
bool btnUp = false;
bool btnDown = false;

bool btnSW = false;

unsigned long autoModeTimer = 0;

unsigned long noteOffTime = 0;   // ノートを止める時刻
int noteLengthMin = 50;          // 最短音長（ミリ秒）
int noteLengthMax = 400;         // 最長音長（ミリ秒）

int noteDots[240];   // x=0〜239 にノートの高さを保存

// アルペジオ状態管理
// ★ アルペジオイベント用グローバル
unsigned long nextArpEventTime = 0;
bool arpEventActive = false;
unsigned long nextArpStepTime = 0;

int arpEventCount = 0;
int arpRepeatCount = 0;
int arpRepeatTarget = 0;
bool arpGoingUp = true;
int arpFlipWidth = 3;
unsigned long lastArpActivity = 0;


int noteToY(uint8_t note) {
    return map(note, 36, 84, 240, 150);  // 下が低音、上が高音
}

void readButtons() {
  btnLeft  = (digitalRead(LEFT_PIN)  == LOW);
  btnRight = (digitalRead(RIGHT_PIN) == LOW);
  btnUp    = (digitalRead(UP_PIN)    == LOW);
  btnDown  = (digitalRead(DOWN_PIN)  == LOW);
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
  long sumX = 0;
  long sumY = 0;

  for (int i = 0; i < 50; i++) {
    sumX += analogRead(27);
    sumY += analogRead(26);
    delay(5);
  }

  centerX = sumX / 50;
  centerY = sumY / 50;

  // バッファ初期化
  for (int i = 0; i < FILTER_N; i++) {
    bufX[i] = centerX;
    bufY[i] = centerY;
  }
}

int joyX = 0;
int joyY = 0;

void readJoystick() {
  joyX = analogRead(JOY_X_PIN);
  joyY = analogRead(JOY_Y_PIN);

  btnSW = (digitalRead(JOY_SW_PIN) == LOW);
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
  //lcdFill(COLOR_BLACK);

  // 上部テキスト
  char buf[32];

  sprintf(buf, "Euclid  S:%d H:%d R:%d", steps, hits, rotation);
  lcdPrint(5, 5, buf, COLOR_WHITE, COLOR_BLACK, 1);

  sprintf(buf, "Prob Step:%d %d%%", selectedStep, prob[selectedStep]);
  lcdPrint(5, 25, buf, COLOR_WHITE, COLOR_BLACK, 1);

  drawProbabilityBars();
  drawStepBars();
  drawStepDots();
}

void drawProbabilityBars() {
  int x0 = 5;
  int y0 = 60;
  int barWidth = 12;
  int maxHeight = 40;
  int gap = 3;

  // 背景を消す
  lcdFillRect(0, 60, 240, 50, COLOR_BLACK);

  for (int i = 0; i < 16; i++) {
    int x = x0 + i * (barWidth + gap);
    int h = map(prob[i], 0, 100, 0, maxHeight);

    lcdFillRect(x, y0 + (maxHeight - h), barWidth, h, COLOR_CYAN);

    if (i == selectedStep) {
      lcdDrawPixel(x - 1, y0 - 1, COLOR_YELLOW);
      lcdDrawPixel(x + barWidth, y0 - 1, COLOR_YELLOW);
      lcdDrawPixel(x - 1, y0 + maxHeight, COLOR_YELLOW);
      lcdDrawPixel(x + barWidth, y0 + maxHeight, COLOR_YELLOW);
    }
  }
}

void drawStepBars() {
  lcdFillRect(0, 120, 240, 30, COLOR_BLACK);

  int x0 = 5;
  int y0 = 120;
  int barWidth = 12;
  int barHeight = 20;
  int gap = 3;

  for (int i = 0; i < 16; i++) {
    int x = x0 + i * (barWidth + gap);

    uint16_t color =
      (i == currentStep) ? COLOR_RED :
      (pattern[i] ? COLOR_WHITE : COLOR_DARK_GRAY);

    lcdFillRect(x, y0, barWidth, barHeight, color);
  }
}

void drawStepDots() {
  lcdFillRect(0, 160, 240, 20, COLOR_BLACK);

  int x0 = 5;
  int y0 = 160;
  int gap = 14;

  for (int i = 0; i < 16; i++) {
    uint16_t col = (i == currentStep) ? COLOR_RED : COLOR_DARK_GRAY;
    lcdPrint(x0 + i * gap, y0, (i == currentStep ? "●" : "・"), col, COLOR_BLACK, 1);
  }
}

void updateProbability() {

  static bool prevLeft=false, prevRight=false, prevUp=false, prevDown=false, prevA=false;

  if (!prevLeft && btnLeft) {
    selectedStep = (selectedStep + 15) % 16;
    drawTopText();
    drawProbabilityBars();
  }
  if (!prevRight && btnRight) {
    selectedStep = (selectedStep + 1) % 16;
    drawTopText();
    drawProbabilityBars();
  }

  if (!prevUp && btnUp) {
    prob[selectedStep] = min(100, prob[selectedStep] + 5);
    drawTopText();
    drawProbabilityBars();
  }
  if (!prevDown && btnDown) {
    prob[selectedStep] = max(0, prob[selectedStep] - 5);
    drawTopText();
    drawProbabilityBars();
  }

  if (!prevA && btnA) {
    prob[selectedStep] = (prob[selectedStep] == 0 ? 100 : 0);
    drawTopText();
    drawProbabilityBars();
  }

  prevLeft  = btnLeft;
  prevRight = btnRight;
  prevUp    = btnUp;
  prevDown  = btnDown;
  prevA     = btnA;
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
/*
    drawTopText();
    drawProbabilityBars();
    drawStepBars();
    drawStepDots();
    drawRandomMode();
*/
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
    // 画面右上に表示（Key の1行下）
    const int x = 180;   // 右寄せ位置（あなたの画面幅に合わせて調整可）
    const int y = 12;    // ← Key が y=0 なので、1行下へ

    // 背景を黒で塗りつぶして上書き（重なり防止）
    lcdFillRect(x, y, 60, 16, COLOR_BLACK);

    // モード文字
    const char* label = "";
    if (randomMode == 0) label = "E";   // Euclid
    if (randomMode == 1) label = "B";   // Both
    if (randomMode == 2) label = "S";   // Step

    // 白文字・黒背景・フォントサイズ2
    lcdPrint(x, y, label, COLOR_WHITE, COLOR_BLACK, 2);
}

void drawGenerativeBackground() {
    if (randomMode == 0) {
        // Euclid → 幾何学点
        for (int i = 0; i < 128; i += 8) {
            int x = i;
            int y = (i * 3) % 128;
            lcdDrawPixel(x, y, COLOR_WHITE);
        }
    }
    else if (randomMode == 1) {
        // Steps → ノイズ
        for (int i = 0; i < 150; i++) {
            int x = rand() % 128;
            int y = rand() % 128;
            lcdDrawPixel(x, y, COLOR_WHITE);
        }
    }
    else {
        // Both → パルス状の塗りつぶし矩形
        for (int i = 0; i < 128; i += 10) {
            int x = i;
            int y = i;
            int w = 128 - i * 2;
            int h = 128 - i * 2;
            if (w <= 0 || h <= 0) break;
            lcdFillRect(x, y, w, h, COLOR_WHITE);
        }
    }
}

void drawNoteDots() {
    // 下半分をクリア
    lcdFillRect(0, 150, 240, 90, COLOR_BLACK);

    for (int x = 0; x < 240; x++) {
        if (noteDots[x] >= 0) {
            int y = noteToY(noteDots[x]);
            lcdDrawPixel(x, y, COLOR_WHITE);
        }
    }
}

void drawStepPulse() {
    int x = currentStep * 8;
    lcdFillRect(x, 120, 8, 8, COLOR_WHITE);
}

void flashScreen() {
    lcdFill(COLOR_WHITE);
    delay(20);
    lcdFill(COLOR_BLACK);
}

void drawLFO() {
    static int phase = 0;
    phase = (phase + 1) % 200;  // ゆっくりループ

    // 0〜199 → 0〜1 の2段階
    bool on = (phase < 100);

    uint16_t col = on ? COLOR_BLACK : COLOR_WHITE;

    lcdFillRect(0, 0, 128, 128, col);
}

void sendNoteOn(uint8_t note, uint8_t velocity) {
  uint8_t msg[3] = {0x90, note, velocity};
  usb_midi.write(msg, 3);
  // ★ Core1 (PRA32-U) へ内部MIDIブリッジ
  midi_bridge_send_note_on(note, velocity);
}

void sendNoteOff(uint8_t note) {
  uint8_t msg[3] = {0x80, note, 0};
  usb_midi.write(msg, 3);
  // ★ Core1 (PRA32-U) へ内部MIDIブリッジ
  midi_bridge_send_note_off(note);
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

void sendClock() {
    uint8_t msg = 0xF8;
    usb_midi.write(&msg, 1);
}

void sendStart() {
    uint8_t msg = 0xFA;
    usb_midi.write(&msg, 1);
}

void sendStop() {
    uint8_t msg = 0xFC;
    usb_midi.write(&msg, 1);
}

uint8_t lastNote = 0;
bool noteIsOn = false;
unsigned long clockInterval = 0;

void initNoteDots() {
  for (int i = 0; i < 240; i++) noteDots[i] = -1;
}

void pushNoteDot(uint8_t note) {
  for (int i = 0; i < 239; i++) {
    noteDots[i] = noteDots[i+1];
  }
  noteDots[239] = note;
}

// ---- Trill globals ----
bool trillActive      = false;
int  trillIndex       = 0;
int  trillDir         = 1;
int  trillCycleCount  = 0;
int  trillCount       = 0;
uint8_t trillBaseNote = 0;
unsigned long trillNextTime    = 0;
bool trillNoteOn      = false;
uint8_t trillLastNote = 0;
unsigned long trillNoteOffTime = 0;

// ---- Track A ----
uint8_t lastNoteA = 0;
bool noteIsOnA = false;
unsigned long noteOffTimeA = 0;

// ---- Track B ----
// ---- テンポ（内部 BPM のみ）----
int baseBPM  = 84;   // 基本 BPM
int stepBPM  = 84;   // ステップ進行用 BPM

void drawTopText() {
  char buf[32];

  // 背景クリア
  lcdFillRect(0, 0, 240, 40, COLOR_BLACK);

  // Euclid 情報
  sprintf(buf, "Euclid  S:%d H:%d R:%d", steps, hits, rotation);
  lcdPrint(5, 5, buf, COLOR_WHITE, COLOR_BLACK, 1);

  // Key 表示
  char tbuf[16];
  sprintf(tbuf, "Key:%+d", transpose);
  lcdPrint(130, 5, tbuf, COLOR_YELLOW, COLOR_BLACK, 1);

  // ★ 音名表示（右端）
  char nbuf[16];
  sprintf(nbuf, "Note:%s", getNoteName(lastNoteA));
  lcdPrint(180, 5, nbuf, COLOR_CYAN, COLOR_BLACK, 1);

  // Probability 情報
  sprintf(buf, "Prob Step:%d %d%%", selectedStep, prob[selectedStep]);
  lcdPrint(5, 25, buf, COLOR_WHITE, COLOR_BLACK, 1);
}

int generateArpInterval(bool goingUp) {
    if (goingUp) {
        int choices[] = { +2, +4, +7 }; // 2度, 4度, 5度
        return choices[rand() % 3];
    } else {
        int choices[] = { -2, -4, -7 };
        return choices[rand() % 3];
    }
}

int getArpDuration() {
    int base = 60 + random(-20, 40);  // 40〜100ms の自然な揺れ
    return base;
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

uint8_t generateArpScale(uint8_t base) {
    int idx = findScaleIndex(base);
    int offset[] = { +1, -1, +2 }; // 隣接中心
    int o = offset[rand() % 3];
    return scale[(idx + o + scaleSize) % scaleSize];
}

uint8_t generateArpMusical(uint8_t noteA, bool reachedPeak, uint8_t lastNoteA, int degree) {

    const int inScale[] = {0, 1, 5, 7, 8};
    const int scaleSize = 5;

    bool goingUp = arpGoingUp;   // ← イベントの方向を使う

    if (noteA <= 38) goingUp = true;

    // =====================================================
    // ★ フレーズの“意志”（12〜24音の長い方向性）
    // =====================================================
    static int phraseDir = +1;
    static int phraseCount = 0;

    if (phraseCount <= 0) {
        phraseDir = (rand() % 100 < 60) ? +1 : -1;
        phraseCount = 12 + rand() % 13;   // 12〜24音
    }
    phraseCount--;

    // =====================================================
    // ★ raw を作る（上昇は歌い上げ、下降は儚く）
    // =====================================================
    int raw = noteA;

    if (phraseDir > 0) {
        int choices[] = { +1, +2, +4, +7, +7, +9, +12 };
        raw = noteA + choices[rand() % 7];

        if (rand() % 100 < 40) {
            raw += (rand() % 2 == 0) ? +1 : -1;
        }

    } else {
        int choices[] = { -1, -2, -3 };
        raw = noteA + choices[rand() % 3];
    }

    // =====================================================
    // ★ raw を陰音階に吸着
    // =====================================================
    int pitch = (raw - transpose) % 12;
    if (pitch < 0) pitch += 12;

    int best = inScale[0];
    int bestDiff = abs(pitch - inScale[0]);

    for (int i = 1; i < scaleSize; i++) {
        int diff = abs(pitch - inScale[i]);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = inScale[i];
        }
    }

    int arp = (raw - pitch) + best;

    // =====================================================
    // ★ 平調子の癖 MAX：0 と 5 に強く吸い寄せる
    // =====================================================
    int arpPitch = (arp - transpose + 12) % 12;

    if (arpPitch == 1 || arpPitch == 8) {
        if (rand() % 100 < 70) arp -= 1;
        else arp += 4;
    }

    if (!goingUp) {
        if (arpPitch == 7) arp -= 2;
        if (arpPitch == 5 && rand() % 100 < 50) arp -= 5;
    }

    // =====================================================
    // ★ 上昇の音痴ゼロ補正
    // =====================================================
    if (goingUp) {
        if (arp <= noteA) arp += 2;
        if (arp <= noteA) arp += 3;
    }

    // =====================================================
    // ★ 下降の儚さ（微振動）
    // =====================================================
    if (!goingUp) {
        if (rand() % 2 == 0) {
            arp += (rand() % 3) - 1;
        }
    }

    // =====================================================
    // ★ 移調・音域制限
    // =====================================================
    arp += transpose;
    arp = constrain(arp, 48, 96);

    if (arp == noteA) {
        arp += 1;
        if (arp > 96) arp -= 2;
    }

    // ★ 安全レンジにクランプ（C2〜C7）
    if (arp < 36) arp = 36;   // C2
    if (arp > 96) arp = 96;   // C7

    return arp;
}

// =====================================================
// ★ メインメロディ生成（陰音階＋パターン揺らぎ＋トリルフラグ）
// =====================================================
uint8_t generateNoteA(bool &trillFlagOut, int &degreeOut) {

    const int inScale[] = {0, 1, 5, 7, 8};   // 陰音階
    const int inSize = 5;

    static int phraseDir = 1;   // +1=上行, -1=下降
    static int degree = 2;      // 現在の度数

    static int upCount = 0;
    static int holdCount = 0;
    static int holdTarget = 0;

    trillFlagOut = false;

    // =====================================================
    // ★ pattern の影響（8分同期）
    // =====================================================
    auto applyPattern = [&](int deg) {
        int idx = currentStep;   // ★ 8分のステップに同期
        int p = pattern[idx];

        if (p > 1) p = 1;
        if (p < -1) p = -1;

        deg += p;
        deg = constrain(deg, 0, inSize - 1);
        return deg;
    };

    // =====================================================
    // ★ degree が端に張り付かないように跳ね返す
    // =====================================================
    auto bounce = [&](int &deg, int &dir) {
        if (deg <= 0) {
            deg = 1;
            dir = +1;
        }
        if (deg >= inSize - 1) {
            deg = inSize - 2;
            dir = -1;
        }
    };

    // =====================================================
    // ★ 上行 → 下降切替
    // =====================================================
    if (phraseDir == 1) {
        upCount++;
        if (upCount >= 3 + random(0, 4)) {
            phraseDir = -1;
            upCount = 0;
            holdCount = 0;
            holdTarget = 0;
        }
    }

    // =====================================================
    // ★ 上行フェーズ
    // =====================================================
    if (phraseDir == 1) {

        degree++;
        bounce(degree, phraseDir);
        degree = applyPattern(degree);

        trillFlagOut = (random(0, 100) < 15);

        uint8_t note = 50 + inScale[degree] + transpose;
        lastNoteA = note;
        degreeOut = degree;
        return note;
    }

// =====================================================
// ★ 下降フェーズ（1ステップにつき必ず1回だけ note を返す）
// =====================================================
if (phraseDir == -1) {

    if (holdTarget == 0)
        holdTarget = 2 + random(0, 3);

    // ★ degree の更新だけ行う（音は返さない）
    if (holdCount < holdTarget) {
        holdCount++;

        degree = applyPattern(degree);
        bounce(degree, phraseDir);
    }
    else {
        degree--;
        bounce(degree, phraseDir);
        degree = applyPattern(degree);

        holdCount = 0;
        holdTarget = 2 + random(0, 3);
    }

    // ★ ここで1回だけ note を返す（2連打完全防止）
    uint8_t note = 50 + inScale[degree] + transpose;
    lastNoteA = note;
    degreeOut = degree;
    return note;
}

    return lastNoteA;
}

// =====================================================
// ★ トリルノート生成（陰音階内で上下揺れ）
// =====================================================
uint8_t generateTrillNote(uint8_t baseNote, int stepIndex) {

    float curve;

    if (stepIndex == 0) {
        // ★ 最初の1発は必ず +1.2音（同音禁止）
        curve = 1.2f;
    } else {
        // ★ 2発目以降は滑らかカーブ
        curve = sin(stepIndex * 1.57f) * 1.8f;
    }

    float raw = baseNote + curve;

    // ★ 陰音階に軽く吸着
    const int scale[] = {0, 1, 5, 7, 8};
    const int size = 5;

    int pitch = ((int)raw - transpose) % 12;
    if (pitch < 0) pitch += 12;

    int best = scale[0];
    int bestDiff = abs(pitch - scale[0]);

    for (int i = 1; i < size; i++) {
        int diff = abs(pitch - scale[i]);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = scale[i];
        }
    }

    if (bestDiff <= 2) {
        raw = (raw - pitch) + best;
    }

    return constrain((int)raw, 48, 96);
}

int downBias = 50;
int upBias = 50;

uint8_t lastNoteB = 36;   // C2
bool noteIsOnB = false;
unsigned long noteOffTimeB = 0;

uint8_t generateNoteB() {

    // 主音（C）を中心にする
    int root = 36 + transpose;  // C2 基準

    int note = root;

    // 20% の確率で 5度（G）
    if (rand() % 100 < 20) {
        note = root + 7;
    }

    // 10% の確率でオクターブ上
    if (rand() % 100 < 10) {
        note = root + 12;
    }

    // 音域制限
    note = constrain(note, 30, 60);

    return note;
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
    // 下部エリアをクリア（高さ20px）
    lcdFillRect(0, 220, 240, 20, 0x0000);  // 黒で塗りつぶし

    char buf[32];
    sprintf(buf, "CC %d : %d", cc, value);

    // 白文字で表示
    lcdPrint(4, 222, buf, 0xFFFF, 0x0000, 1);

    ccDisplayTimer = millis();
}

void drawEuclidParams() {
    // 上部エリアをクリア（高さ20px）
    lcdFillRect(0, 0, 240, 20, 0x0000);  // 黒

    char buf[32];
    sprintf(buf, "EUC S:%d H:%d R:%d", steps, hits, rotation);

    // 白文字で表示
    lcdPrint(4, 4, buf, 0xFFFF, 0x0000, 1);
}

bool isPlaying = true;
// ★ 小休止用
unsigned long lastRest = 0;
bool inRest = false;
int restStepsRemaining = 0; // ★ 休止ステップ数

int sustainBias = 125;

void handleCC(byte cc, byte value) {

    // ============================
    // ★ デバッグ表示（必要なら）
    // ============================
    Serial.print("CC=");
    Serial.print(cc);
    Serial.print(" val=");
    Serial.println(value);

    // ============================
    // ★ PLAY / STOP
    // ============================
    if (cc == 41 && value > 0) {
        isPlaying = true;

    }

    if (cc == 42 && value > 0) {
        isPlaying = false;

        // STOP 時に音を止める
        if (noteIsOnA) {
            sendNoteOff(lastNoteA);
            noteIsOnA = false;
        }

    }

    // ============================
    // ★ フェーダー1 → BPM
    // ============================
    if (cc == 0) {
        baseBPM = map(value, 0, 127, 40, 260);
        drawTopText();
    }


    // CC1 = フェーダー2 → Key (transpose)
    if (cc == 1) {
        transpose = map(value, 0, 127, 2, 4);
        drawTopText();
    }

    // ============================
    // ★ ノブ1 → sustainBias
    // ============================
    if (cc == 16) {
        sustainBias = map(value, 0, 127, 80, 140);
    }

    // ============================
    // ★ ノブ2 → 上昇バイアス
    // ============================
    if (cc == 17) {
        upBias = map(value, 0, 127, 0, 100);
    }

    // ============================
    // ★ ノブ3 → 下降バイアス
    // ============================
    if (cc == 18) {
        downBias = map(value, 0, 127, 0, 100);
    }

    // ============================
    // ★ Euclid steps / hits / rotation
    // ============================
    // SOLO → steps
    if (cc == 32 && value > 0) {
        steps = constrain(steps + 1, 1, 16);
        makeEuclid(steps, hits, rotation, pattern);
        drawEuclidParams();
    }

    // MUTE → hits
    if (cc == 33 && value > 0) {
        hits = constrain(hits + 1, 0, steps);
        makeEuclid(steps, hits, rotation, pattern);
        drawEuclidParams();
    }

    // REC → rotation
    if (cc == 34 && value > 0) {
        rotation = (rotation + 1) % steps;
        makeEuclid(steps, hits, rotation, pattern);
        drawEuclidParams();
    }

    // ============================
    // ★ CYCLE → 7ステップ休止強制
    // ============================
    if (cc == 46 && value > 0) {
        inRest = true;
        restStepsRemaining = 7;
        lastRest = millis();
        drawTopText();
    }

    // ============================
    // ★ REW / FF → currentStep 移動
    // ============================
    if (cc == 44 && value > 0) {  // REW
        currentStep = (currentStep + 15) % 16;
        drawStepBars();
        drawStepDots();
    }

    if (cc == 45 && value > 0) {  // FF
        currentStep = (currentStep + 1) % 16;
        drawStepBars();
        drawStepDots();
    }

    // ============================
    // ★ OLED に CC 値表示
    // ============================
    drawCCValue(cc, value);
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

uint8_t generateArpStep(uint8_t baseNote, bool goingUp) {

    int step;

    if (baseNote <= 38) goingUp = true;

    if (goingUp) {
        int upChoices[] = { +2, +4, +5, +7, +9, +12 };
        step = upChoices[rand() % 6];
    } else {
        int downChoices[] = { -1, -2, -3, -5, -7, -12 };
        step = downChoices[rand() % 6];
    }

    // ★ 微妙な揺らぎ
    if (rand() % 100 < 40) {
        step += (rand() % 2 ? +1 : -1);
    }

    int raw = baseNote + step;

    // ★ 平調子に吸着
    int snapped = snapToHeichoshi(raw, transpose);

    // ★ 安全レンジ
    if (snapped < 36) snapped = 36;
    if (snapped > 96) snapped = 96;

    // ★ baseNote と同じなら強制的にずらす
    if (snapped == baseNote) {
        snapped += (goingUp ? +2 : -2);
    }

    return (uint8_t)snapped;
}


unsigned long silenceLength = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
 
  usb_midi.begin();  
  
  //lcdInit();
  
  //lcdFill(COLOR_BLACK);

  for (int i = 0; i < 16; i++) prob[i] = 100;

  //drawUI();

  executeRandom();
  nextArpEventTime = millis() + random(10000, 20000);  // 10〜20秒
  transpose += 5;
  baseBPM = max(20, baseBPM - 4);   // ★ 基準テンポを4下げる

  // ★ 起動直後にアルペジオを強制スタート
  lastNoteA = 52 + transpose;   // E3 を基準に（あなたの希望）

  // ★ 起動時アルペジオを長くする
  arpEventActive = true;
  arpEventCount = 0;
  arpRepeatCount = 0;

  // ★ 起動時だけ長く（10〜20回の往復）
  arpRepeatTarget = random(10, 21);

  // ★ 1方向の音数も増やすための初期化
  arpGoingUp = (rand() % 100 < 60);

  // ★ すぐに開始
  nextArpStepTime = millis();
}

int phraseDir = 1;   // +1 = 上行, -1 = 下降

// 平調子（主音＝D）
const int heiScale[] = {2, 4, 7, 9, 11};
const int heiSize = 5;

// =====================================================
// ★ 平調子：滑らか上行（トリル用）
// =====================================================
uint8_t generateSmoothRise(uint8_t base, int stepIndex) {

    // 滑らか上昇カーブ（0.4〜2.4音）
    float curve = 0.4f + stepIndex * 0.35f;
    int raw = base + (int)curve;

    // 平調子に軽く吸着
    int pitch = raw % 12;
    int best = heiScale[0];
    int bestDiff = abs(pitch - heiScale[0]);

    for (int i = 1; i < heiSize; i++) {
        int diff = abs(pitch - heiScale[i]);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = heiScale[i];
        }
    }

    if (bestDiff <= 2) {
        raw = (raw - pitch) + best;
    }

    return constrain(raw, 48, 96);
}

// =====================================================
// ★ 平調子：滑らか下降（メインライン用）
// =====================================================
uint8_t generateSmoothFall(uint8_t base, int stepIndex) {

    // 滑らか下降カーブ（-0.4〜-2.4音）
    float curve = -0.4f - stepIndex * 0.35f;
    int raw = base + (int)curve;

    // 平調子に軽く吸着
    int pitch = raw % 12;
    int best = heiScale[0];
    int bestDiff = abs(pitch - heiScale[0]);

    for (int i = 1; i < heiSize; i++) {
        int diff = abs(pitch - heiScale[i]);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = heiScale[i];
        }
    }

    if (bestDiff <= 2) {
        raw = (raw - pitch) + best;
    }

    return constrain(raw, 48, 96);
}

void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel = 0) {
    midi_bridge_send_cc(cc, value, channel);
}

void sendAllNotesOff() {
    for (int ch = 0; ch < 16; ch++) {
        sendControlChange(123, 0, ch);  // CC123 = All Notes Off
    }
}

void loop() {

    unsigned long now = millis();

#if !DISABLE_SILENCE
    static bool silenceActive = false;
    static unsigned long silenceEndTime = 0;
    static unsigned long nextSilenceTime = millis() + random(25000, 30000);
#endif

#if !DISABLE_SILENCE
    // ★ 20秒安全装置：無音中は動かない、アルペジオが20秒鳴いていない時だけ
    if (!silenceActive && !arpEventActive && (now - lastArpActivity > 20000)) {

        // 無音も一旦キャンセル
        silenceActive   = false;
        silenceEndTime  = 0;

        // ★ 次の無音は必ず30秒以上未来に飛ばす（最重要）
        nextSilenceTime = now + 30000;

        // ★ キーもリセット（または上方向に寄せる）
        transpose = 3;   // もしくは +12 など
    
        // アルペジオ完全リセット
        arpEventActive  = true;
        arpEventCount   = 0;
        arpRepeatCount  = 0;
        arpRepeatTarget = random(4, 7);
        arpGoingUp      = (rand() % 100 < 60);
        arpFlipWidth    = random(2, 5);

        nextArpStepTime  = now;
        nextArpEventTime = now + random(5000, 15000);
        lastArpActivity = now;
    }
#endif
    
    readButtons();
    readJoystick();

#if !DISABLE_SILENCE
    // ★ 無音処理（ミュート専用、安全版）
    if (!silenceActive && now >= nextSilenceTime) {

        silenceActive = true;

        // 無音の長さ（1.8〜4.0秒）
        silenceLength  = 1800 + random(0, 2200);
        silenceEndTime = now + silenceLength;

        // 次の無音タイミング（25〜30秒）
        nextSilenceTime = now + random(25000, 30000);

        // ★ 音だけ止める（アルペジオ状態には触らない）
        sendAllNotesOff();
        if (noteIsOnA) { sendNoteOff(lastNoteA); noteIsOnA = false; }
        if (trillNoteOn) { sendNoteOff(trillLastNote); trillNoteOn = false; }
        if (noteIsOnB) { sendNoteOff(lastNoteB); noteIsOnB = false; }
        lastArpActivity = now;
        return;  // 無音開始直後はここで終了
    }
#endif
#if !DISABLE_SILENCE
    if (silenceActive) {

        if (now >= silenceEndTime) {

            // ★ 無音終了：キー変更だけ行う（アルペジオには触らない）
            float factor = (float)(silenceLength - 1800) / (4000 - 1800);
            int upBias = 60 + (int)(factor * 30);

            int r = random(0, 100);
            int delta = (r < upBias) ? +5 : -5;
            transpose += delta;

            if (transpose > 24) transpose = 24;
            if (transpose < -24) transpose = -24;

            drawTopText();

            silenceActive = false;
            lastArpActivity = now;   // ★ 追加
            
            return;  // 無音終了直後もここで抜ける

        } else {
            // ★ 無音中は return しない！
            // 音を出さないだけで、アルペジオの状態は進める
            // つまりここは何もせず下へ流す
        }
    }
#endif

    // ★ 呼吸する BPM（sin 波で ±5 揺らす）
    static unsigned long breathStart = millis();
    static unsigned long breathDuration = 8000 + random(0, 7000);  // 8〜15秒で1呼吸

    float t = (float)(now - breathStart) / (float)breathDuration;  // 0.0〜1.0

    if (t >= 1.0f) {
        breathStart = now;
        breathDuration = 8000 + random(0, 7000);
        t = 0.0f;
    }

    // ★ sin 波（吸う→吐く）
    float wave = sin(t * 3.14159265f * 2.0f);  // -1〜+1

    // ★ ±5 BPM の揺らぎ
    int delta = (int)(wave * 5.0f);

    // ★ 呼吸 BPM
    stepBPM = baseBPM + delta;
    if (stepBPM < 30) stepBPM = 30;

    // ★ 自動トランスポーズ（4度 = 5半音）をランダムで発生
    static unsigned long nextTransposeTime = millis() + random(5000, 15000);

    if (now >= nextTransposeTime) {

        // +5 または -5 をランダムに選ぶ
        int delta = (rand() % 2 == 0) ? +5 : -5;

        transpose += delta;

        // 安全範囲に制限
        if (transpose > 24) transpose = 24;
        if (transpose < -24) transpose = -24;

        // 次の発生タイミング（5〜15秒）
        nextTransposeTime = now + random(5000, 15000);

        // UI更新
        drawTopText();
    }

    // ★ BPM を ±2 揺らす（ゆっくり呼吸するように）
    static unsigned long lastBpmChange = 0;
    if (now - lastBpmChange > 500) {  // 0.5秒ごとにゆっくり揺らす
        lastBpmChange = now;

        int delta = random(-2, 3);  // -3〜+3
        stepBPM = baseBPM + delta;

        // 下限を安全に確保
        if (stepBPM < 30) stepBPM = 30;
    }

    // =====================================================
    // ★ 5〜15秒ランダムでアルペジオイベント（2〜4回の上下）
    // =====================================================

    // ---- イベント開始 ----
    if (!arpEventActive && now >= nextArpEventTime) {
        /*
        arpEventActive = true;

        arpEventCount = 0;
        arpRepeatCount = 0;

        arpRepeatTarget = random(4, 7);   // ★ 2〜4回の往復
        arpGoingUp = (rand() % 100 < 60); // 最初は上昇60%

        // ★ 反転幅をイベント開始時に決める（安定化の核心）
        arpFlipWidth = random(2, 5);      // 2〜4音で反転

        nextArpStepTime = now;

        // ★ 開始時点で活動扱いにする
        lastArpActivity = now;
        */
        arpEventActive  = true;
        arpEventCount   = 0;
        arpRepeatCount  = 0;
        arpRepeatTarget = random(4, 7);
        arpGoingUp      = (rand() % 100 < 60);
        arpFlipWidth    = random(2, 5);
        nextArpStepTime = now;
        lastArpActivity = now;
    }


    // ---- イベント中（32分アルペジオ）----
    if (arpEventActive) {

        unsigned long interval = 60000UL / stepBPM / 4;  // 8分
        unsigned long arpInterval = interval / 4;        // ★ 32分音符

        if (now >= nextArpStepTime) {

            // ★ 上昇 or 下降の1音
            uint8_t note = generateArpStep(lastNoteA, arpGoingUp);
            sendNoteOn(note, 90);
            lastNoteA = note;

            lastArpActivity = now;

            nextArpStepTime = now + arpInterval;
            arpEventCount++;

            // ★ 反転幅は固定（イベント開始時に決めた値）
            if (arpEventCount >= arpFlipWidth) {
                arpEventCount = 0;
                arpGoingUp = !arpGoingUp;   // ★ 方向反転
                arpRepeatCount++;

                // ★ 2〜4回の往復が終わったら終了
                if (arpRepeatCount >= arpRepeatTarget) {
                    arpEventActive = false;

                    // ★ 無音中でも確実に未来に飛ばす
                    unsigned long base = millis();  
                    nextArpEventTime = base + random(3000, 8000);

                    // ★ 安全のため活動扱いにする
                    lastArpActivity = base;
                }
            }
        }
    }

    // =====================================================
    // ★ モード切替（10〜20秒）
    // =====================================================
    static unsigned long lastModeChange = 0;
    static unsigned long modeInterval = 10000 + random(0, 10000);

    if (now - lastModeChange >= modeInterval) {
        lastModeChange = now;
        modeInterval = 10000 + random(0, 10000);

        randomMode = (randomMode + 1) % 3;
        currentStep = -1;
        executeRandom();
    }

    // =====================================================
    // ★ メインステップ（8分）
    // =====================================================
    static unsigned long lastStep = 0;
    unsigned long interval = 60000UL / stepBPM / 4;

    if (now - lastStep >= interval) {
        lastStep = now;

        if (!isPlaying) {
            if (noteIsOnA) { sendNoteOff(lastNoteA); noteIsOnA = false; }
            return;
        }

        currentStep = (currentStep + 1) % steps;
        drawStepBars();
        drawStepDots();

        if (noteIsOnA && now >= noteOffTimeA) {
            sendNoteOff(lastNoteA);
            noteIsOnA = false;
        }

        // ---- メインノート ----
        bool fire = true;

        if (fire && !arpEventActive) {  
            // ★ アルペジオ中はメインノートを鳴らさない（自然な間）
            bool trillFlag = false;
            int dummyDegree = 0;

            uint8_t noteA = generateNoteA(trillFlag, dummyDegree);

            int velA = 90 + random(-20, 20);
            sendNoteOn(noteA, velA);
            noteIsOnA = true;
            noteOffTimeA = now + (interval / 4);

            //lastNoteA = noteA;
            
            pushNoteDot(noteA);
            drawNoteDots();
            drawTopText();
        }
    }
}
