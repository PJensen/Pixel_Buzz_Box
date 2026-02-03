// Pixel Buzz Box - High Score System
// Persistent storage using EEPROM emulation on RP2040 flash
#pragma once

#include <Arduino.h>

// -------------------- CONSTANTS --------------------
namespace HighScore {
  constexpr uint32_t MAGIC = 0xBEE5C0DE;  // Magic number for validation
  constexpr uint8_t VERSION = 1;
  constexpr uint8_t MAX_ENTRIES = 5;
  constexpr uint8_t INITIALS_LEN = 3;
  constexpr uint16_t EEPROM_ADDR = 0;
  constexpr char CHAR_SET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
  constexpr uint8_t CHAR_COUNT = 37;  // 26 letters + 10 digits + space
}

// -------------------- DATA STRUCTURES --------------------
struct HighScoreEntry {
  char initials[4];     // 3 chars + null terminator
  uint16_t score;
  uint8_t dayReached;
  uint8_t flags;        // Reserved: bit 0 = died at night
};

struct HighScoreTable {
  uint32_t magic;
  uint8_t version;
  uint8_t count;
  uint8_t reserved[2];
  HighScoreEntry entries[HighScore::MAX_ENTRIES];
  uint16_t checksum;
};

// -------------------- ENTRY STATE --------------------
enum HighScoreEntryState : uint8_t {
  HS_INACTIVE = 0,
  HS_ENTERING,      // Player selecting initials
  HS_SAVING,        // Brief save animation
  HS_DONE           // Entry complete, show leaderboard
};

struct HighScoreEntryData {
  HighScoreEntryState state;
  uint8_t cursorPos;          // 0-2: which initial being edited
  uint8_t charIndex[3];       // Index into CHAR_SET for each position
  uint16_t score;             // Score being entered
  uint8_t day;                // Day reached
  int8_t rank;                // Rank achieved (-1 if not a high score)
  uint32_t stateStartMs;      // Timestamp for animations
  int8_t lastJoyY;            // For edge detection on joystick
  int8_t lastJoyX;
};

// -------------------- GLOBAL STATE --------------------
extern HighScoreTable highScoreTable;
extern HighScoreEntryData hsEntry;

// -------------------- API --------------------

// Initialization (call in setup)
void initHighScores();

// Query functions
bool isHighScore(uint16_t score);
int8_t getHighScoreRank(uint16_t score);  // Returns 0-4, or -1 if not qualifying
const HighScoreTable& getHighScores();

// Entry flow
void beginHighScoreEntry(uint16_t score, uint8_t day, bool diedAtNight);
void updateHighScoreEntry(bool btnPressed, int8_t joyX, int8_t joyY);
bool isHighScoreEntryActive();
bool isHighScoreEntryComplete();

// Persistence
void insertHighScore(const char* initials, uint16_t score, uint8_t day, uint8_t flags);
void saveHighScores();
void resetHighScores();
