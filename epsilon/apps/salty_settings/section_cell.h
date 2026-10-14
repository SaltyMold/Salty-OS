#pragma once

#include <escher/bordered.h>
#include <escher/buffer_text_view.h>
#include <escher/highlight_cell.h>
#include <escher/metric.h>
#include <escher/theme/components.h>
#include <ion/display.h>

namespace SaltySettings {

/* Title of a group of rows. It cannot be selected. */
class SectionCell : public Escher::Bordered, public Escher::HighlightCell {
 public:
  SectionCell();
  void setHighlighted(bool highlight) override { assert(!highlight); }
  KDSize minimalSizeForOptimalDisplay() const override {
    return KDSize(bounds().width(), k_height);
  }
  void drawRect(KDContext* ctx, KDRect rect) const override;
  void layoutSubviews(bool force = false) override;
  // Actual height is 24 with the overlapping pixel from SelectableTableView.
  constexpr static KDCoordinate k_height = 23;

  Escher::AbstractBufferTextView* textView() { return &m_textView; }

 private:
  constexpr static Escher::ThemeComponent k_backgroundColor =
      Escher::ThemeComponent::WallScreen;
  constexpr static Escher::ThemeComponent k_textColor =
      Escher::ThemeComponent::TextGrayDark;

  int numberOfSubviews() const override { return 1; }
  Escher::View* subviewAtIndex(int index) override {
    assert(index == 0);
    return &m_textView;
  }

  Escher::OneLineBufferTextView<> m_textView;
};

}  // namespace SaltySettings
