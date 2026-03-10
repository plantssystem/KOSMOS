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

// ==== Waveshare Pico-LCD-1.3 ピン定義 ====
#define LCD_DC   8
#define LCD_CS   9
#define LCD_RST 12
#define LCD_BL  13
#define LCD_SCK 10
#define LCD_MOSI 11
#define LCD_SPI SPI1

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

bool patternA[16];
bool patternB[16];

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

bool pattern[16];
int prob[16];

int currentStep = 0;
int selectedStep = 0;

// =====================================================
//  Euclid パターン生成
// =====================================================
void makeEuclid(int steps, int hits, int rot, bool *pattern) {
  for (int i = 0; i < steps; i++) pattern[i] = false;

  int bucket = 0;
  for (int i = 0; i < steps; i++) {
    bucket += hits;
    if (bucket >= steps) {
      bucket -= steps;
      int idx = (i + rot) % steps;
      pattern[idx] = true;
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

// ランダムモード
// 0 = Euclidランダム
// 1 = ステップランダム
// 2 = 両方ランダム
int randomMode = 0;

unsigned long swPressStart = 0;

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

// ---- 音名テーブル（シャープ表記）----
const char* noteNames[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// ---- 音名を返す関数 ----
const char* getNoteName(uint8_t note) {
    int idx = note % 12;
    return noteNames[idx];
}

void executeRandom() {

  if (randomMode == 0 || randomMode == 2) {
    hits = random(1, steps + 1);
    rotation = random(0, steps);
    makeEuclid(steps, hits, rotation, pattern);
  }

  if (randomMode == 1 || randomMode == 2) {
    for (int i = 0; i < 16; i++) {
      pattern[i] = random(0, 2);
    }
  }
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
}

void sendNoteOff(uint8_t note) {
    uint8_t msg[3] = {0x80, note, 0};
    usb_midi.write(msg, 3);
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

// ---- Track A ----
uint8_t lastNoteA = 0;
bool noteIsOnA = false;
unsigned long noteOffTimeA = 0;

// ---- Track B ----
// ---- テンポ（内部 BPM のみ）----
int baseBPM  = 120;   // 基本 BPM
int stepBPM  = 120;   // ステップ進行用 BPM

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

    bool goingUp = (noteA > lastNoteA);

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

    return arp;
}

int downBias = 50;
int upBias = 50;

uint8_t generateNoteA(bool &reachedPeakOut, int &degreeOut) {

    // 陰音階（in-scale）
    const int inScale[] = {0, 1, 5, 7, 8};
    const int scaleSize = 5;

    // フレーズ方向（上昇 or 下降）
    static int phraseDir = 1;

    // ★ 息継ぎゼロ：フレーズは永遠に続く
    // ★ 方向転換の確率を 2% → 1% に弱体化（下降が激減）
    if (rand() % 100 < 1) {
        phraseDir = -phraseDir;
    }

    // 現在のスケール度数を推定
    int baseNote = lastNoteA - transpose;
    int degree = 0;
    int minDiff = 999;

    for (int i = 0; i < scaleSize; i++) {
        int diff = abs((baseNote % 12) - inScale[i]);
        if (diff < minDiff) {
            minDiff = diff;
            degree = i;
        }
    }

    // フレーズ方向に沿って動く
    int move = phraseDir;

    // ★ 歌のアクセント（20%）の下降成分を弱体化
    //    → 上昇アクセントはそのまま、下降アクセントは 1/5 に弱体化
    if (rand() % 100 < 20) {
        int accent = (rand() % 2 == 0) ? 1 : -1;

        if (accent < 0) {
            // 下降アクセントは 20% → 4% に弱体化
            if (rand() % 100 < 20) {
                move -= 1;
            }
        } else {
            move += 1;
        }
    }

    // ★ 下降方向そのものを弱体化（phraseDir が -1 のとき）
    if (phraseDir == -1) {
        // 下降方向の動きは 20% の確率でしか採用しない
        if (rand() % 100 >= 20) {
            move = 0;  // 下降をキャンセル
        }
    }

    degree += move;
    degree = constrain(degree, 0, scaleSize - 1);

    // ★ 山 or 谷の判定
    bool reachedPeak   = (phraseDir == 1 && degree == scaleSize - 1);
    bool reachedValley = (phraseDir == -1 && degree == 0);

    reachedPeakOut = reachedPeak;
    degreeOut      = degree;

    // ★ 山・谷でオクターブ跨ぎ（自然な跳ね）
    int octaveShift = 0;
    if (reachedPeak  && rand() % 100 < 40) octaveShift = 12;
    if (reachedValley && rand() % 100 < 40) octaveShift = -12;

    // 実際の音に変換
    int octave = 48;
    int newNote = octave + inScale[degree] + octaveShift;

    // 同じ音が続かないように
    if (newNote == lastNoteA) {
        degree = (degree + 1) % scaleSize;
        newNote = octave + inScale[degree];
    }

    // 移調
    newNote += transpose;

    // 音域制限
    newNote = constrain(newNote, 48, 96);

    lastNoteA = newNote;
    return newNote;
}

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
const unsigned long modeInterval = 8000;

// =====================================================
// ★ モード切替インターバル
// =====================================================
void updateRandomMode() {
    unsigned long now = millis();
    if (now - lastModeChange < modeInterval) return;

    randomMode = (randomMode + 1) % 3;
    drawRandomMode();
    executeRandom();

    lastModeChange = now;
}

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

void setup() {
  Serial.begin(115200);
  delay(200);
  
  usb_midi.begin();  
  
  sendStart();
  
  lcdInit();
  
  // ★ ジョイスティック SW
  pinMode(JOY_SW_PIN, INPUT_PULLUP);

  // ★ 実際に存在する4ボタンだけ
  pinMode(21, INPUT_PULLUP);  // LEFT
  pinMode(19, INPUT_PULLUP);  // DOWN
  pinMode(17, INPUT_PULLUP);  // UP
  pinMode(15, INPUT_PULLUP);  // RIGHT

  // ★ ジョイスティック X/Y
  pinMode(JOY_X_PIN, INPUT);
  pinMode(JOY_Y_PIN, INPUT);

  lcdFill(COLOR_BLACK);

  for (int i = 0; i < 16; i++) prob[i] = 100;

  makeEuclid(steps, hits, rotation, pattern);

  drawUI();
}

void loop() {
    readJoystick();
    readButtons();

    // ★ USB-MIDI CC 受信（Adafruit_USBD_MIDI 正式版）
    int b;
    while ((b = usb_midi.read()) != -1) {

      static uint8_t status = 0;
      static uint8_t data1  = 0;
      static bool waitingForData1 = false;
      static bool waitingForData2 = false;

      uint8_t byteIn = (uint8_t)b;

      // ステータスバイト（0x80〜0xEF）
      if (byteIn & 0x80) {
        status = byteIn;
        waitingForData1 = true;
        waitingForData2 = false;
        continue;
      }

      // データ1
      if (waitingForData1) {
        data1 = byteIn;
        waitingForData1 = false;
        waitingForData2 = true;
        continue;
      }

      // データ2 → ここで1メッセージ完成
      if (waitingForData2) {
        uint8_t data2 = byteIn;
        waitingForData2 = false;

        // ★ CC メッセージ（0xB0〜0xBF）
        if ((status & 0xF0) == 0xB0) {
            handleCC(data1, data2);
        }
      }
    }

    // =====================================================
    // ★ 休止フラグを立てる（まだ休止しない）
    // =====================================================
    if (!inRest && millis() - lastRest >= nextRestInterval) {
        lastRest = millis();
        inRest = true;
        restStepsRemaining = 7;  // 7ステップ休止
        nextRestInterval = 14000 + rand() % 10000; // 14〜24秒
    }

    // =====================================================
    // ★ キーチェンジ
    // =====================================================
    if (millis() - lastKeyChange >= nextKeyChangeInterval) {
        lastKeyChange = millis();
        transpose = 2 + rand() % 3;
        drawTopText();
        nextKeyChangeInterval = 18000 + rand() % 14000;
    }

    updateProbability();
    bool euclidChanged = updateEuclidEdit();  // ← さっきの bool 版

    static unsigned long lastStep = 0;

    // =====================================================
    // ★ BPM大揺らぎ（B）
    // =====================================================
    if (currentStep == 0) {
        int jump = (rand() % 100 < 70)
            ? random(20, 90)
            : -random(10, 40);

        baseBPM = constrain(baseBPM + jump, 40, 260);

        microBpmNudge();  // B：大揺らぎ直後に微細揺らぎ
    }

    stepBPM = constrain(baseBPM + random(-10, 11), 40, 260);

    // ★ 8分音符の基準
    unsigned long interval = 60000UL / stepBPM / 4;

    // =====================================================
    // ★ ゴースト弱め（8分中心）
    // =====================================================
    int ghostRoll = rand() % 100;
    if (ghostRoll < 1) interval = interval * 3 / 4;
    else if (ghostRoll < 4) interval = interval * 4 / 5;
    else if (ghostRoll < 9) interval = interval * 5 / 6;

    // =====================================================
    // ★ NoteOff
    // =====================================================
    if (noteIsOnA && millis() >= noteOffTimeA) {
        sendNoteOff(lastNoteA);
        noteIsOnA = false;
    }

    // =====================================================
    // ★ ステップ進行（8分音符）※ isPlaying でガード
    // =====================================================
    if (isPlaying && millis() - lastStep >= interval) {
        lastStep = millis();

        // =====================================================
        // ★ E：Euclid パターン切替で微細揺らぎ
        // =====================================================
        if (euclidChanged) {
            microBpmNudge();
        }

        // =====================================================
        // ★ 休止ステップ（7ステップ連続）
        // =====================================================
        if (inRest) {

            if (noteIsOnA) sendNoteOff(lastNoteA);
            noteIsOnA = false;

            restStepsRemaining--;

            if (restStepsRemaining <= 0) {
                inRest = false;  // 7ステップ終わったら解除
            }

            currentStep = (currentStep + 1) % 16;
            drawStepBars();
            drawStepDots();
            return;  // このステップは完全に無音
        }

        // =====================================================
        // ★ メインノートは毎ステップ鳴らす（8分音符）
        // =====================================================
        if (noteIsOnA) sendNoteOff(lastNoteA);

        bool reachedPeak = false;
        int degree = 0;

        uint8_t noteA = generateNoteA(reachedPeak, degree);

        bool goingUpPhrase = (noteA > lastNoteA);

        int velA = 90 + random(-20, 20);
        sendNoteOn(noteA, velA);
        noteIsOnA = true;

        drawTopText();

        float sustainFactor = 1.25;
        if (goingUpPhrase) sustainFactor = 1.30;
        if (!goingUpPhrase) sustainFactor = 1.15;
        if (degree == 0) sustainFactor = 1.35;

        // sustainBias を使うならここで掛ける
        // sustainFactor *= (sustainBias / 100.0f);

        noteOffTimeA = millis() + (unsigned long)(interval * sustainFactor);

        lastNoteA = noteA;
        pushNoteDot(noteA);

        // =====================================================
        // ★ S：フレーズ意志切替（reachedPeak）で微細揺らぎ
        // =====================================================
        if (reachedPeak) {
            updateRandomMode();
            microBpmNudge();  // S 揺らぎ
        }

        // =====================================================
        // ★ Euclid はアルペジオ専用
        // =====================================================
        bool euclidHit = pattern[currentStep];

        int arpCount = 0;

        if (euclidHit) {
            if (reachedPeak) {
                arpCount = 3 + rand() % 3;
            }
            else if (!goingUpPhrase) {
                arpCount = (rand() % 100 < 97) ? 0 : 1;
            }
            else if (degree == 0) {
                arpCount = rand() % 2;
            }
            else {
                arpCount = 1 + rand() % 2;
            }
        }

        unsigned long arpInterval = interval / 2;

        for (int i = 0; i < arpCount; i++) {
            delay(arpInterval);

            uint8_t arp = generateArpMusical(noteA, reachedPeak, lastNoteA, degree);

            int velArp = 75 + random(-15, 20);

            sendNoteOn(arp, velArp);
            delay(arpInterval);
            sendNoteOff(arp);
        }

        currentStep = (currentStep + 1) % 16;

        drawStepBars();
        drawStepDots();
    }

    drawNoteDots();
    
    if (millis() - ccDisplayTimer > 1000) {
      // 下部エリアを消す
      lcdFillRect(0, 220, 240, 20, 0x0000);
    }

}
