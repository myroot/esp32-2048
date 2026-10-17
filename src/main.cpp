#include "LGFX_ESP32_4848S040.hpp"
#include <Arduino.h>

LGFX_ESP32_4848S040 lcd;
LGFX_Sprite canvas(&lcd); // PSRAM Framebuffer for smooth rendering

// ─── Screen & UI Layout Dimensions ─────────────────────────────
#define SCR_W 480
#define SCR_H 480

#define HEADER_H 75

#define BOARD_SIZE 380
#define BOARD_X ((SCR_W - BOARD_SIZE) / 2) // 50px left/right
#define BOARD_Y 85                         // 85px top offset

#define GRID_SIZE 4
#define CELL_MARGIN 8
#define CELL_SIZE                                                              \
  ((BOARD_SIZE - (GRID_SIZE + 1) * CELL_MARGIN) / GRID_SIZE) // 85px

// ─── Official Classic 2048 Color Palette (Gabriele Cirulli) ────
static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return lcd.color565(r, g, b);
}

// Official 2048 Tile Colors
uint16_t tileColor(int val) {
  switch (val) {
  case 2:
    return rgb(238, 228, 218); // #eee4da
  case 4:
    return rgb(237, 224, 200); // #ede0c8
  case 8:
    return rgb(242, 177, 121); // #f2b179
  case 16:
    return rgb(245, 149, 99); // #f59563
  case 32:
    return rgb(246, 124, 95); // #f67c5f
  case 64:
    return rgb(246, 94, 59); // #f65e3b
  case 128:
    return rgb(237, 207, 114); // #edcf72
  case 256:
    return rgb(237, 204, 97); // #edcc61
  case 512:
    return rgb(237, 200, 80); // #edc850
  case 1024:
    return rgb(237, 197, 63); // #edc53f
  case 2048:
    return rgb(237, 194, 46); // #edc22e
  default:
    return rgb(60, 58, 50); // #3c3a32
  }
}

// Official 2048 Text Colors: Dark Taupe (#776e65) for 2/4, Off-White (#f9f6f2)
// for 8+
uint16_t tileTextColor(int val) {
  return (val <= 4) ? rgb(119, 110, 101) : rgb(249, 246, 242);
}

// ─── Game State ───────────────────────────────────────────────
int board[GRID_SIZE][GRID_SIZE];
int score = 0, best = 0;
bool gameOver = false;
bool won = false;

int newTileR = -1, newTileC = -1; // Position of newly spawned tile

// ─── Touch Swipe Tracking ─────────────────────────────────────
int32_t startX = 0, startY = 0;
int32_t lastX = 0, lastY = 0;
bool wasTouching = false;
bool swipeHandled = false;

// ─── Animation Structures ─────────────────────────────────────
struct AnimTile {
  int fromR, fromC;
  int toR, toC;
  int val;
};
AnimTile animList[16];
int animCount = 0;

// ─── Helpers ─────────────────────────────────────────────────
void getCellPos(int r, int c, int &x, int &y) {
  x = CELL_MARGIN + c * (CELL_SIZE + CELL_MARGIN);
  y = CELL_MARGIN + r * (CELL_SIZE + CELL_MARGIN);
}

void addRandomTile() {
  int ex[16], ey[16], n = 0;
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (!board[r][c]) {
        ex[n] = c;
        ey[n] = r;
        n++;
      }
    }
  }
  if (!n)
    return;
  int i = random(n);
  newTileR = ey[i];
  newTileC = ex[i];
  board[newTileR][newTileC] = (random(10) < 9) ? 2 : 4;
}

void initGame() {
  memset(board, 0, sizeof(board));
  score = 0;
  gameOver = false;
  won = false;
  newTileR = -1;
  newTileC = -1;
  addRandomTile();
  addRandomTile();
}

// ─── Canvas Drawing Functions ─────────────────────────────────
void drawTileOnCanvas(int x, int y, int size, int val) {
  if (!val) {
    canvas.fillRoundRect(x, y, size, size, 6,
                         rgb(205, 193, 180)); // Official empty slot #cdc1b4
    return;
  }
  uint16_t bg = tileColor(val);
  canvas.fillRoundRect(x, y, size, size, 6, bg);

  canvas.setTextColor(tileTextColor(val));
  canvas.setTextDatum(MC_DATUM);
  int cx = x + size / 2;
  int cy = y + size / 2;
  String s = String(val);

  if (val < 100)
    canvas.setTextSize(3);
  else if (val < 1000)
    canvas.setTextSize(3);
  else if (val < 10000)
    canvas.setTextSize(2);
  else
    canvas.setTextSize(2);

  canvas.drawString(s, cx, cy);
}

void drawHeader() {
  // Header area background (Official #faf8ef)
  lcd.fillRect(0, 0, SCR_W, HEADER_H, rgb(250, 248, 239));

  // Game Title "2048" (Official #776e65)
  lcd.setTextColor(rgb(119, 110, 101));
  lcd.setTextSize(4);
  lcd.setTextDatum(TL_DATUM);
  lcd.drawString("2048", 15, 10);

  // NEW GAME Button (Official #8f7a66)
  lcd.fillRoundRect(15, 45, 115, 28, 6, rgb(143, 122, 102));
  lcd.setTextSize(2);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextDatum(MC_DATUM);
  lcd.drawString("NEW GAME", 72, 59);

  // SCORE Box (Official #bbada0)
  lcd.fillRoundRect(240, 8, 105, 60, 8, rgb(187, 173, 160));
  lcd.setTextSize(1);
  lcd.setTextColor(rgb(238, 228, 218));
  lcd.drawString("SCORE", 292, 20);
  lcd.setTextSize(2);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawString(String(score), 292, 44);

  // BEST Box (Official #bbada0)
  lcd.fillRoundRect(360, 8, 105, 60, 8, rgb(187, 173, 160));
  lcd.setTextSize(1);
  lcd.setTextColor(rgb(238, 228, 218));
  lcd.drawString("BEST", 412, 20);
  lcd.setTextSize(2);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawString(String(best), 412, 44);
}

void renderBoardCanvas(float scaleNewTile = 1.0f) {
  // Background board container (Official #bbada0)
  canvas.fillRoundRect(0, 0, BOARD_SIZE, BOARD_SIZE, 10, rgb(187, 173, 160));

  // Draw grid cells
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      int x, y;
      getCellPos(r, c, x, y);
      int v = board[r][c];

      if (r == newTileR && c == newTileC && scaleNewTile < 0.99f && v) {
        // Empty slot background
        canvas.fillRoundRect(x, y, CELL_SIZE, CELL_SIZE, 6, rgb(205, 193, 180));
        // Fast scale pop-in
        int sz = (int)(CELL_SIZE * scaleNewTile);
        int off = (CELL_SIZE - sz) / 2;
        drawTileOnCanvas(x + off, y + off, sz, v);
      } else {
        drawTileOnCanvas(x, y, CELL_SIZE, v);
      }
    }
  }
}

void drawOverlay(const char *line1, const char *line2, uint16_t col1) {
  int ox = 60;
  int oy = 165;
  int ow = 360;
  int oh = 220;
  lcd.fillRoundRect(ox, oy, ow, oh, 16, rgb(30, 30, 30));
  lcd.setTextColor(col1);
  lcd.setTextSize(3);
  lcd.setTextDatum(MC_DATUM);
  lcd.drawString(line1, ox + ow / 2, oy + 70);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  lcd.drawString(line2, ox + ow / 2, oy + 150);
}

void redrawStatic() {
  drawHeader();
  renderBoardCanvas(1.0f);
  canvas.pushSprite(BOARD_X, BOARD_Y);
  if (won)
    drawOverlay("You Win!", "Tap NEW GAME to continue", rgb(255, 215, 0));
  if (gameOver)
    drawOverlay("Game Over", "Tap NEW GAME to retry", rgb(255, 100, 100));
}

// ─── Physical cell rotation helper ────────────────────────────
void getPhysCell(int r, int c, int dir, int &pr, int &pc) {
  switch (dir) {
  case 0:
    pr = r;
    pc = c;
    break; // Up
  case 1:
    pr = GRID_SIZE - 1 - c;
    pc = r;
    break; // Right
  case 2:
    pr = GRID_SIZE - 1 - r;
    pc = GRID_SIZE - 1 - c;
    break; // Down
  case 3:
    pr = c;
    pc = GRID_SIZE - 1 - r;
    break; // Left
  }
}

// ─── Move Logic with Fast Snappy Animations ───────────────────
void rotateBoardRight() {
  int t[GRID_SIZE][GRID_SIZE];
  for (int r = 0; r < GRID_SIZE; r++)
    for (int c = 0; c < GRID_SIZE; c++)
      t[c][GRID_SIZE - 1 - r] = board[r][c];
  memcpy(board, t, sizeof(board));
}

bool canMove() {
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {
      if (!board[r][c])
        return true;
      if (c < GRID_SIZE - 1 && board[r][c] == board[r][c + 1])
        return true;
      if (r < GRID_SIZE - 1 && board[r][c] == board[r + 1][c])
        return true;
    }
  }
  return false;
}

void processMove(int dir) { // 0=Up, 1=Right, 2=Down, 3=Left
  if (gameOver)
    return;

  // 1. Rotate board to align target move direction with Left
  for (int i = 0; i < dir; i++)
    rotateBoardRight();

  animCount = 0;
  bool moved = false;
  int nextBoard[GRID_SIZE][GRID_SIZE] = {};

  // 2. Compute 1D slide & record trajectory for each tile
  for (int r = 0; r < GRID_SIZE; r++) {
    int targetC = 0;
    int lastMergedC = -1;

    for (int c = 0; c < GRID_SIZE; c++) {
      if (!board[r][c])
        continue;
      int val = board[r][c];

      // Check if can merge with previous cell
      if (targetC > 0 && nextBoard[r][targetC - 1] == val &&
          lastMergedC != targetC - 1) {
        // Merge!
        int destC = targetC - 1;
        nextBoard[r][destC] *= 2;
        score += nextBoard[r][destC];
        if (score > best)
          best = score;
        if (nextBoard[r][destC] == 2048)
          won = true;

        lastMergedC = destC;
        moved = true;

        int pr1, pc1, pr2, pc2;
        getPhysCell(r, c, dir, pr1, pc1);
        getPhysCell(r, destC, dir, pr2, pc2);
        animList[animCount++] = {pr1, pc1, pr2, pc2, val};
      } else {
        // Slide to targetC
        nextBoard[r][targetC] = val;
        if (c != targetC)
          moved = true;

        int pr1, pc1, pr2, pc2;
        getPhysCell(r, c, dir, pr1, pc1);
        getPhysCell(r, targetC, dir, pr2, pc2);
        animList[animCount++] = {pr1, pc1, pr2, pc2, val};

        targetC++;
      }
    }
  }

  // 3. Restore orientation
  for (int i = 0; i < (4 - dir) % 4; i++)
    rotateBoardRight();

  if (!moved)
    return; // No move happened

  // Update actual board matrix to target orientation
  for (int r = 0; r < GRID_SIZE; r++)
    for (int c = 0; c < GRID_SIZE; c++) {
      int pr, pc;
      getPhysCell(r, c, dir, pr, pc);
      board[pr][pc] = nextBoard[r][c];
    }

  // ─── 4. FAST SNAPPY SLIDE ANIMATION (4 Frames ~ 24ms) ──────
  const int SLIDE_FRAMES = 4;
  for (int frame = 1; frame <= SLIDE_FRAMES; frame++) {
    float p = (float)frame / (float)SLIDE_FRAMES;
    float t = p * (2.0f - p); // Fast ease-out curve

    // Background & empty slots
    canvas.fillRoundRect(0, 0, BOARD_SIZE, BOARD_SIZE, 10, rgb(187, 173, 160));
    for (int r = 0; r < GRID_SIZE; r++)
      for (int c = 0; c < GRID_SIZE; c++) {
        int x, y;
        getCellPos(r, c, x, y);
        canvas.fillRoundRect(x, y, CELL_SIZE, CELL_SIZE, 6, rgb(205, 193, 180));
      }

    // Draw moving tiles at interpolated (X, Y)
    for (int i = 0; i < animCount; i++) {
      int fx, fy, tx, ty;
      getCellPos(animList[i].fromR, animList[i].fromC, fx, fy);
      getCellPos(animList[i].toR, animList[i].toC, tx, ty);

      int cx = fx + (int)((tx - fx) * t);
      int cy = fy + (int)((ty - fy) * t);

      drawTileOnCanvas(cx, cy, CELL_SIZE, animList[i].val);
    }

    canvas.pushSprite(BOARD_X, BOARD_Y);
    delay(6);
  }

  // ─── 5. ADD NEW TILE & QUICK POP-IN ANIMATION (2 Frames ~ 12ms)
  addRandomTile();
  if (!canMove())
    gameOver = true;

  drawHeader(); // Update score display

  const int POP_FRAMES = 2;
  for (int frame = 1; frame <= POP_FRAMES; frame++) {
    float scale = 0.4f + 0.6f * ((float)frame / (float)POP_FRAMES);
    renderBoardCanvas(scale);
    canvas.pushSprite(BOARD_X, BOARD_Y);
    delay(6);
  }

  // Final static render
  redrawStatic();
}

// ─── Setup & Loop ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("Starting Official 2048 Classic Game Theme...");

  lcd.init();
  lcd.setBrightness(200);
  lcd.fillScreen(rgb(250, 248, 239));

  // Create 380x380 double buffer canvas in 8MB PSRAM
  canvas.setColorDepth(16);
  canvas.createSprite(BOARD_SIZE, BOARD_SIZE);

  randomSeed(esp_random());
  initGame();
  redrawStatic();

  Serial.println("2048 Game Ready.");
}

void loop() {
  int32_t tx = 0, ty = 0;
  bool touching = lcd.getTouch(&tx, &ty);

  if (touching) {
    if (!wasTouching) {
      // Touch Press Start
      startX = tx;
      startY = ty;
      lastX = tx;
      lastY = ty;
      swipeHandled = false;
    } else {
      // Touch Drag / Move
      lastX = tx;
      lastY = ty;

      int p_dx = lastY - startY;
      int p_dy = lastX - startX;

      int p_startY = startX;
      int p_startX = startY;

      // Trigger swipe during drag if threshold reached (35px)
      if (!swipeHandled && (abs(p_dx) > 35 || abs(p_dy) > 35)) {
        if (p_startY < HEADER_H + 10) {
          // Tap / Swipe in Header area
          if (p_startX < 140) {
            initGame();
            redrawStatic();
          }
          swipeHandled = true;
        } else if (!gameOver) {
          if (abs(p_dx) > abs(p_dy)) {
            processMove(p_dx > 0 ? 1
                                 : 3); // Physical Right = 1, Physical Left = 3
          } else {
            processMove(p_dy > 0 ? 2 : 0); // Physical Down = 2, Physical Up = 0
          }
          swipeHandled = true;
        }
      }
    }
  } else if (wasTouching) {
    // Touch Release / Finger Up
    if (!swipeHandled) {
      int p_dx = lastY - startY;
      int p_dy = lastX - startX;

      int p_startY = startX;
      int p_startX = startY;

      if (p_startY < HEADER_H + 10 && p_startX < 140) {
        initGame();
        redrawStatic();
      } else if (!gameOver && (abs(p_dx) > 15 || abs(p_dy) > 15)) {
        if (abs(p_dx) > abs(p_dy)) {
          processMove(p_dx > 0 ? 1 : 3);
        } else {
          processMove(p_dy > 0 ? 2 : 0);
        }
      }
    }
    swipeHandled = false;
  }

  wasTouching = touching;
  delay(15);
}
