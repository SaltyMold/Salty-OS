#include "splash_view.h"

#include <omg/memory.h>

namespace OnBoarding {

void SplashView::drawRect(KDContext* ctx, KDRect rect) const {
  // salty_os.png is opaque and was blended on a white background
  ctx->fillRect(bounds(), KDColorWhite);
}

void SplashView::layoutSubviews(bool force) {
  KDSize logoSize = m_logoView.minimalSizeForOptimalDisplay();
  setChildFrame(&m_logoView,
                KDRect((bounds().width() - logoSize.width()) / 2,
                       (bounds().height() - logoSize.height()) / 2,
                       logoSize.width(), logoSize.height()),
                force);
}

void SplashView::LogoView::drawRect(KDContext* ctx, KDRect rect) const {
  if (m_image == nullptr) {
    return;
  }
  assert(bounds().width() == m_image->width());
  assert(bounds().height() == m_image->height());

  int numberOfPixels = m_image->width() * m_image->height();
  assert(numberOfPixels <= k_maxNumberOfPixels);

  OMG::Memory::Decompress(
      m_image->compressedPixelData(), reinterpret_cast<uint8_t*>(m_pixels),
      m_image->compressedPixelDataSize(), numberOfPixels * sizeof(KDColor));

  ctx->fillRectWithPixels(bounds(), m_pixels, nullptr);
}

}  // namespace OnBoarding
