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
  size_t n = count < numberOfPaletteSlots() ? count : numberOfPaletteSlots();
  for (size_t i = 0; i < n; i++) {
    *s_paletteSlots[i] = colors[i];
  }
}

}  // namespace Escher