#include "box_banner_view.h"

#include <assert.h>

using namespace Escher;

namespace Statistics {

BoxBannerView::BoxBannerView()
    : m_seriesName(BannerFieldFormat()),
      m_calculationBuffer(BannerFieldFormat()) {}

View* BoxBannerView::subviewAtIndex(int index) {
  assert(0 <= index && index < numberOfSubviews());
  View* subviews[] = {&m_seriesName, &m_calculationBuffer};
  return subviews[index];
}

}  // namespace Statistics
