#include "theme_manager.h"

#include <escher/palette.h>
#include <omg/memory.h>
#include <string.h>

using namespace Ion::Device::Board::Config;

const ThemeAreaHeader* ThemeManager::s_header = nullptr;
bool ThemeManager::s_valid = false;
int ThemeManager::s_selectedTheme = 0;

void ThemeManager::init() {
  const ThemeAreaHeader* h =
      reinterpret_cast<const ThemeAreaHeader*>(ThemeAreaStart);

  if (h->magic != ThemeAreaHeaderMagic ||
      h->version != ThemeAreaHeaderVersion ||
      h->themeCount > ThemeCount) {
    s_valid = false;
    return;
  }

  s_header = h;
  s_valid = true;
  s_selectedTheme = 0;
}

const ThemeEntry* ThemeManager::entry(int index) {
  if (!s_valid || index < 0 || index >= (int)s_header->themeCount) {
    return nullptr;
  }
  const uint8_t* base =
      reinterpret_cast<const uint8_t*>(ThemeAreaStart) + sizeof(ThemeAreaHeader);
  return reinterpret_cast<const ThemeEntry*>(base) + index;
}

void ThemeManager::selectTheme(int index) {
  if (s_valid && index >= 0 && index < (int)s_header->themeCount) {
    s_selectedTheme = index;
  }
}

const ThemeEntry* ThemeManager::currentEntry() { return entry(s_selectedTheme); }

namespace {

/* Matches the blob layout written by generate_theme.py's
 * png_to_lz4_wallpaper_blob():
 *   uint32 width, height, chunkRows, chunkCount;
 *   uint32 offsets[chunkCount];   // relative to the start of chunk data
 *   uint32 sizes[chunkCount];     // compressed size of each chunk
 *   <compressed chunk 0><compressed chunk 1>...
 */
struct WallpaperBlobHeader {
  uint32_t width;
  uint32_t height;
  uint32_t chunkRows;
  uint32_t chunkCount;
};

// Must be big enough for the largest possible chunk: screen width x the
// chunk height used at generation time (defaults to 16 rows in
// generate_theme.py). Bump both if you ever generate taller chunks.
constexpr int k_maxChunkWidth = 320;
constexpr int k_maxChunkHeight = 16;

const WallpaperBlobHeader* wallpaperBlobHeader(const ThemeEntry* e) {
  return reinterpret_cast<const WallpaperBlobHeader*>(ThemeAreaStart +
                                                        e->wallpaperOffset);
}

}  // namespace

bool ThemeManager::hasWallpaper() {
  const ThemeEntry* e = currentEntry();
  return e != nullptr && e->isWallpaper && e->wallpaperOffset != 0;
}

int ThemeManager::wallpaperWidth() {
  const ThemeEntry* e = currentEntry();
  if (!e || !e->wallpaperOffset) {
    return 0;
  }
  return (int)wallpaperBlobHeader(e)->width;
}

int ThemeManager::wallpaperHeight() {
  const ThemeEntry* e = currentEntry();
  if (!e || !e->wallpaperOffset) {
    return 0;
  }
  return (int)wallpaperBlobHeader(e)->height;
}

const KDColor* ThemeManager::wallpaperChunkContaining(int globalY, int* localY) {
  static KDColor s_chunkBuffer[k_maxChunkWidth * k_maxChunkHeight];
  static int s_cachedThemeIndex = -1;
  static int s_cachedChunkIndex = -1;
  static int s_cachedChunkTop = 0;

  const ThemeEntry* e = currentEntry();
  if (!e || !hasWallpaper() || globalY < 0) {
    return nullptr;
  }

  const WallpaperBlobHeader* header = wallpaperBlobHeader(e);
  if (globalY >= (int)header->height || header->width > (uint32_t)k_maxChunkWidth ||
      header->chunkRows > (uint32_t)k_maxChunkHeight) {
    return nullptr;
  }

  int chunkIndex = globalY / (int)header->chunkRows;
  if (chunkIndex < 0 || chunkIndex >= (int)header->chunkCount) {
    return nullptr;
  }

  if (s_cachedThemeIndex != s_selectedTheme || s_cachedChunkIndex != chunkIndex) {
    const uint8_t* blobStart = reinterpret_cast<const uint8_t*>(header);
    const uint32_t* offsets = reinterpret_cast<const uint32_t*>(
        blobStart + sizeof(WallpaperBlobHeader));
    const uint32_t* sizes = offsets + header->chunkCount;
    const uint8_t* chunksData =
        reinterpret_cast<const uint8_t*>(sizes + header->chunkCount);

    int chunkTop = chunkIndex * (int)header->chunkRows;
    int chunkRows = (int)header->height - chunkTop;
    if (chunkRows > (int)header->chunkRows) {
      chunkRows = (int)header->chunkRows;
    }
    int dstSize = chunkRows * (int)header->width * sizeof(KDColor);

    OMG::Memory::Decompress(chunksData + offsets[chunkIndex],
                            reinterpret_cast<uint8_t*>(s_chunkBuffer),
                            sizes[chunkIndex], dstSize);

    s_cachedThemeIndex = s_selectedTheme;
    s_cachedChunkIndex = chunkIndex;
    s_cachedChunkTop = chunkTop;
  }

  *localY = globalY - s_cachedChunkTop;
  return s_chunkBuffer;
}

/* Icons -------------------------------------------------------------------
 * Icon blobs are a single flat LZ4 block (generate_theme.py's
 * png_to_lz4_image_blob()) with NO header at all: just RGB565 pixels,
 * compressed with the same lz4_compress() used for the wallpaper chunks.
 * Width/height are not stored in the blob - the firmware and the theme
 * generator both assume the fixed builtin icon size (passed in by the
 * caller, e.g. AppCell's k_iconWidth / k_iconHeight). */

int ThemeManager::iconIndexForApp(I18n::Message appName) {
  // Order must match FIXED_ICON_FILES in generate_theme.py exactly.
  // Names confirmed against apps/*/app.h / app.cpp (Descriptor::name()).
  switch (appName) {
    case I18n::Message::CalculApp:
      return 0;
    case I18n::Message::CodeApp:
      return 1;
    case I18n::Message::DistributionsApp:
      return 2;
    case I18n::Message::ElementsApp:
      return 3;
    case I18n::Message::FinanceApp:
      return 4;
    case I18n::Message::FunctionApp:
      return 5;
    case I18n::Message::InferenceApp:
      return 6;
    case I18n::Message::RegressionApp:
      return 7;
    case I18n::Message::SequenceApp:
      return 8;
    case I18n::Message::SettingsApp:
      return 9;
    case I18n::Message::SolverApp:
      return 10;
    case I18n::Message::StatsApp:
      return 11;
    default:
      return -1;
  }
}

bool ThemeManager::hasPalette() {
  const ThemeEntry* e = currentEntry();
  return e != nullptr && e->isPalette && e->paletteOffset != 0;
}

void ThemeManager::applyPalette() {
  if (!hasPalette()) {
    return;
  }
  const ThemeEntry* e = currentEntry();
  // Raw uint32s written by encode_palette() in generate_theme.py: one
  // 0x00RRGGBB value per color, already in the exact format KDColor::RGB24
  // expects, so no bit-shuffling needed here.
  const uint32_t* rawColors =
      reinterpret_cast<const uint32_t*>(ThemeAreaStart + e->paletteOffset);
  size_t count = e->paletteSize / sizeof(uint32_t);
  size_t n = count < Escher::Palette::numberOfPaletteSlots()
                 ? count
                 : Escher::Palette::numberOfPaletteSlots();

  KDColor colors[Escher::Palette::numberOfPaletteSlots()];
  for (size_t i = 0; i < n; i++) {
    colors[i] = KDColor::RGB24(rawColors[i]);
  }
  Escher::Palette::ApplyPalette(colors, n);
}

bool ThemeManager::hasIcon(int iconIndex) {
  const ThemeEntry* e = currentEntry();
  return e != nullptr && e->isIcons && iconIndex >= 0 &&
         iconIndex < (int)ThemeIconCount && e->iconOffsets[iconIndex] != 0;
}

const KDColor* ThemeManager::iconPixels(int iconIndex, int iconWidth,
                                        int iconHeight) {
  // Generous upper bound so a single static buffer can serve any reasonable
  // fixed icon size. Bump if the builtin icon size ever grows past this.
  constexpr int k_maxIconWidth = 64;
  constexpr int k_maxIconHeight = 64;
  static KDColor s_iconBuffer[k_maxIconWidth * k_maxIconHeight];
  static int s_cachedThemeIndex = -1;
  static int s_cachedIconIndex = -1;

  if (!hasIcon(iconIndex) || iconWidth <= 0 || iconHeight <= 0 ||
      iconWidth > k_maxIconWidth || iconHeight > k_maxIconHeight) {
    return nullptr;
  }

  if (s_cachedThemeIndex != s_selectedTheme || s_cachedIconIndex != iconIndex) {
    const ThemeEntry* e = currentEntry();
    uint32_t offset = e->iconOffsets[iconIndex];
    uint32_t size = e->iconSizes[iconIndex];
    int dstSize = iconWidth * iconHeight * (int)sizeof(KDColor);

    OMG::Memory::Decompress(
        reinterpret_cast<const uint8_t*>(ThemeAreaStart + offset),
        reinterpret_cast<uint8_t*>(s_iconBuffer), size, dstSize);

    s_cachedThemeIndex = s_selectedTheme;
    s_cachedIconIndex = iconIndex;
  }

  return s_iconBuffer;
}

void ThemeManager::refreshTheme(AppsWindow* window) {
  ThemeManager::applyPalette();
  if (window != nullptr) {
    window->reloadTitleBarView();
    window->redraw(true);
  }
}