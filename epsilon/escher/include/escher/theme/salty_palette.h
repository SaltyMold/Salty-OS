#pragma once

#include <kandinsky/color.h>
#include <stddef.h>

#include "palette_values.h"
#include "theme_container.h"

namespace Escher {

/* Salty-OS runtime palette. A flashed theme lists its colors by name (core
 * colors, then the colors of each app: Calculation, Python, Graph...), the
 * names being the ones of SALTY_PALETTE_SLOTS. Nothing about the theme is
 * compiled in: selecting a theme starts from the default values, then every
 * color the theme provides replaces the one of the same name. A name this build
 * does not know is ignored, a color the theme does not provide keeps its
 * default value.
 * Applying a theme rebuilds a light-like ThemeContainer that ColorPicker uses
 * instead of Epsilon's light/dark themes, until Reset() is called ("Default"
 * theme). */
class SaltyPalette {
 public:
  // Number of colors this build knows by name
  static constexpr size_t numberOfSlots() { return k_numberOfPaletteSlots; }

  // Applying a theme: Begin(), one Set() per color of the theme, then End().
  static void Begin();
  // Returns false (and ignores the color) when no color has this name.
  static bool Set(const char* name, KDColor color);
  static void End();
  // Back to Epsilon's own light / dark theme.
  static void Reset();

  static bool IsActive() { return s_active; }
  /* A theme whose Background is dark gets Epsilon's behavior for dark themes:
   * dark variants of the images, grid in tables, lighter blending, double
   * selection borders. The colors themselves always come from the palette. */
  static bool IsDark() { return s_active && s_dark; }
  static const ThemeContainer& Theme() { return s_theme; }

  // Every color of the active theme (the default values if none is active)
  static const PaletteValues& Values() {
    return s_active ? s_values : k_defaultPaletteValues;
  }

  // Salty-only slots, with defaults when no palette is active
  static KDColor WallpaperColor() { return Values().WallpaperColor; }
  static KDColor TextColor() { return Values().TextColor; }
  static KDColor TextColorHover() { return Values().TextColorHover; }

 private:
  /* No dynamic initializers on device (static constructors are not run): these
   * are only read once IsActive() is true, i.e. after End() filled them. */
  static bool s_active;
  static bool s_dark;
  static PaletteValues s_values;
  static ThemeContainer s_theme;
};

}  // namespace Escher
