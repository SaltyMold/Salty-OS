#include "app_cell.h"

#include <assert.h>
#include <apps/exam_mode_manager.h>
#include <escher/theme/color_picker.h>
#include <escher/theme/salty_palette.h>
#include <escher/transparent_themed_image_view.h>
#include <string.h>

#include "../theme_manager.h"


using namespace Escher;

namespace {

// Home screen colors: Salty theme palette when one is active, Epsilon's own
// (light / dark) theme otherwise.
KDColor wallpaperColor() {
  return SaltyPalette::IsActive()
             ? SaltyPalette::WallpaperColor()
             : ColorPicker::Resolve(ThemeComponent::DefaultBackground);
}

KDColor highlightBoxColor() {
  return ColorPicker::Resolve(ThemeComponent::SurfaceSelectApps);
}

void fillWallpaperLineRange(KDRect nameRect, int globalX, int globalY,
                            int nameHeight, KDColor* buffer, int rectHeight,
                            int rectWidth) {
  if (!ThemeManager::hasWallpaper() || ExamModeManager::ExamMode().isActive()) {
    // No Theme Area flashed, or the selected theme has no wallpaper: fall
    // back to a flat background instead of reading garbage.
    for (int i = 0; i < rectHeight * rectWidth; i++) {
      buffer[i] = wallpaperColor();
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
        buffer[y * rectWidth + x] = wallpaperColor();
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
      buffer[y * rectWidth + x] = wallpaperColor();
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
  if (!ThemeManager::hasWallpaper() || ExamModeManager::ExamMode().isActive()) {
    ctx->fillRect(bounds, wallpaperColor());
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
      ctx->fillRect(KDRect(0, y, bounds.width(), 1), wallpaperColor());
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
                    wallpaperColor());
    }
  }
}

// True when the app name band should be painted on an opaque background
// instead of blended transparently into the wallpaper.
//
// - No wallpaper to blend into at all: always opaque, regardless of
//   highlight state (nothing to show through anyway).
// - Wallpaper present and the theme's variables ask for an opaque
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

namespace {

/* Rounded icon mask ---------------------------------------------------------
 * The anti-aliased coverage of the circle mask only depends on the circle
 * level, never on the icon or on the wallpaper: it is computed once per level
 * (3x3 blur of k_circleMask) and kept, instead of being recomputed for each of
 * the 3080 pixels of every icon at every redraw.
 *
 * For each row we also keep the longest run of fully opaque pixels
 * (alpha == 255): it is copied straight from the icon with a memcpy, and only
 * the few pixels left of / right of it (the rounded corners) need the
 * wallpaper behind them. */
struct IconMask {
  bool built;  // false (zero-initialized, so no .data) until first use
  uint8_t level;
  bool fullyOpaque;  // every pixel is opaque (square icon): nothing to blend
  uint8_t opaqueStart[k_circleMaskHeight];  // [opaqueStart, opaqueEnd) of
  uint8_t opaqueEnd[k_circleMaskHeight];    // alpha == 255 in each row
  uint8_t alpha[k_circleMaskHeight][k_circleMaskWidth];
};
IconMask s_iconMask;

// Smooth coverage (0..255) of the circle mask at (x, y)
uint8_t circleCoverage(int x, int y, int width, int height, uint8_t bitMask) {
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

const IconMask& iconMask(uint8_t level) {
  IconMask& m = s_iconMask;
  if (m.built && m.level == level) {
    return m;
  }
  const uint8_t bitMask = 1 << level;
  bool allOpaque = true;
  for (int y = 0; y < k_circleMaskHeight; y++) {
    int bestStart = 0, bestLength = 0, runStart = 0, runLength = 0;
    for (int x = 0; x < k_circleMaskWidth; x++) {
      // Outside the mask the alpha is 0 (plain wallpaper). Inside the mask,
      // only the contour pixels (those with an outside neighbour) are
      // partially transparent.
      uint8_t a = (k_circleMask[y][x] & bitMask)
                      ? circleCoverage(x, y, k_circleMaskWidth,
                                       k_circleMaskHeight, bitMask)
                      : 0;
      m.alpha[y][x] = a;
      if (a == 0xFF) {
        if (runLength == 0) {
          runStart = x;
        }
        runLength++;
        if (runLength > bestLength) {
          bestLength = runLength;
          bestStart = runStart;
        }
      } else {
        runLength = 0;
        allOpaque = false;
      }
    }
    // No opaque pixel: start == end == 0, the whole row is "edge".
    m.opaqueStart[y] = bestLength ? bestStart : 0;
    m.opaqueEnd[y] = bestLength ? bestStart + bestLength : 0;
  }
  m.fullyOpaque = allOpaque;
  m.level = level;
  m.built = true;
  return m;
}

}  // namespace

/* Draws the part of a rounded icon that lies in `clip` (a sub-rect of
 * iconRect, both in cell coordinates), over the wallpaper.
 *
 * `pixels` are the iconRect.width() x iconRect.height() pixels of the icon: they
 * can be read-only and straight from flash, they are never modified. The result
 * is assembled row by row in one buffer and pushed to the screen with a single
 * fillRectWithPixels call. Only what is actually visible (clip) is computed, and
 * the wallpaper is only looked up for the rows that have rounded-corner
 * pixels, once per row (not once per pixel). */
void fillRoundedIcon(KDContext* ctx, KDRect iconRect, KDRect clip,
                     const KDColor* pixels, int globalX, int globalY,
                     int nameHeight) {
  const int iconWidth = iconRect.width();
  assert(iconWidth == k_circleMaskWidth &&
         iconRect.height() == k_circleMaskHeight);
  clip = clip.intersectedWith(iconRect);
  if (clip.isEmpty()) {
    return;
  }

  uint8_t level = ThemeManager::isValid() ? (uint8_t)ThemeManager::defaultCircleLevel()
                                          : circleLevel;
  const IconMask& mask = iconMask(level);
  if (mask.fullyOpaque && clip == iconRect) {
    // Square icon, fully visible: the flash content goes to the screen as is.
    ctx->fillRectWithPixels(iconRect, pixels, nullptr);
    return;
  }

  const int x0 = clip.x() - iconRect.x();
  const int y0 = clip.y() - iconRect.y();
  const int w = clip.width();
  const int h = clip.height();
  const int x1 = x0 + w;

  const bool useWallpaper =
      ThemeManager::hasWallpaper() && !ExamModeManager::ExamMode().isActive();
  const int wallpaperWidth = useWallpaper ? ThemeManager::wallpaperWidth() : 0;
  const KDColor flatColor = wallpaperColor();

  KDColor out[k_circleMaskWidth * k_circleMaskHeight];
  for (int y = y0; y < y0 + h; y++) {
    const KDColor* src = pixels + y * iconWidth;
    KDColor* dst = out + (y - y0) * w;  // dst[x - x0] is the icon's column x

    // Opaque middle of the row: plain copy.
    int a = mask.opaqueStart[y] > x0 ? mask.opaqueStart[y] : x0;
    int b = mask.opaqueEnd[y] < x1 ? mask.opaqueEnd[y] : x1;
    if (a < b) {
      memcpy(dst + (a - x0), src + a, (b - a) * sizeof(KDColor));
    } else {
      a = b = x1;  // nothing opaque in this range: all of it is "edge"
    }
    if (a == x0 && b == x1) {
      continue;  // all opaque: no wallpaper needed for this row
    }

    // Rounded corners: [x0, a) and [b, x1). The wallpaper row behind the icon
    // is fetched once for the row.
    const KDColor* wallpaperRow = nullptr;
    if (useWallpaper) {
      int localY = 0;
      const KDColor* chunk = ThemeManager::wallpaperChunkContaining(
          globalY + iconRect.y() + y - nameHeight - 4, &localY);
      if (chunk != nullptr) {
        wallpaperRow = chunk + localY * wallpaperWidth;
      }
    }
    const int rowOriginX = globalX + iconRect.x();
    auto cornerPixel = [&](int x) {
      const uint8_t alpha = mask.alpha[y][x];
      if (alpha == 0xFF) {
        dst[x - x0] = src[x];
        return;
      }
      const int wx = rowOriginX + x;
      const KDColor background =
          (wallpaperRow != nullptr && wx >= 0 && wx < wallpaperWidth)
              ? wallpaperRow[wx]
              : flatColor;
      dst[x - x0] = alpha == 0 ? background
                               : KDColor::Blend(src[x], background, alpha);
    };
    for (int x = x0; x < a; x++) {
      cornerPixel(x);
    }
    for (int x = b; x < x1; x++) {
      cornerPixel(x);
    }
  }
  ctx->fillRectWithPixels(clip, out, nullptr);
}

AppCell::AppCell()
    : HighlightCell(),
      m_messageNameView((I18n::Message)0, k_glyphsFormat),
      m_image(0, 0, nullptr, 0),
      m_pointerNameView(nullptr, k_glyphsFormat) {
  // Same colors as in reloadCell so the names are correct on first display
  applyNameColors();
}


bool AppCell::hasThemedIcon() const {
  return m_themeIconIndex >= 0 && ThemeManager::hasIcon(m_themeIconIndex);
}

bool AppCell::shouldRoundCustomIcon() const {
  return ThemeManager::hasWallpaper() && (ThemeManager::isValid() || m_pointerNameView.text() != nullptr) && !ExamModeManager::ExamMode().isActive();
}

KDRect AppCell::iconRect() const {
  return KDRect((bounds().width() - k_iconWidth) / 2, k_iconMargin,
                k_iconWidth, k_iconHeight);
}

KDRect AppCell::nameRect() const {
  KDSize nameSize = textView()->minimalSizeForOptimalDisplay();
  return KDRect(0, bounds().height() - nameSize.height() - 2 * k_nameHeightMargin,
                bounds().width(), nameSize.height() + 2 * k_nameHeightMargin);
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

  // `rect` is the part of the cell that actually needs to be redrawn: do not
  // spend time on what is outside of it (for instance the icon, when only the
  // name changed).
  if (rect.intersects(nameRect)) {
    drawNameBand(ctx, nameRect, nameSize.height());
  }

  KDRect icon = iconRect();
  if (!rect.intersects(icon)) {
    return;
  }

  /* Themed icon: RGB565 pixels given by ThemeManager::iconPixels, either read
   * straight from flash (raw icons) or decompressed into its shared buffer
   * (LZ4 icons): fillRoundedIcon only reads them, whatever the format. When
   * present, this replaces the normal IconView draw (excluded from
   * numberOfSubviews() below) so we don't draw both on top of each other. */
  if (hasThemedIcon()) {
    const KDColor* pixels =
        ThemeManager::iconPixels(m_themeIconIndex, k_iconWidth, k_iconHeight);
    if (pixels != nullptr) {
      KDPoint globalOrigin = ctx->origin();
      fillRoundedIcon(ctx, icon, rect, pixels, globalOrigin.x(),
                      globalOrigin.y(), nameSize.height());
    }
  } else if (ThemeManager::hasWallpaper() && ThemeManager::isValid() &&
             iconImage() != nullptr && iconImage()->width() > 0) {
    drawCompressedIcon(ctx, rect, icon, iconImage()->compressedPixelData(),
                       iconImage()->compressedPixelDataSize(),
                       nameSize.height());
  } else if (m_pointerNameView.text() != nullptr && m_image.width() > 0 &&
             ThemeManager::hasWallpaper() && !ThemeManager::isValid()) {
    drawCompressedIcon(ctx, rect, icon, m_image.compressedPixelData(),
                       m_image.compressedPixelDataSize(), nameSize.height());
  }
}

/* Icons that are not part of the Theme Area (the builtin Escher ones when the
 * theme has no icon for them, and the external apps' ones) are compiled in /
 * stored by Escher in its own compressed format: they still have to be
 * decompressed before being rounded. */
void AppCell::drawCompressedIcon(KDContext* ctx, KDRect rect, KDRect icon,
                                 const uint8_t* compressed, int compressedSize,
                                 int nameHeight) const {
  KDColor pixels[k_iconWidth * k_iconHeight];
  OMG::Memory::Decompress(compressed, reinterpret_cast<uint8_t*>(pixels),
                          compressedSize,
                          k_iconWidth * k_iconHeight * sizeof(KDColor));
  KDPoint globalOrigin = ctx->origin();
  fillRoundedIcon(ctx, icon, rect, pixels, globalOrigin.x(), globalOrigin.y(),
                  nameHeight);
}

void AppCell::drawNameBand(KDContext* ctx, KDRect nameRect,
                           int nameHeight) const {
  int rectWidth = nameRect.width();
  int rectHeight = nameRect.height();
  KDColor buffer[104 * 20];
  KDPoint globalOrigin = ctx->origin();

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
      // Clear the band with the real wallpaper pixels first (rather than a
      // flat color), so the area around the TextView's own, narrower opaque
      // box still shows the photo instead of a plain rectangle.
      fillWallpaperLineRange(nameRect, globalOrigin.x(), globalOrigin.y(),
                             nameHeight, buffer, rectHeight, rectWidth);
      ctx->fillRectWithPixels(nameRect, buffer, nullptr);
    } else {
      ctx->fillRect(nameRect, wallpaperColor());
    }
    return;
  }

  // Not the highlighted cell (or the theme doesn't ask for an opaque
  // highlight box at all): blend the name into the real wallpaper pixels
  // instead of painting any flat rectangle behind it.
  fillWallpaperLineRange(nameRect, globalOrigin.x(), globalOrigin.y(),
                         nameHeight, buffer, rectHeight, rectWidth);
  ctx->fillRectWithPixels(nameRect, buffer, nullptr);
  // Draw the text transparently on top of the wallpaper so we don't fill an
  // opaque background. Then avoid letting the TextView draw itself later
  // (excluded from subviews below).
  const_cast<TextView*>(textView())->drawTextTransparent(ctx, nameRect);
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
       iconImage() != nullptr);
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
       iconImage() != nullptr);
  bool includeIcon = !manuallyDrawIcon;
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
  m_iconView.setThemedImage(descriptor->icon());
  m_messageNameView.setMessage(descriptor->name());
  m_pointerNameView.setText(nullptr);
  m_iconView.setThemedImage(descriptor->icon());
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
  m_iconView.setSingleImage(&m_image);
  m_themeIconIndex = -1;
  layoutSubviews();
}

void AppCell::setVisible(bool visible) {
  if (isVisible() != visible) {
    Escher::HighlightCell::setVisible(visible);
    markWholeFrameAsDirty();
  }
}

void AppCell::setHighlighted(bool highlight) {
  if (isHighlighted() == highlight) {
    return;
  }
  setHighlightedWithoutReload(highlight);
  applyNameColors();
  /* The highlight only changes the colors and the box of the name: the icon
   * does not depend on it, so only the name band is redrawn. Moving the
   * selection no longer recomposites two icons over the wallpaper. */
  markRectAsDirty(nameRect());
}

void AppCell::reloadCell() {
  applyNameColors();
  markWholeFrameAsDirty();
}

void AppCell::applyNameColors() {
  TextView* views[] = {&m_messageNameView, &m_pointerNameView};
  const bool highlighted = isHighlighted();
  for (TextView* t : views) {
    if (SaltyPalette::IsActive()) {
      t->setCustomColors(highlighted ? SaltyPalette::TextColor()
                                     : SaltyPalette::TextColorHover(),
                         highlighted ? highlightBoxColor() : wallpaperColor());
    } else {
      t->clearCustomColors();
      t->setTextColor(highlighted ? ThemeComponent::TextWhite
                                  : ThemeComponent::TextDefault);
      t->setBackgroundColor(highlighted ? ThemeComponent::SurfaceSelectApps
                                        : ThemeComponent::DefaultCellBackground);
    }
  }
}

const Escher::TextView* AppCell::textView() const {
  if (m_pointerNameView.text()) {
    return &m_pointerNameView;
  } else {
    return &m_messageNameView;
  }
}

}  // namespace Home