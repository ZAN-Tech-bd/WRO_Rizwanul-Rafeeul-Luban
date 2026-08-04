// =====================================================================
//  main.cpp — SD card reader bring-up for ESP32.
//
//  Wiring (VSPI default pins — see include/config.h):
//      SD module   ESP32 GPIO
//      VCC         5V   (use a module WITH a 3.3V regulator; see config.h)
//      GND         GND
//      MISO        19
//      MOSI        23
//      SCK         18
//      CS          5
//
//  What it does on boot:
//      1. Mounts the card and prints its type + size.
//      2. Lists the full directory tree (root + subfolders) to Serial.
//      3. Dumps the contents of a default file (DEFAULT_READ_FILE) if present.
//
//  USB serial commands (115200 baud, one per line):
//      LS                  list the root directory
//      TREE                list root + all subdirectories
//      READ <path>         print a file's text contents, e.g.  READ /test.txt
//      INFO                reprint card type + size
//      HELP                list commands
// =====================================================================

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"

// File dumped automatically at boot if it exists. Change as needed.
static const char DEFAULT_READ_FILE[] = "/test.txt";

// Safety cap so a huge binary file never floods the serial line.
static const size_t MAX_DUMP_BYTES = 8 * 1024;

// ---- helpers --------------------------------------------------------

// Human-readable card type name.
static const char* cardTypeName(uint8_t t) {
  switch (t) {
    case CARD_NONE:    return "NONE / no card";
    case CARD_MMC:     return "MMC";
    case CARD_SD:      return "SD";
    case CARD_SDHC:    return "SDHC";
    default:           return "Unknown";
  }
}

// ---- directory iteration note --------------------------------------
// On the Arduino-ESP32 3.x core, File::openNextFile() builds each child
// File with a malloc'd _path that is free()'d before it is returned, leaving
// _path dangling. Calling .name() on such a File then reads freed memory and
// printf("%s", ...) crashes with LoadProhibited (strlen on garbage/NULL).
//
// FIX: iterate directories with getNextFileName(bool* isDir), which copies
// the raw d_name into a String at read time and is safe. Sizes are fetched
// by opening the file by full path (uses the file handle, not name()).

// Extract the last path component: "/mp3/0001.mp3" -> "0001.mp3".
// (getNextFileName() returns the FULL path, so we need this for display.)
static String baseName(const String& path) {
  int slash = path.lastIndexOf('/');
  return (slash >= 0 && slash < (int)path.length() - 1)
       ? path.substring(slash + 1)
       : path;
}

// Look up a file's size by opening it (uint64_t)-1 means "unknown".
static uint64_t fileSize(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return (uint64_t)-1;
  uint64_t s = f.size();
  f.close();
  return s;
}

// Pretty-print one directory entry from its name + flags.
static void printEntry(const String& name, bool isDir, uint64_t size) {
  if (isDir) {
    Serial.printf("  %-32s  <DIR>\n", name.c_str());
  } else {
    uint64_t sz = (size == (uint64_t)-1) ? 0ULL : size;
    Serial.printf("  %-32s  %llu bytes\n", name.c_str(), sz);
  }
}

// List one directory level (non-recursive).
static void listDir(const char* path) {
  File dir = SD.open(path);
  if (!dir) {
    Serial.printf("[LS] Cannot open %s\n", path);
    return;
  }
  Serial.printf("Listing: %s\n", path);
  bool isDir = false;
  while (true) {
    String full = dir.getNextFileName(&isDir);   // returns the FULL path
    if (!full.length()) break;                   // end of directory
    String nm = baseName(full);
    if (nm == "." || nm == "..") continue;
    uint64_t sz = isDir ? 0 : fileSize(full.c_str());
    printEntry(nm, isDir, sz);
  }
  dir.close();
}

// Recursive directory walk (root + nested folders).
static void walkTree(const char* path, uint8_t depth) {
  File dir = SD.open(path);
  if (!dir) {
    Serial.printf("[TREE] Cannot open %s\n", path);
    return;
  }
  for (uint8_t i = 0; i < depth; i++) Serial.print("    ");
  if (!dir.isDirectory()) {               // a plain file at the top
    Serial.printf("%s  (%llu bytes)\n", path, dir.size());
    dir.close();
    return;
  }
  Serial.printf("[%s]/\n", path);

  bool isDir = false;
  while (true) {
    String full = dir.getNextFileName(&isDir);   // returns the FULL path
    if (!full.length()) break;
    String nm = baseName(full);
    if (nm == "." || nm == "..") continue;
    if (isDir) {
      // Skip macOS/Windows indexing noise so the tree stays readable and fast
      // (these folders hold hundreds of tiny files and aren't user data).
      if (nm.startsWith(".") || nm == "System Volume Information") {
        for (uint8_t i = 0; i < depth + 1; i++) Serial.print("    ");
        Serial.printf("%s/  (skipped)\n", nm.c_str());
        continue;
      }
      for (uint8_t i = 0; i < depth + 1; i++) Serial.print("    ");
      Serial.printf("%s/\n", nm.c_str());
      walkTree(full.c_str(), depth + 1);         // full path is already correct
    } else {
      uint64_t sz = fileSize(full.c_str());
      for (uint8_t i = 0; i < depth + 1; i++) Serial.print("    ");
      printEntry(nm, isDir, sz);
    }
  }
  dir.close();
}

// Print a file's contents to Serial (text). Capped at MAX_DUMP_BYTES.
static void readFile(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("[READ] Could not open \"%s\". (Check path / use LS)\n", path);
    return;
  }
  if (f.isDirectory()) {
    Serial.printf("[READ] \"%s\" is a directory, not a file.\n", path);
    f.close();
    return;
  }

  // f.size() returns a 32-bit size_t -> cast to uint64_t for %llu (else printf
  // reads 8 bytes off the stack and prints garbage).
  Serial.printf("----- BEGIN \"%s\" (%llu bytes) -----\n", path, (uint64_t)f.size());
  size_t printed = 0;
  while (f.available() && printed < MAX_DUMP_BYTES) {
    // Read in small chunks instead of byte-by-byte for speed.
    uint8_t buf[64];
    int n = f.read(buf, sizeof(buf));
    if (n <= 0) break;
    // Only printable text + common whitespace; skip other control bytes.
    for (int i = 0; i < n && printed < MAX_DUMP_BYTES; i++, printed++) {
      char c = (char)buf[i];
      if (c == '\r') continue;                  // tidy line endings
      if (c == '\n' || c == '\t' || (c >= 0x20 && c <= 0x7E) || (c & 0x80)) {
        Serial.print(c);
      } else {
        Serial.print('.');                      // placeholder for control chars
      }
    }
  }
  if (f.available()) {
    Serial.printf("\n... (truncated at %u bytes)\n", (unsigned)MAX_DUMP_BYTES);
  }
  Serial.printf("\n------ END \"%s\" ------\n", path);
  f.close();
}

// Print card type, size and free space.
static void printCardInfo() {
  uint8_t  type = SD.cardType();
  uint64_t size = SD.cardSize();          // bytes
  uint64_t used = SD.usedBytes();

  Serial.println("---- SD card info ----");
  Serial.printf("  Type : %s\n", cardTypeName(type));
  if (type == CARD_NONE) return;
  Serial.printf("  Size : %llu MB  (%llu bytes)\n",
                size / (1024ULL * 1024ULL), size);
  Serial.printf("  Used : %llu MB  (%llu bytes)\n",
                used / (1024ULL * 1024ULL), used);
  Serial.println("----------------------");
}

// ---- serial command parsing ----------------------------------------

// Tiny line buffer for incoming USB commands.
static String cmdLine;

static void handleCommand(const String& raw) {
  String line = raw;
  line.trim();
  if (line.length() == 0) return;

  String cmd = line;
  cmd.toUpperCase();

  if (cmd == "HELP") {
    Serial.println("Commands: LS | TREE | READ <path> | INFO | HELP");
  } else if (cmd == "LS") {
    listDir("/");
  } else if (cmd == "TREE") {
    walkTree("/", 0);
  } else if (cmd == "INFO") {
    printCardInfo();
  } else if (cmd.startsWith("READ ")) {
    // Preserve original case of the path (filenames are case-sensitive here).
    String path = raw.substring(5);
    path.trim();
    readFile(path.c_str());
  } else {
    Serial.printf("Unknown command: \"%s\". Type HELP.\n", line.c_str());
  }
}

// ---- setup / loop --------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(400);                       // let the USB-CDC / monitor settle
  Serial.println();
  Serial.println("=== ESP32 SD card reader ===");

  // Configure VSPI with our explicit pins (they are the defaults anyway).
  Serial.printf("Pins -> SCK:%d MISO:%d MOSI:%d CS:%d\n",
                SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  // Mount the card. CS pin MUST be passed to SD.begin().
  Serial.print("Mounting SD card... ");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("FAILED.");
    Serial.println("Check: card seated, wiring, 3.3V/5V module power, CS pin.");
    return;                         // loop() will keep answering HELP, etc.
  }
  Serial.println("OK.");

  printCardInfo();

  // Show what's on the card.
  Serial.println();
  listDir("/");
  Serial.println();
  Serial.println("Full tree:");
  walkTree("/", 0);

  // Dump the default file if the user created one.
  Serial.println();
  if (SD.exists(DEFAULT_READ_FILE)) {
    readFile(DEFAULT_READ_FILE);
  } else {
    Serial.printf("(No %s on the card — create one, or use: READ /yourfile)\n",
                  DEFAULT_READ_FILE);
  }

  Serial.println();
  Serial.println("Ready. Type HELP for commands (set monitor to 115200, NL).");
}

void loop() {
  // Read one serial line so the user can browse the card interactively.
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdLine.length() > 0) {
        handleCommand(cmdLine);
        cmdLine = "";
      }
    } else if (cmdLine.length() < 127) {
      cmdLine += c;                 // cap to avoid unbounded RAM growth
    }
  }
}
