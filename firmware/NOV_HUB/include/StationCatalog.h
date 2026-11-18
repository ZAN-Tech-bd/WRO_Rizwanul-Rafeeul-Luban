#pragma once
#include <stdint.h>

// RFID lives on the robot, not on the Hub. These are the agreed reference UIDs.
// stationId=0 is not an active artifact. End is a control card, NOT station 8.
struct CatalogCard {
  const char* role;
  uint8_t stationId;
  const char* id;
  const char* title;
  const char* uid;
  bool active;
  bool simulated;
};
static const CatalogCard STATION_CATALOG[] = {
  {"start",   0, "start",      "Start",               "83 54 6A 28", true,  false},
  {"station", 1, "terracotta", "Terracotta Plaque",   "64 14 F6 B3", true,  false},
  {"station", 2, "woodenboat", "Wooden boat",         "4A 6B DF 34", true,  false},
  {"station", 3, "rickshaw",   "Rickshaw Art",        "D8 36 D4 B9", true,  false},
  {"station", 4, "pottery",    "Traditional pottery", "94 60 6B B2", true,  false},
  {"station", 5, "flute",      "Bamboo flute",        "18 48 F4 BA", true,  false},
  {"station", 6, "basket",     "Bamboo basket",       "94 58 1A B3", true,  false},
  {"reserved",0, "jamdani",    "Jamdani shari",       "09 4C B1 92", false, false},
  {"end",     0, "end",        "End",                 "13 8C 5A 28", true,  false}
};
static const uint8_t CATALOG_CARD_COUNT = sizeof(STATION_CATALOG) / sizeof(STATION_CATALOG[0]);

