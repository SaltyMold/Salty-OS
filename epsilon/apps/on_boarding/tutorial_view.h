#pragma once

#include <apps/i18n.h>
#include <escher/image.h>
#include <escher/view.h>
#include <ion/keyboard.h>
#include <kandinsky/color.h>

namespace OnBoarding {

/* One page of the Salty-OS tutorial: a title, a few lines of text and the
 * shortcuts to press, drawn with the images of the keys (keys/ folder). The
 * keys shown come from shortcuts.h, so they follow any remapping. */
class TutorialView : public Escher::View {
 public:
  constexpr static int k_numberOfPages = 4;

  int page() const { return m_page; }
  bool isFirstPage() const { return m_page == 0; }
  bool isLastPage() const { return m_page == k_numberOfPages - 1; }
  void setPage(int page);

  /* Remembers which keys of the page are held down: their image is drawn a bit
   * lower. Returns true if something changed (the view is then dirty). */
  bool setPressedKeys(Ion::Keyboard::State state);
  bool hasPressedKeys() const { return m_pressedMask != 0; }

  void drawRect(KDContext* ctx, KDRect rect) const override;

 private:
  // The biggest key image is the parenthesis one: 70 x 44
  constexpr static int k_maxNumberOfPixels = 70 * 44;

  struct ComboGeometry {
    KDCoordinate top;  // Top of the row of images
    KDCoordinate prefix1X;
    KDCoordinate prefix2X;
    KDCoordinate plusX;  // Only if the combo has a key
    KDCoordinate keyX;   // Only if the combo has a key
  };
  ComboGeometry comboGeometry(int index) const;

  void drawCentered(KDContext* ctx, const char* text, KDCoordinate y,
                    KDFont::Size font, KDColor color) const;
  // Draws nothing if the image is outside of the rect that is redrawn
  void drawImage(KDContext* ctx, KDRect rect, const Escher::Image* image,
                 KDCoordinate x, KDCoordinate y) const;
  void drawCombo(KDContext* ctx, KDRect rect, int index) const;

  int m_page = 0;
  /* Bits 0 and 1: prefix keys. Bit 2 + i: key of the combo i of the page. */
  uint8_t m_pressedMask = 0;
  mutable KDColor m_pixels[k_maxNumberOfPixels];
};

}  // namespace OnBoarding
