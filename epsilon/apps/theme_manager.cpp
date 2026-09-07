#include "theme_manager.h"

using namespace Ion::Device::Board::Config;

const ThemeAreaHeader* ThemeManager::s_header = nullptr;
bool ThemeManager::s_valid = false;

void ThemeManager::init() {
  const ThemeAreaHeader* h = reinterpret_cast<const ThemeAreaHeader*>(ThemeAreaStart);

  if (h->magic != ThemeAreaHeaderMagic ||
      h->version != ThemeAreaHeaderVersion ||
      h->themeCount > ThemeCount) {
    s_valid = false;
    return;
  }

  s_header = h;
  s_valid = true;
}

const ThemeEntry* ThemeManager::entry(int index) {
  if (!s_valid || index < 0 || index >= (int)s_header->themeCount) {
    return nullptr;
  }
  const uint8_t* base = reinterpret_cast<const uint8_t*>(ThemeAreaStart) + sizeof(ThemeAreaHeader);
  return reinterpret_cast<const ThemeEntry*>(base) + index;
}