#include "app_cell.h"

#include <assert.h>

#include "home_wallpaper.h"
#include <omg/memory.h>


using namespace Escher;

namespace Home {

AppCell::AppCell()
    : HighlightCell(),
      m_messageNameView((I18n::Message)0, k_glyphsFormat),
      m_image(0, 0, nullptr, 0),
      m_pointerNameView(nullptr, k_glyphsFormat) {}

void AppCell::drawRect(KDContext* ctx, KDRect rect) const {
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

  // Decompress wallpaper on first use into a static buffer
  static KDColor wallpaperPixels[320 * 240];
  static bool wallpaperInitialized = false;
  if (!wallpaperInitialized) {
    OMG::Memory::Decompress(
        Ion::HomeWallpaper::compressedPixelData,
        reinterpret_cast<uint8_t*>(wallpaperPixels),
        Ion::HomeWallpaper::k_compressedPixelSize,
        Ion::HomeWallpaper::k_width * Ion::HomeWallpaper::k_height * sizeof(KDColor));
    wallpaperInitialized = true;
  }
  
  // Get position 
  KDPoint globalOrigin = ctx->origin();
  int globalX = globalOrigin.x();
  int globalY = globalOrigin.y();
  
  // Copy line by line
  for (int y = 0; y < rectHeight; y++) {
    int srcOffset = (globalY + nameRect.origin().y() + y - nameSize.height() - 4) * screenWidth + (globalX + nameRect.origin().x());
        memcpy(&buffer[y * rectWidth], &wallpaperPixels[srcOffset],
          rectWidth * sizeof(KDColor));
  }
  
  ctx->fillRectWithPixels(nameRect, buffer, nullptr);
}

int AppCell::numberOfSubviews() const { return isVisible() ? 2 : 0; }

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
  layoutSubviews();
}

void AppCell::setExternalApp(Ion::ExternalApps::App app) {
  m_pointerNameView.setText(app.name());
  m_messageNameView.setMessage((I18n::Message)0);
  m_image = Image(k_iconWidth, k_iconHeight, app.iconData(), app.iconSize());
  m_iconView.setImage(&m_image);
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
  t->setTextColor(isHighlighted() ? KDColorWhite : KDColorBlack);
  t->setBackgroundColor(isHighlighted() ? Palette::YellowDark : KDColorWhite);
}

const Escher::TextView* AppCell::textView() const {
  if (m_pointerNameView.text()) {
    return &m_pointerNameView;
  } else {
    return &m_messageNameView;
  }
}

}  // namespace Home
