// Pixel Buzz Box - High Score Implementation
#include "highscore.h"
#include <EEPROM.h>

// -------------------- GLOBAL STATE --------------------
HighScoreTable highScoreTable;
HighScoreEntryData hsEntry;

// -------------------- INTERNAL HELPERS --------------------

static uint16_t calculateChecksum(const HighScoreTable& table) {
  uint16_t sum = 0;
  const uint8_t* data = (const uint8_t*)&table;
  // XOR all bytes except the checksum field itself
  size_t checksumOffset = offsetof(HighScoreTable, checksum);
  for (size_t i = 0; i < checksumOffset; i++) {
    sum ^= data[i];
    sum = (sum << 1) | (sum >> 15);  // Rotate left
  }
  return sum;
}

static bool validateTable(const HighScoreTable& table) {
  if (table.magic != HighScore::MAGIC) return false;
  if (table.version != HighScore::VERSION) return false;
  if (table.count > HighScore::MAX_ENTRIES) return false;
  if (calculateChecksum(table) != table.checksum) return false;
  return true;
}

static void initEmptyTable() {
  highScoreTable.magic = HighScore::MAGIC;
  highScoreTable.version = HighScore::VERSION;
  highScoreTable.count = 0;
  highScoreTable.reserved[0] = 0;
  highScoreTable.reserved[1] = 0;
  for (int i = 0; i < HighScore::MAX_ENTRIES; i++) {
    highScoreTable.entries[i].initials[0] = '-';
    highScoreTable.entries[i].initials[1] = '-';
    highScoreTable.entries[i].initials[2] = '-';
    highScoreTable.entries[i].initials[3] = '\0';
    highScoreTable.entries[i].score = 0;
    highScoreTable.entries[i].dayReached = 0;
    highScoreTable.entries[i].flags = 0;
  }
  highScoreTable.checksum = calculateChecksum(highScoreTable);
}

static void initEntryState() {
  hsEntry.state = HS_INACTIVE;
  hsEntry.cursorPos = 0;
  hsEntry.charIndex[0] = 0;  // 'A'
  hsEntry.charIndex[1] = 0;
  hsEntry.charIndex[2] = 0;
  hsEntry.score = 0;
  hsEntry.day = 0;
  hsEntry.rank = -1;
  hsEntry.stateStartMs = 0;
  hsEntry.lastJoyY = 0;
  hsEntry.lastJoyX = 0;
}

// -------------------- PUBLIC API --------------------

void initHighScores() {
  EEPROM.begin(sizeof(HighScoreTable) + 16);  // Extra margin
  EEPROM.get(HighScore::EEPROM_ADDR, highScoreTable);

  if (!validateTable(highScoreTable)) {
    initEmptyTable();
    saveHighScores();
  }

  initEntryState();
}

bool isHighScore(uint16_t score) {
  if (score == 0) return false;

  // If table not full, any score qualifies
  if (highScoreTable.count < HighScore::MAX_ENTRIES) return true;

  // Check if score beats the lowest entry
  return score > highScoreTable.entries[HighScore::MAX_ENTRIES - 1].score;
}

int8_t getHighScoreRank(uint16_t score) {
  if (score == 0) return -1;

  for (int i = 0; i < (int)highScoreTable.count; i++) {
    if (score > highScoreTable.entries[i].score) {
      return (int8_t)i;
    }
  }

  // If table not full, new entry goes at the end
  if (highScoreTable.count < HighScore::MAX_ENTRIES) {
    return (int8_t)highScoreTable.count;
  }

  return -1;  // Doesn't qualify
}

const HighScoreTable& getHighScores() {
  return highScoreTable;
}

void beginHighScoreEntry(uint16_t score, uint8_t day, bool diedAtNight) {
  hsEntry.state = HS_ENTERING;
  hsEntry.cursorPos = 0;
  hsEntry.charIndex[0] = 0;
  hsEntry.charIndex[1] = 0;
  hsEntry.charIndex[2] = 0;
  hsEntry.score = score;
  hsEntry.day = day;
  hsEntry.rank = getHighScoreRank(score);
  hsEntry.stateStartMs = millis();
  hsEntry.lastJoyY = 0;
  hsEntry.lastJoyX = 0;
}

void updateHighScoreEntry(bool btnPressed, int8_t joyX, int8_t joyY) {
  if (hsEntry.state != HS_ENTERING) return;

  // Joystick Y: cycle characters (with edge detection)
  int8_t joyYEdge = 0;
  if (joyY > 30 && hsEntry.lastJoyY <= 30) joyYEdge = 1;   // Down
  if (joyY < -30 && hsEntry.lastJoyY >= -30) joyYEdge = -1; // Up
  hsEntry.lastJoyY = joyY;

  if (joyYEdge != 0) {
    int idx = hsEntry.charIndex[hsEntry.cursorPos];
    idx += joyYEdge;
    if (idx < 0) idx = HighScore::CHAR_COUNT - 1;
    if (idx >= HighScore::CHAR_COUNT) idx = 0;
    hsEntry.charIndex[hsEntry.cursorPos] = (uint8_t)idx;
  }

  // Joystick X: move cursor (with edge detection)
  int8_t joyXEdge = 0;
  if (joyX > 30 && hsEntry.lastJoyX <= 30) joyXEdge = 1;   // Right
  if (joyX < -30 && hsEntry.lastJoyX >= -30) joyXEdge = -1; // Left
  hsEntry.lastJoyX = joyX;

  if (joyXEdge != 0) {
    int pos = hsEntry.cursorPos + joyXEdge;
    if (pos < 0) pos = 0;
    if (pos > 2) pos = 2;
    hsEntry.cursorPos = (uint8_t)pos;
  }

  // Button: confirm
  if (btnPressed) {
    if (hsEntry.cursorPos < 2) {
      // Move to next position
      hsEntry.cursorPos++;
    } else {
      // All three entered - save and transition
      char initials[4];
      initials[0] = HighScore::CHAR_SET[hsEntry.charIndex[0]];
      initials[1] = HighScore::CHAR_SET[hsEntry.charIndex[1]];
      initials[2] = HighScore::CHAR_SET[hsEntry.charIndex[2]];
      initials[3] = '\0';

      uint8_t flags = 0;
      // Could add flags here based on game state

      insertHighScore(initials, hsEntry.score, hsEntry.day, flags);
      saveHighScores();

      hsEntry.state = HS_SAVING;
      hsEntry.stateStartMs = millis();
    }
  }
}

bool isHighScoreEntryActive() {
  return hsEntry.state == HS_ENTERING || hsEntry.state == HS_SAVING;
}

bool isHighScoreEntryComplete() {
  if (hsEntry.state == HS_SAVING) {
    // Brief "SAVED" display for 800ms
    if (millis() - hsEntry.stateStartMs > 800) {
      hsEntry.state = HS_DONE;
    }
  }
  return hsEntry.state == HS_DONE || hsEntry.state == HS_INACTIVE;
}

void insertHighScore(const char* initials, uint16_t score, uint8_t day, uint8_t flags) {
  int8_t rank = getHighScoreRank(score);
  if (rank < 0) return;

  // Shift lower scores down
  for (int i = HighScore::MAX_ENTRIES - 1; i > rank; i--) {
    highScoreTable.entries[i] = highScoreTable.entries[i - 1];
  }

  // Insert new entry
  strncpy(highScoreTable.entries[rank].initials, initials, 3);
  highScoreTable.entries[rank].initials[3] = '\0';
  highScoreTable.entries[rank].score = score;
  highScoreTable.entries[rank].dayReached = day;
  highScoreTable.entries[rank].flags = flags;

  // Update count
  if (highScoreTable.count < HighScore::MAX_ENTRIES) {
    highScoreTable.count++;
  }

  // Update rank in entry state for highlighting
  hsEntry.rank = rank;
}

void saveHighScores() {
  highScoreTable.checksum = calculateChecksum(highScoreTable);
  EEPROM.put(HighScore::EEPROM_ADDR, highScoreTable);
  EEPROM.commit();
}

void resetHighScores() {
  initEmptyTable();
  saveHighScores();
  initEntryState();
}
