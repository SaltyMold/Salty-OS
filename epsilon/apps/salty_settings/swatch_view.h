#pragma once

#include <escher/cell_widget.h>
#include <escher/view.h>
#include <kandinsky/color.h>

namespace SaltySettings {

/* A small square filled with a color, usable as the accessory of a MenuCell. */
class SwatchView : public Escher::View, public Escher::CellWidget {
 public:
  void setColor(KDColor color) {
    if (color != m_color) {
      m_color = color;
      markWholeFrameAsDirty();
    }
  }
  KDSize minimalSizeForOptimalDisplay() const override {
    return KDSize(k_size, k_size);
  }
  void drawRect(KDContext* ctx, KDRect rect) const override {
    // Gray frame so that any color stays visible on any background
    ctx->fillRect(bounds(), KDColor::RGB24(0x808080));
    ctx->fillRect(KDRect(1, 1, bounds().width() - 2, bounds().height() - 2),
                  m_color);
  }

  // CellWidget
  const Escher::View* view() const override { return this; }

  constexpr static KDCoordinate k_size = 16;

 private:
  KDColor m_color = KDColor::RGB24(0x000000);
};

}  // namespace SaltySettings
