#include <assert.h>
#include <escher/palette.h>

namespace Escher {

// Note: the color members are now `inline static` (see palette.h), so they
// are fully defined in the header - the out-of-class
// `static inline KDColor Palette::X;` redeclarations that used to live here are
// no longer needed (and would no longer compile, since they're not
// `constexpr` anymore).

KDColor Palette::nextDataColor(int* colorIndex) {
  int nbOfColors = numberOfDataColors();
  assert(*colorIndex < nbOfColors);
  KDColor c = DataColor[*colorIndex];
  *colorIndex = (*colorIndex + 1) % nbOfColors;
  return c;
}

void Palette::ApplyPalette(const KDColor* colors, size_t count) {
  CaptureDefaultsIfNeeded();
  size_t n = count < numberOfPaletteSlots() ? count : numberOfPaletteSlots();
  for (size_t i = 0; i < n; i++) {
    *s_paletteSlots[i] = colors[i];
  }
}

void Palette::ResetToDefaults() {
  CaptureDefaultsIfNeeded();
  for (size_t i = 0; i < numberOfPaletteSlots(); i++) {
    *s_paletteSlots[i] = s_defaultValues[i];
  }
}

bool Palette::s_defaultsCaptured = false;
KDColor Palette::s_defaultValues[Palette::numberOfPaletteSlots()];

void Palette::CaptureDefaultsIfNeeded() {
  // Runs on the very first ApplyPalette()/ResetToDefaults() call, i.e.
  // before anything has ever overwritten the slots, so this snapshot is
  // guaranteed to hold the compiled-in defaults declared in palette.h.
  if (s_defaultsCaptured) {
    return;
  }
  for (size_t i = 0; i < numberOfPaletteSlots(); i++) {
    s_defaultValues[i] = *s_paletteSlots[i];
  }
  s_defaultsCaptured = true;
}

}  // namespace Escher