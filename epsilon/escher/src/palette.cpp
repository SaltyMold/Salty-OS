#include <assert.h>
#include <escher/palette.h>

namespace Escher {

// Note: the color members are now `inline static` (see palette.h), so they
// are fully defined in the header - the out-of-class
// `static inline KDColor Palette::X;` redeclarations that used to live here are
// no longer needed (and would no longer compile, since they're not
// `constexpr` anymore).

// RefreshDataColors() below fills these arrays one entry at a time: make sure
// they keep the expected size if someone adds or removes a data color.
static_assert(Palette::numberOfDataColors() == 10,
              "RefreshDataColors() must be updated");
static_assert(Palette::numberOfLightDataColors() == 10,
              "RefreshDataColors() must be updated");

KDColor Palette::nextDataColor(int* colorIndex) {
  int nbOfColors = numberOfDataColors();
  assert(*colorIndex < nbOfColors);
  KDColor c = DataColor[*colorIndex];
  *colorIndex = (*colorIndex + 1) % nbOfColors;
  return c;
}

void Palette::RefreshDataColors() {
  // DataColor/DataColorLight are initialized with literals in palette.h
  // (a dynamic initializer reading the named colors would never run on
  // device). Re-sync them with the named colors, which may just have been
  // overwritten by a theme.
  DataColor[0] = Red;
  DataColor[1] = Blue;
  DataColor[2] = Green;
  DataColor[3] = YellowDark;
  DataColor[4] = Magenta;
  DataColor[5] = Turquoise;
  DataColor[6] = Pink;
  DataColor[7] = Orange;
  DataColor[8] = Violet;
  DataColor[9] = Mint;

  DataColorLight[0] = RedLight;
  DataColorLight[1] = BlueLight;
  DataColorLight[2] = GreenLight;
  DataColorLight[3] = YellowLight;
  DataColorLight[4] = MagentaLight;
  DataColorLight[5] = TurquoiseLight;
  DataColorLight[6] = PinkLight;
  DataColorLight[7] = OrangeLight;
  DataColorLight[8] = VioletLight;
  DataColorLight[9] = MintLight;
}

void Palette::ApplyPalette(const KDColor* colors, size_t count) {
  CaptureDefaultsIfNeeded();
  size_t n = count < numberOfPaletteSlots() ? count : numberOfPaletteSlots();
  for (size_t i = 0; i < n; i++) {
    *s_paletteSlots[i] = colors[i];
  }
  RefreshDataColors();
}

void Palette::ResetToDefaults() {
  CaptureDefaultsIfNeeded();
  for (size_t i = 0; i < numberOfPaletteSlots(); i++) {
    *s_paletteSlots[i] = s_defaultValues[i];
  }
  RefreshDataColors();
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