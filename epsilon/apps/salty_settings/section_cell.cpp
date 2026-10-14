#include "section_cell.h"

#include <assert.h>
#include <escher/theme/highlight.h>

using namespace Escher;

namespace SaltySettings {

SectionCell::SectionCell()
    : Bordered(),
      HighlightCell(),
      m_textView({.style = {.glyphColor = k_textColor,
                            .backgroundColor = k_backgroundColor,
                            .font = KDFont::Size::Small}}) {}

void SectionCell::drawRect(KDContext* ctx, KDRect rect) const {
  DrawInnerRect(ctx, bounds(), Highlight(k_backgroundColor, isHighlighted()));
  DrawBorderOfRect(ctx, bounds(), ThemeComponent::BorderGrayBright);
}

void SectionCell::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  if (width == 0 || height == 0) {
    return;
  }
  KDCoordinate x = k_separatorThickness + Metric::CellMargins.left();
  width -= x + Metric::CellMargins.right() + k_separatorThickness;
  // Same layout as the subtitles of the Python menu
  KDCoordinate topOffset = 2;
  KDCoordinate y = k_separatorThickness + topOffset;
  height -= y + k_separatorThickness;

  assert(width > 0 && height > 0);

  setChildFrame(&m_textView, KDRect(x, y, width, height), force);
}

}  // namespace SaltySettings
