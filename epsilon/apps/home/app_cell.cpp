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
    const KDColor* sourceRow = chunk + localY * wallpaperWidth + srcX;
    memcpy(&buffer[y * rectWidth], sourceRow, rectWidth * sizeof(KDColor));
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
  }
}

}  // namespace

namespace Home {

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

  // KDSize nameSize = textView()->minimalSizeForOptimalDisplay();
  // ctx->fillRect(
  //     KDRect(0, bounds().height() - nameSize.height() - 2 * k_nameHeightMargin,
  //            bounds().width(), nameSize.height() + 2 * k_nameHeightMargin),
  //     KDColorWhite);
    
  KDSize nameSize = textView()->minimalSizeForOptimalDisplay();

  KDRect nameRect = KDRect(0, bounds().height() - nameSize.height() - 2 * k_nameHeightMargin,
                           bounds().width(), nameSize.height() + 2 * k_nameHeightMargin);
  
  // Get the width and height of the rectangle
  int rectWidth = nameRect.width();
  int rectHeight = nameRect.height();
  int screenWidth = 320;
  
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
  // opaque background. Then avoid letting the TextView draw itself later.
  const_cast<TextView*>(textView())->drawTextTransparent(ctx, nameRect);

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
      ctx->fillRectWithPixels(iconRect, pixels, nullptr);
    }
  }
}

int AppCell::numberOfSubviews() const {
  if (!isVisible()) {
    return 0;
  }
  // When we're drawing the themed icon manually above, don't also let the
  // normal IconView subview draw the builtin icon on top of it.
  return hasThemedIcon() ? 0 : 1;
}

View* AppCell::subviewAtIndex(int index) {
  View* views[] = {&m_iconView, const_cast<TextView*>(textView())};
  return views[index];
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
  m_iconView.setImage(descriptor->icon());
  m_messageNameView.setMessage(descriptor->name());
  m_pointerNameView.setText(nullptr);
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
  // Do not set an opaque background so wallpaper remains visible.
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