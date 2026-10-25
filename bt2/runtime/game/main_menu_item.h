#pragma once
#include "ee/ee_runtime.h"
#include <string>

// Dragon Rage's eleventh item of the main menu, "Online Mode" (after Dragon Library), for a player's run with BT3's
// renderer (BT2_RENDERER=bt3). The overlay's main menu has ten items; the functions that read its cursor and draw
// its wheel are replaced by ones that count eleven (main_menu_item.cpp). The item's pictures are the game's own
// kind: its name in the free fourth row of the third sheet of names, its plate in a third row the sheet of plates
// is given (the renderer's menu art swap, gs/hw/bt3_host.c), both read from `art` (a folder: text.raw and plate.raw,
// 512 x 64 RGBA with the GS's alpha; sheets.txt, the two sheets' hashes). Choosing it does nothing yet.
void installMainMenuItem(EeRuntime& runtime, const std::string& art);
