#include "tutorial_view.h"

#include <apps/shortcuts.h>
#include <assert.h>
#include <omg/memory.h>

#include "keys/alpha.h"
#include "keys/backspace.h"
#include "keys/left_parenthesis.h"
#include "keys/right_parenthesis.h"
#include "keys/shift.h"
#include "keys/xnt.h"

namespace OnBoarding {

namespace {

constexpr KDColor k_titleColor = KDBlack;
constexpr KDColor k_textColor = KDColor::RGB24(0x6E6E6E);
constexpr KDColor k_hintColor = KDColor::RGB24(0x8A8A8A);

constexpr KDCoordinate k_margin = 8;
constexpr KDCoordinate k_parenthesisHeight = 44;
constexpr KDCoordinate k_keyHeight = 33;
constexpr KDCoordinate k_keyWidth = 50;
constexpr KDCoordinate k_parenthesisWidth = 70;
constexpr KDCoordinate k_imageGap = 4;
constexpr KDCoordinate k_plusMargin = 10;
constexpr KDCoordinate k_labelGap = 10;
// A pressed key is drawn this much lower
constexpr KDCoordinate k_pressedOffset = 6;
constexpr KDCoordinate k_comboGap = 8;

// What a page shows. A combo is "( ) + key", or only "( )" if there is no key
struct Combo {
  bool withKey;
  Shortcuts::Action action;  // Ignored if !withKey
  I18n::Message label;
};

struct Page {
  I18n::Message title;
  int numberOfLines;
  I18n::Message lines[3];
  int numberOfCombos;
  Combo combos[2];
};

constexpr Page k_pages[TutorialView::k_numberOfPages] = {
    {I18n::Message::OnBoardingWelcomeTitle,
     3,
     {I18n::Message::OnBoardingWelcomeText1,
      I18n::Message::OnBoardingWelcomeText2,
      I18n::Message::OnBoardingWelcomeText3},
     1,
     {{false, Shortcuts::Action::NumberOfActions,
       I18n::Message::OnBoardingHoldBoth}}},
    {I18n::Message::OnBoardingThemesTitle,
     0,
     {},
     2,
     {{true, Shortcuts::Action::NextTheme, I18n::Message::OnBoardingNextTheme},
      {true, Shortcuts::Action::PreviousTheme,
       I18n::Message::OnBoardingPreviousTheme}}},
    {I18n::Message::OnBoardingExternalTitle,
     0,
     {},
     1,
     {{true, Shortcuts::Action::ToggleExternalApps,
       I18n::Message::OnBoardingToggleExternal}}},
    {I18n::Message::OnBoardingSettingsTitle,
     0,
     {},
     1,
     {{true, Shortcuts::Action::OpenSettings,
       I18n::Message::OnBoardingOpenSettings}}},
};

// Only the keys that can be assigned to a shortcut have an image
const Escher::Image* ImageForKey(Ion::Keyboard::Key key) {
  switch (key) {
    case Ion::Keyboard::Key::LeftParenthesis:
      return ImageStore::LeftParenthesis;
    case Ion::Keyboard::Key::RightParenthesis:
      return ImageStore::RightParenthesis;
    case Ion::Keyboard::Key::Shift:
      return ImageStore::Shift;
    case Ion::Keyboard::Key::Alpha:
      return ImageStore::Alpha;
    case Ion::Keyboard::Key::XNT:
      return ImageStore::Xnt;
    case Ion::Keyboard::Key::Backspace:
      return ImageStore::Backspace;
    default:
      return nullptr;
  }
}

}  // namespace

void TutorialView::setPage(int page) {
  assert(page >= 0 && page < k_numberOfPages);
  if (page != m_page) {
    m_page = page;
    m_pressedMask = 0;
    markWholeFrameAsDirty();
  }
}

bool TutorialView::setPressedKeys(Ion::Keyboard::State state) {
  uint8_t mask = 0;
  if (state.keyDown(Shortcuts::k_prefixKey1)) {
    mask |= 1 << 0;
  }
  if (state.keyDown(Shortcuts::k_prefixKey2)) {
    mask |= 1 << 1;
  }
  const Page& page = k_pages[m_page];
  for (int i = 0; i < page.numberOfCombos; i++) {
    if (page.combos[i].withKey &&
        state.keyDown(Shortcuts::KeyForAction(page.combos[i].action))) {
      mask |= 1 << (2 + i);
    }
  }
  const uint8_t changed = mask ^ m_pressedMask;
  if (changed == 0) {
    return false;
  }
  m_pressedMask = mask;
  /* Only the images that moved are redrawn (not the whole screen): the dirty
   * rect covers both the normal and the lowered position. */
  for (int i = 0; i < page.numberOfCombos; i++) {
    const ComboGeometry g = comboGeometry(i);
    if (changed & (1 << 0)) {
      markRectAsDirty(KDRect(g.prefix1X, g.top, k_parenthesisWidth,
                             k_parenthesisHeight + k_pressedOffset));
    }
    if (changed & (1 << 1)) {
      markRectAsDirty(KDRect(g.prefix2X, g.top, k_parenthesisWidth,
                             k_parenthesisHeight + k_pressedOffset));
    }
    if (page.combos[i].withKey && (changed & (1 << (2 + i)))) {
      markRectAsDirty(KDRect(g.keyX,
                             g.top + (k_parenthesisHeight - k_keyHeight) / 2,
                             k_keyWidth, k_keyHeight + k_pressedOffset));
    }
  }
  return true;
}

void TutorialView::drawCentered(KDContext* ctx, const char* text, KDCoordinate y,
                                KDFont::Size font, KDColor color) const {
  KDCoordinate width = KDFont::Font(font)->stringSize(text).width();
  ctx->drawString(text, KDPoint((bounds().width() - width) / 2, y),
                  {.glyphColor = color, .backgroundColor = KDWhite, .font = font});
}

void TutorialView::drawImage(KDContext* ctx, KDRect rect,
                             const Escher::Image* image, KDCoordinate x,
                             KDCoordinate y) const {
  if (image == nullptr ||
      !KDRect(x, y, image->width(), image->height()).intersects(rect)) {
    return;
  }
  int numberOfPixels = image->width() * image->height();
  assert(numberOfPixels <= k_maxNumberOfPixels);
  OMG::Memory::Decompress(image->compressedPixelData(),
                          reinterpret_cast<uint8_t*>(m_pixels),
                          image->compressedPixelDataSize(),
                          numberOfPixels * sizeof(KDColor));
  ctx->fillRectWithPixels(KDRect(x, y, image->width(), image->height()),
                          m_pixels, nullptr);
}

TutorialView::ComboGeometry TutorialView::comboGeometry(int index) const {
  const Page& page = k_pages[m_page];
  const KDCoordinate largeHeight = KDFont::GlyphHeight(KDFont::Size::Large);
  const KDCoordinate smallHeight = KDFont::GlyphHeight(KDFont::Size::Small);
  const KDCoordinate plusWidth =
      KDFont::Font(KDFont::Size::Large)->stringSize("+").width();

  // Shortcuts are centered in the space left between the text and the footer
  const KDCoordinate zoneTop = k_margin + largeHeight + k_margin +
                               page.numberOfLines * (smallHeight + 2);
  const KDCoordinate footerY = bounds().height() - smallHeight - k_margin;
  const KDCoordinate zoneHeight = footerY - k_margin - zoneTop;
  const KDCoordinate comboHeight =
      k_parenthesisHeight + k_labelGap + largeHeight;
  const KDCoordinate combosHeight =
      page.numberOfCombos * comboHeight + (page.numberOfCombos - 1) * k_comboGap;

  ComboGeometry g;
  g.top = zoneTop + (zoneHeight - combosHeight) / 2 +
          index * (comboHeight + k_comboGap);
  const bool withKey = page.combos[index].withKey;
  KDCoordinate totalWidth = 2 * k_parenthesisWidth + k_imageGap;
  if (withKey) {
    totalWidth += 2 * k_plusMargin + plusWidth + k_keyWidth;
  }
  g.prefix1X = (bounds().width() - totalWidth) / 2;
  g.prefix2X = g.prefix1X + k_parenthesisWidth + k_imageGap;
  g.plusX = g.prefix2X + k_parenthesisWidth + k_plusMargin;
  g.keyX = g.plusX + plusWidth + k_plusMargin;
  return g;
}

void TutorialView::drawCombo(KDContext* ctx, KDRect rect, int index) const {
  const Combo& combo = k_pages[m_page].combos[index];
  const ComboGeometry g = comboGeometry(index);
  const KDCoordinate prefix1Offset =
      (m_pressedMask & (1 << 0)) ? k_pressedOffset : 0;
  const KDCoordinate prefix2Offset =
      (m_pressedMask & (1 << 1)) ? k_pressedOffset : 0;
  const KDCoordinate keyOffset =
      (m_pressedMask & (1 << (2 + index))) ? k_pressedOffset : 0;

  drawImage(ctx, rect, ImageForKey(Shortcuts::k_prefixKey1), g.prefix1X,
            g.top + prefix1Offset);
  drawImage(ctx, rect, ImageForKey(Shortcuts::k_prefixKey2), g.prefix2X,
            g.top + prefix2Offset);
  if (combo.withKey) {
    const KDFont::Size plusFont = KDFont::Size::Large;
    ctx->drawString("+",
                    KDPoint(g.plusX, g.top + (k_parenthesisHeight -
                                              KDFont::GlyphHeight(plusFont)) / 2),
                    {.glyphColor = k_titleColor,
                     .backgroundColor = KDWhite,
                     .font = plusFont});
    const Ion::Keyboard::Key key = Shortcuts::KeyForAction(combo.action);
    const Escher::Image* keyImage = ImageForKey(key);
    const KDCoordinate keyY =
        g.top + (k_parenthesisHeight - k_keyHeight) / 2 + keyOffset;
    if (keyImage != nullptr) {
      drawImage(ctx, rect, keyImage, g.keyX, keyY);
    } else {
      // A key without image (remapped shortcut)
      ctx->fillRect(KDRect(g.keyX, keyY, k_keyWidth, k_keyHeight),
                    KDColor::RGB24(0xC8C8C8));
      ctx->drawString("?", KDPoint(g.keyX + k_keyWidth / 2 - 5, keyY + 8),
                      {.glyphColor = k_titleColor,
                       .backgroundColor = KDColor::RGB24(0xC8C8C8),
                       .font = KDFont::Size::Large});
    }
  }
  drawCentered(ctx, I18n::translate(combo.label),
               g.top + k_parenthesisHeight + k_labelGap, KDFont::Size::Large,
               k_titleColor);
}

void TutorialView::drawRect(KDContext* ctx, KDRect rect) const {
  ctx->fillRect(bounds(), KDWhite);
  const Page& page = k_pages[m_page];
  const KDCoordinate largeHeight = KDFont::GlyphHeight(KDFont::Size::Large);
  const KDCoordinate smallHeight = KDFont::GlyphHeight(KDFont::Size::Small);

  // Title and text
  KDCoordinate y = k_margin;
  drawCentered(ctx, I18n::translate(page.title), y, KDFont::Size::Large,
               k_titleColor);
  y += largeHeight + k_margin;
  for (int i = 0; i < page.numberOfLines; i++) {
    drawCentered(ctx, I18n::translate(page.lines[i]), y, KDFont::Size::Small,
                 k_textColor);
    y += smallHeight + 2;
  }

  for (int i = 0; i < page.numberOfCombos; i++) {
    drawCombo(ctx, rect, i);
  }

  const KDCoordinate footerY = bounds().height() - smallHeight - k_margin;
  // Footer: back hint, page number and next hint
  const KDGlyph::Style hintStyle = {.glyphColor = k_hintColor,
                                    .backgroundColor = KDWhite,
                                    .font = KDFont::Size::Small};
  if (!isFirstPage()) {
    ctx->drawString(I18n::translate(I18n::Message::OnBoardingBack),
                    KDPoint(k_margin, footerY), hintStyle);
  }
  const char* next = I18n::translate(isLastPage()
                                         ? I18n::Message::OnBoardingStart
                                         : I18n::Message::OnBoardingNext);
  KDCoordinate nextWidth = KDFont::Font(KDFont::Size::Small)->stringSize(next).width();
  ctx->drawString(next,
                  KDPoint(bounds().width() - k_margin - nextWidth, footerY),
                  hintStyle);
  char counter[] = {static_cast<char>('1' + m_page), '/',
                    static_cast<char>('0' + k_numberOfPages), 0};
  drawCentered(ctx, counter, footerY, KDFont::Size::Small, k_hintColor);
}

}  // namespace OnBoarding
