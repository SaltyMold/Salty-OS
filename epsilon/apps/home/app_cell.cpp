#include "app_cell.h"

#include <assert.h>
#include <string.h>

#include "../theme_manager.h" 


using namespace Escher;

namespace {

void fillWallpaperLineRange(KDRect nameRect, int globalX, int globalY,
                            int nameHeight, KDColor* buffer, int rectHeight,
                            int rectWidth) {
  if (!ThemeManager::hasWallpaper()) {
    // No Theme Area flashed, or the selected theme has no wallpaper: fall
    // back to a flat background instead of reading garbage.
    for (int i = 0; i < rectHeight * rectWidth; i++) {
      buffer[i] = Palette::WallpaperColor;
    }
    return;
  }

  int wallpaperWidth = ThemeManager::wallpaperWidth();

  for (int y = 0; y < rectHeight; y++) {
    int srcY = globalY + nameRect.origin().y() + y - nameHeight - 4;
    int localY = 0;
    const KDColor* chunk = ThemeManager::wallpaperChunkContaining(srcY, &localY);
    if (chunk == nullptr) {
      for (int x = 0; x < rectWidth; x++) {
        buffer[y * rectWidth + x] = Palette::WallpaperColor;
      }
      continue;
    }

    int srcX = globalX + nameRect.origin().x();
    // Clamp to what the wallpaper actually has on this row: srcX + rectWidth
    // can run past wallpaperWidth (narrower wallpaper, or a cell near the
    // right edge), and reading past it would walk off the end of the
    // (shared, fixed-size) chunk buffer.
    int available = wallpaperWidth - srcX;
    int copyWidth = available < rectWidth ? available : rectWidth;
    if (copyWidth < 0) {
      copyWidth = 0;
    }
    const KDColor* sourceRow = chunk + localY * wallpaperWidth + srcX;
    memcpy(&buffer[y * rectWidth], sourceRow, copyWidth * sizeof(KDColor));
    for (int x = copyWidth; x < rectWidth; x++) {
      buffer[y * rectWidth + x] = Palette::WallpaperColor;
    }
  }
}

// Fills the whole given bounds with the plain wallpaper (no text, no icon).
// Used for hidden cells: unlike subview drawing (numberOfSubviews()),
// drawRect isn't gated by isVisible() automatically - a hidden cell's
// drawRect still gets called whenever it's marked dirty (which
// AppCell::setVisible(false) itself does), so it must actively repaint
// itself as blank, or it keeps showing whatever it last displayed.
void fillWholeCellWallpaper(KDContext* ctx, KDRect bounds, int globalX,
                            int globalY, int nameHeight) {
  if (!ThemeManager::hasWallpaper()) {
    ctx->fillRect(bounds, Palette::WallpaperColor);
    return;
  }

  int wallpaperWidth = ThemeManager::wallpaperWidth();
  int lineWidth = bounds.width() < wallpaperWidth - globalX
                      ? bounds.width()
                      : wallpaperWidth - globalX;
  if (lineWidth < 0) {
    lineWidth = 0;
  }

  KDColor lineBuffer[320];
  for (int y = 0; y < bounds.height(); y++) {
    int localY = 0;
    // Same vertical calibration offset as fillWallpaperLineRange, applied
    // here to the whole cell instead of just the name band, so a cell that
    // toggles between visible/hidden samples the wallpaper at the same
    // position either way (no seam/tearing on redraw).
    int srcY = globalY + y - nameHeight - 4;
    const KDColor* chunk =
        ThemeManager::wallpaperChunkContaining(srcY, &localY);
    if (chunk == nullptr) {
      ctx->fillRect(KDRect(0, y, bounds.width(), 1), Palette::WallpaperColor);
      continue;
    }
    const KDColor* sourceRow = chunk + localY * wallpaperWidth + globalX;
    memcpy(lineBuffer, sourceRow, lineWidth * sizeof(KDColor));
    ctx->fillRectWithPixels(KDRect(0, y, lineWidth, 1), lineBuffer, nullptr);
    if (lineWidth < bounds.width()) {
      // Wallpaper doesn't reach this far right (narrower image, or this
      // cell sits past its edge): paint the leftover strip flat instead of
      // leaving it unpainted.
      ctx->fillRect(KDRect(lineWidth, y, bounds.width() - lineWidth, 1),
                    Palette::WallpaperColor);
    }
  }
}

// True when the app name band should be painted on an opaque background
// instead of blended transparently into the wallpaper.
//
// - No wallpaper to blend into at all: always opaque, regardless of
//   highlight state (nothing to show through anyway).
// - Wallpaper present and the theme's variables.txt asks for an opaque
//   name background on highlight (isAppNameBackgroundHover): opaque ONLY
//   for the currently highlighted cell (the solid YellowDark selection
//   box). Every other, non-highlighted cell still blends into the real
//   wallpaper pixels instead of showing a flat color box.
bool useOpaqueNameBackground(bool cellIsHighlighted) {
  return !ThemeManager::hasWallpaper() ||
         (ThemeManager::isAppNameBackgroundHover() && cellIsHighlighted);
}

}  // namespace

namespace Home {

uint8_t circleLevel = 1;
// Smooth coverage (0..255) of the circle mask at (x, y)
static uint8_t circleCoverage(int x, int y, int width, int height,
                              uint8_t bitMask) {
  static constexpr uint8_t k_weights[3][3] = {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}};
  int sum = 0;
  for (int dy = -1; dy <= 1; dy++) {
    int ny = y + dy;
    ny = ny < 0 ? 0 : (ny >= height ? height - 1 : ny);
    for (int dx = -1; dx <= 1; dx++) {
      int nx = x + dx;
      nx = nx < 0 ? 0 : (nx >= width ? width - 1 : nx);
      if (k_circleMask[ny][nx] & bitMask) {
        sum += k_weights[dy + 1][dx + 1];
      }
    }
  }
  return (sum * 255) / 16;
}

static KDColor wallpaperPixelAt(int srcX, int srcY, int wallpaperWidth) {
  if (ThemeManager::hasWallpaper()) {
    int localY = 0;
    const KDColor* chunk = ThemeManager::wallpaperChunkContaining(srcY, &localY);
    if (chunk != nullptr && srcX >= 0 && srcX < wallpaperWidth) {
      return chunk[localY * wallpaperWidth + srcX];
    }
  }
  return Palette::WallpaperColor;
}

void fillRoundedIcon(KDContext* ctx, KDRect iconRect, const KDColor* pixels,
                     int iconWidth, int iconHeight, int globalX, int globalY,
                     int nameHeight) {
  uint8_t level = ThemeManager::isValid() ? (uint8_t)ThemeManager::defaultCircleLevel()
                                          : circleLevel;
  uint8_t bitMask = 1 << level;
  KDColor maskedPixels[55 * 56];

  int wallpaperWidth = ThemeManager::wallpaperWidth();
  for (int y = 0; y < iconHeight; y++) {
    for (int x = 0; x < iconWidth; x++) {
      int idx = y * iconWidth + x;
      bool insideMask = (k_circleMask[y][x] & bitMask) != 0;
      uint8_t alpha = 0;
      if (insideMask) {
        alpha = circleCoverage(x, y, iconWidth, iconHeight, bitMask);
        if (alpha == 0xFF) {
          maskedPixels[idx] = pixels[idx];
          continue;
        }
      }
      // Outside the mask: alpha stays 0, so we get the plain wallpaper
      // (same as the original hard mask). Inside the mask, only the contour
      // pixels of the icon (those with an outside neighbour) are blended.
      int srcX = globalX + iconRect.origin().x() + x;
      int srcY = globalY + iconRect.origin().y() + y - nameHeight - 4;
      KDColor background = wallpaperPixelAt(srcX, srcY, wallpaperWidth);
      maskedPixels[idx] = alpha == 0 ? background
                                     : KDColor::Blend(pixels[idx], background, alpha);
    }
  }
  ctx->fillRectWithPixels(iconRect, maskedPixels, nullptr);
}

AppCell::AppCell()
    : HighlightCell(),
      m_messageNameView((I18n::Message)0, k_glyphsFormat),
      m_image(0, 0, nullptr, 0),
      m_pointerNameView(nullptr, k_glyphsFormat) {
  // Initialize text color to the same default used in reloadCell so the
  // names are correctly colored on first display.
  KDColor initialColor = isHighlighted() ? Palette::TextColor : Palette::TextColorHover;
  m_messageNameView.setTextColor(initialColor);
  m_pointerNameView.setTextColor(initialColor);
}


bool AppCell::hasThemedIcon() const {
  return m_themeIconIndex >= 0 && ThemeManager::hasIcon(m_themeIconIndex);
}

bool AppCell::shouldRoundCustomIcon() const {
  return ThemeManager::hasWallpaper() &&
         (ThemeManager::isValid() || m_pointerNameView.text() != nullptr);
}

void AppCell::drawRect(KDContext* ctx, KDRect rect) const {
  if (!isVisible()) {
    // See fillWholeCellWallpaper's comment: drawRect has no automatic
    // isVisible() gate, so a hidden cell must actively blank itself out
    // instead of leaving whatever it last displayed on screen.
    KDPoint globalOrigin = ctx->origin();
    int nameHeight = textView()->minimalSizeForOptimalDisplay().height();
    fillWholeCellWallpaper(ctx, bounds(), globalOrigin.x(), globalOrigin.y(),
                           nameHeight);
    return;
  }

  KDSize nameSize = textView()->minimalSizeForOptimalDisplay();

  KDRect nameRect = KDRect(0, bounds().height() - nameSize.height() - 2 * k_nameHeightMargin,
                           bounds().width(), nameSize.height() + 2 * k_nameHeightMargin);

  if (useOpaqueNameBackground(isHighlighted())) {
    // Either there's no wallpaper at all, or the theme asks for an opaque
    // highlight box (isAppNameBackgroundHover) AND this cell is currently
    // the highlighted one. Either way the name itself is drawn by the
    // TextView as a normal (opaque) subview - see
    // numberOfSubviews()/subviewAtIndex() below - so it stays perfectly
    // readable. But we still need to clear the *whole* nameRect band (full
    // cell width, not just the width of the text) every time we draw, or
    // leftovers from a previous, wider TextView content / themed icon can
    // remain visible at the edges.
    if (ThemeManager::hasWallpaper()) {
      // This only happens for the highlighted cell of a theme that asks
      // for an opaque name background: clear the band with the real
      // wallpaper pixels first (rather than a flat color), so the area
      // around the TextView's own, narrower opaque YellowDark box still
      // shows the photo instead of a plain rectangle.
      int rectWidth = nameRect.width();
      int rectHeight = nameRect.height();

      KDColor buffer[104 * 20];

      KDPoint globalOrigin = ctx->origin();
      fillWallpaperLineRange(nameRect, globalOrigin.x(), globalOrigin.y(),
                             nameSize.height(), buffer, rectHeight, rectWidth);

      ctx->fillRectWithPixels(nameRect, buffer, nullptr);
    } else {
      // No wallpaper at all: keep the previous, simpler behavior of a flat
      // background fill.
      ctx->fillRect(nameRect, Palette::WallpaperColor);
    }
  } else {
    // Not the highlighted cell (or the theme doesn't ask for an opaque
    // highlight box at all): blend the name into the real wallpaper pixels
    // instead of painting any flat rectangle behind it.
    // Get the width and height of the rectangle
    int rectWidth = nameRect.width();
    int rectHeight = nameRect.height();

    // Max temp buffer for copying
    KDColor buffer[104 * 20];

    // Get position
    KDPoint globalOrigin = ctx->origin();
    int globalX = globalOrigin.x();
    int globalY = globalOrigin.y();

    fillWallpaperLineRange(nameRect, globalX, globalY, nameSize.height(), buffer,
                           rectHeight, rectWidth);

    ctx->fillRectWithPixels(nameRect, buffer, nullptr);
    // Draw the text transparently on top of the wallpaper so we don't fill an
    // opaque background. Then avoid letting the TextView draw itself later
    // (excluded from subviews below).
    const_cast<TextView*>(textView())->drawTextTransparent(ctx, nameRect);
  }

  // Themed icon: decompressed manually (see ThemeManager::iconPixels), since
  // we don't know the codec Escher::Image/IconView expect for
  // compressedPixelData() and can't safely hand our LZ4 blob to it. When
  // present, this replaces the normal IconView draw (excluded from
  // numberOfSubviews() below) so we don't draw both on top of each other.
  if (hasThemedIcon()) {
    KDRect iconRect((bounds().width() - k_iconWidth) / 2, k_iconMargin,
                    k_iconWidth, k_iconHeight);
    const KDColor* pixels =
        ThemeManager::iconPixels(m_themeIconIndex, k_iconWidth, k_iconHeight);
    if (pixels != nullptr) {
      KDPoint globalOrigin = ctx->origin();
      fillRoundedIcon(ctx, iconRect, pixels, k_iconWidth, k_iconHeight,
                      globalOrigin.x(), globalOrigin.y(), nameSize.height());
    }
  } else if (ThemeManager::hasWallpaper() && ThemeManager::isValid() &&
             m_iconImage != nullptr && m_iconImage->width() > 0) {
    KDRect iconRect((bounds().width() - k_iconWidth) / 2, k_iconMargin,
                    k_iconWidth, k_iconHeight);
    KDColor pixels[k_iconWidth * k_iconHeight];
    OMG::Memory::Decompress(m_iconImage->compressedPixelData(),
                            reinterpret_cast<uint8_t*>(pixels),
                            m_iconImage->compressedPixelDataSize(),
                            k_iconWidth * k_iconHeight * sizeof(KDColor));
    KDPoint globalOrigin = ctx->origin();
    fillRoundedIcon(ctx, iconRect, pixels, k_iconWidth, k_iconHeight,
                    globalOrigin.x(), globalOrigin.y(), nameSize.height());
  }

  if (m_pointerNameView.text() != nullptr && m_image.width() > 0 &&
      ThemeManager::hasWallpaper() && !ThemeManager::isValid()) {
    KDRect iconRect((bounds().width() - k_iconWidth) / 2, k_iconMargin,
                    k_iconWidth, k_iconHeight);
    KDColor pixels[k_iconWidth * k_iconHeight];
    OMG::Memory::Decompress(m_image.compressedPixelData(),
                            reinterpret_cast<uint8_t*>(pixels),
                            m_image.compressedPixelDataSize(),
                            k_iconWidth * k_iconHeight * sizeof(KDColor));
    KDPoint globalOrigin = ctx->origin();
    fillRoundedIcon(ctx, iconRect, pixels, k_iconWidth, k_iconHeight,
                    globalOrigin.x(), globalOrigin.y(), nameSize.height());
  }
}

int AppCell::numberOfSubviews() const {
  if (!isVisible()) {
    return 0;
  }
  // When we're drawing the themed icon manually above, don't also let the
  // normal IconView subview draw the builtin icon on top of it. External
  // apps have no themed-icon slot of their own (see setExternalApp()), so
  // they only need this manual/rounded treatment when there's a wallpaper
  // to blend into - with no wallpaper (e.g. the "Default" theme) they fall
  // back to the same plain square iconView as an untheme builtin app.
  bool manuallyDrawIcon =
      hasThemedIcon() ||
      (m_pointerNameView.text() != nullptr && ThemeManager::hasWallpaper()) ||
      (ThemeManager::hasWallpaper() && ThemeManager::isValid() &&
       m_iconImage != nullptr);
  int count = manuallyDrawIcon ? 0 : 1;
  // The name is only drawn manually (drawTextTransparent, see drawRect())
  // when there's a wallpaper to blend with, and either the theme doesn't
  // ask for an opaque name background at all, or it does but this cell
  // isn't the currently highlighted one. Otherwise we fall back to the
  // previous behavior: the TextView draws itself normally, as an ordinary
  // opaque subview.
  if (useOpaqueNameBackground(isHighlighted())) {
    count += 1;
  }
  return count;
}

View* AppCell::subviewAtIndex(int index) {
  bool manuallyDrawIcon =
      hasThemedIcon() ||
      (m_pointerNameView.text() != nullptr && ThemeManager::hasWallpaper()) ||
      (ThemeManager::hasWallpaper() && ThemeManager::isValid() &&
       m_iconImage != nullptr);
  bool includeIcon = !manuallyDrawIcon;
  bool includeText = useOpaqueNameBackground(isHighlighted());
  if (includeIcon) {
    if (index == 0) {
      return &m_iconView;
    }
    index--;
  }
  assert(includeText && index == 0);
  return const_cast<TextView*>(textView());
}

void AppCell::layoutSubviews(bool force) {
  setChildFrame(&m_iconView,
                KDRect((bounds().width() - k_iconWidth) / 2, k_iconMargin,
                       k_iconWidth, k_iconHeight),
                force);
  KDSize nameSize = textView()->minimalSizeForOptimalDisplay();
  setChildFrame(
      const_cast<TextView*>(textView()),
      KDRect((bounds().width() - nameSize.width()) / 2 - k_nameWidthMargin,
             bounds().height() - nameSize.height() - 2 * k_nameHeightMargin,
             nameSize.width() + 2 * k_nameWidthMargin,
             nameSize.height() + 2 * k_nameHeightMargin),
      force);
}

void AppCell::setBuiltinAppDescriptor(const ::App::Descriptor* descriptor) {
  m_messageNameView.setMessage(descriptor->name());
  m_pointerNameView.setText(nullptr);
  m_iconImage = descriptor->icon();
  m_iconView.setImage(m_iconImage);
  // Only builtin apps can have a themed icon: the theme's fixed icon set
  // (calculation_icon.png, code_icon.png, ...) has no slot for arbitrary
  // external/third-party apps.
  m_themeIconIndex = ThemeManager::iconIndexForApp(descriptor->name());
  layoutSubviews();
}

void AppCell::setExternalApp(Ion::ExternalApps::App app) {
  m_pointerNameView.setText(app.name());
  m_messageNameView.setMessage((I18n::Message)0);
  m_image = Image(k_iconWidth, k_iconHeight, app.iconData(), app.iconSize());
  m_iconImage = &m_image;
  m_iconView.setImage(&m_image);
  m_themeIconIndex = -1;
  layoutSubviews();
}

void AppCell::setVisible(bool visible) {
  if (isVisible() != visible) {
    Escher::HighlightCell::setVisible(visible);
    markWholeFrameAsDirty();
  }
}

void AppCell::reloadCell() {
  TextView* t = const_cast<TextView*>(textView());
  t->setTextColor(isHighlighted() ? Palette::TextColor : Palette::TextColorHover);
  if (useOpaqueNameBackground(isHighlighted())) {
    // Opaque name background: either there's no wallpaper at all, or this
    // is the highlighted cell of a theme that asks for an opaque highlight
    // box. In the wallpaper case isHighlighted() is necessarily true here
    // (see useOpaqueNameBackground()), so this always resolves to
    // YellowDark; in the no-wallpaper case both branches still apply as
    // before. The TextView draws itself as a normal opaque subview (see
    // numberOfSubviews()/subviewAtIndex()), so give it an opaque
    // background again, same spirit as the pre-wallpaper code.
    t->setBackgroundColor(isHighlighted() ? Palette::YellowDark
                                          : Palette::WallpaperColor);
  }
  // Otherwise (wallpaper present, and either the theme doesn't ask for an
  // opaque highlight box, or this cell isn't the highlighted one), the name
  // is painted manually via drawTextTransparent() in drawRect(), which
  // ignores the TextView's own background - leave it alone so nothing
  // opaque gets set on it.
  markWholeFrameAsDirty();
}

const Escher::TextView* AppCell::textView() const {
  if (m_pointerNameView.text()) {
    return &m_pointerNameView;
  } else {
    return &m_messageNameView;
  }
}

}  // namespace Home