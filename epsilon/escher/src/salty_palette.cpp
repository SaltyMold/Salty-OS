#include <escher/theme/light_theme.h>
#include <escher/theme/salty_palette.h>
#include <string.h>

namespace Escher {

bool SaltyPalette::s_active = false;
bool SaltyPalette::s_dark = false;
PaletteValues SaltyPalette::s_values;
ThemeContainer SaltyPalette::s_theme;

namespace {

struct Slot {
  const char* name;
  KDColor PaletteValues::*member;
};

constexpr Slot k_slots[] = {
#define X(name) {#name, &PaletteValues::name},
    SALTY_PALETTE_SLOTS(X)
#undef X
};
static_assert(sizeof(k_slots) / sizeof(k_slots[0]) == k_numberOfPaletteSlots);

}  // namespace

void SaltyPalette::Begin() {
  /* Deactivated until End(): s_values is being rewritten, nothing must read
   * it in the meantime (Values() falls back to the default values). */
  s_active = false;
  s_values = k_defaultPaletteValues;
}

bool SaltyPalette::Set(const char* name, KDColor color) {
  for (const Slot& slot : k_slots) {
    if (strcmp(slot.name, name) == 0) {
      s_values.*slot.member = color;
      return true;
    }
  }
  return false;
}

void SaltyPalette::End() {
  s_theme = MakeLightTheme(s_values);
  // Perceived brightness of the background (ITU-R BT.601), 0 to 255
  const KDColor background = s_values.Background;
  s_dark = (background.red() * 299 + background.green() * 587 +
            background.blue() * 114) /
               1000 <
           128;
  s_active = true;
}

void SaltyPalette::Reset() { s_active = false; }

}  // namespace Escher
