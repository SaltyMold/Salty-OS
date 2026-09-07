#pragma once

#include <stdint.h>
#include <ion/src/device/n0120/shared/drivers/config/board.h> 

class ThemeManager {
public:
  static void init();
  static bool isValid() { return s_valid; }
  static int themeCount() { return s_valid ? (int)s_header->themeCount : 0; }
  static const Ion::Device::Board::Config::ThemeEntry* entry(int index);

private:
  static const Ion::Device::Board::Config::ThemeAreaHeader* s_header;
  static bool s_valid;
};