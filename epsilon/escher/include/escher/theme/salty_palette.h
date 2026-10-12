#pragma once

#include <kandinsky/color.h>
#include <stddef.h>

#include "theme_container.h"

namespace Escher {

/* Salty-OS runtime palette. A Salty theme provides numberOfSlots() colors (see
 * themes/ folder, the slot order is the order of the theme palette files). Applying
 * them rebuilds a light-like ThemeContainer that ColorPicker uses instead of
 * Epsilon's light/dark themes, until Reset() is called ("Default" theme). */
class SaltyPalette {
 public:
  static constexpr size_t numberOfSlots() { return k_numberOfSlots; }

  // Overrides the first min(count, numberOfSlots()) slots and activates.
  static void Apply(const KDColor* colors, size_t count);
  // Back to Epsilon's own light / dark theme.
  static void Reset();

  static bool IsActive() { return s_active; }
  static const ThemeContainer& Theme() { return s_theme; }

  // Salty-only slots, with defaults when no palette is active
  static KDColor WallpaperColor();
  static KDColor TextColor();
  static KDColor TextColorHover();

 private:
  static constexpr size_t k_numberOfSlots = 43;
  /* No dynamic initializers on device (static constructors are not run): these
   * are only read once IsActive() is true, i.e. after Apply() filled them. */
  static bool s_active;
  static ThemeContainer s_theme;
};

}  // namespace Escher
