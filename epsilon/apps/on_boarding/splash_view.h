#pragma once

#include <assert.h>
#include <escher/image.h>
#include <escher/image_view.h>
#include <escher/view.h>
#include <kandinsky/color.h>

namespace OnBoarding {

/* The splash screen displays salty_os.png, centered on a white background.
 *
 * Like every other image of the firmware, salty_os.png is converted to an
 * LZ4-compressed Escher::Image by the inliner at build time (see Makefile).
 *
 * It is however much bigger than the icons Escher::ImageView is made for:
 * 300 x 73 = 21900 pixels (43.8 KB once decompressed), whereas ImageView only
 * handles 4000 pixels and the whole userland stack is 32 KiB. LogoView thus
 * decompresses the image in a buffer it owns, instead of on the stack. */
class SplashView : public Escher::View {
 public:
  void setImage(const Escher::Image* image) { m_logoView.setImage(image); }
  void drawRect(KDContext* ctx, KDRect rect) const override;

 private:
  class LogoView : public Escher::ImageView {
   public:
    void drawRect(KDContext* ctx, KDRect rect) const override;

   private:
    // Must be at least the number of pixels of salty_os.png
    constexpr static int k_maxNumberOfPixels = 300 * 73;
    mutable KDColor m_pixels[k_maxNumberOfPixels];
  };

  int numberOfSubviews() const override { return 1; }
  Escher::View* subviewAtIndex(int index) override {
    assert(index == 0);
    return &m_logoView;
  }
  void layoutSubviews(bool force = false) override;

  LogoView m_logoView;
};

}  // namespace OnBoarding
