#include "histogram_banner_view.h"

#include <apps/constant.h>
#include <assert.h>

using namespace Escher;

namespace Statistics {

HistogramBannerView::HistogramBannerView()
    : m_seriesName(BannerFieldFormat()),
      m_intervalBuffer(BannerFieldFormat()),
      m_frequencyBuffer(BannerFieldFormat()),
      m_relativeFrequencyBuffer(BannerFieldFormat()) {}

View* HistogramBannerView::subviewAtIndex(int index) {
  assert(0 <= index && index < numberOfSubviews());
  View* subviews[] = {&m_seriesName, &m_intervalBuffer, &m_frequencyBuffer,
                      &m_relativeFrequencyBuffer};
  return subviews[index];
}

}  // namespace Statistics
