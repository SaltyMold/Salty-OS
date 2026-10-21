#include "theme_manager.h"

#include <escher/theme/salty_palette.h>
#include <omg/memory.h>
#include <string.h>

#include "apps/apps_container.h"
#include "apps/home/app.h"

#if !PLATFORM_DEVICE
#include "../../shared/ion/src/simulator/shared/theme_area.h"
#endif

using namespace Ion::Device::Board::Config;

#if !PLATFORM_DEVICE
namespace SimArea = Ion::Simulator::ThemeArea;
static_assert(SimArea::k_capacity == ThemeAreaSize, "theme_area.h out of sync");
static_assert(SimArea::k_magic == ThemeAreaHeaderMagic, "theme_area.h out of sync");
static_assert(SimArea::k_version == ThemeAreaHeaderVersion, "theme_area.h out of sync");
static_assert(SimArea::k_maxThemes == ThemeCount, "theme_area.h out of sync");
static_assert(SimArea::k_numberOfIcons == ThemeIconCount, "theme_area.h out of sync");
static_assert(sizeof(ThemeAreaHeader) == SimArea::k_headerSize, "theme_area.h out of sync");
static_assert(sizeof(ThemeEntry) == SimArea::k_entrySize, "theme_area.h out of sync");
static_assert(offsetof(ThemeEntry, wallpaperOffset) == 24, "theme_area.cpp out of sync");
static_assert(offsetof(ThemeEntry, paletteOffset) == 32, "theme_area.cpp out of sync");
static_assert(offsetof(ThemeEntry, iconOffsets) == 40, "theme_area.cpp out of sync");
static_assert(offsetof(ThemeEntry, iconSizes) == 88, "theme_area.cpp out of sync");
#endif

const ThemeAreaHeader* ThemeManager::s_header = nullptr;
bool ThemeManager::s_valid = false;
int ThemeManager::s_selectedTheme = 0;

void ThemeManager::init() {
  s_valid = false;
  s_header = nullptr;
  s_selectedTheme = 0;

  const uintptr_t base = areaBase();
  if (base == 0) {
    /* Simulators: no Theme Area received yet. Nothing is mapped at the flash
     * address there, dereferencing it would trap in WebAssembly. */
    return;
  }

  const ThemeAreaHeader* h = reinterpret_cast<const ThemeAreaHeader*>(base);
  if (h->magic != ThemeAreaHeaderMagic ||
      h->version != ThemeAreaHeaderVersion ||
      h->themeCount > ThemeCount) {
    return;
  }

  s_header = h;
  s_valid = true;
  // Boot on the virtual "Default" theme (index 0): applyPalette() called
  // right after init() will then be a no-op, leaving Escher's builtin
  // palette/icons untouched until the user explicitly picks a real theme.
}

uintptr_t ThemeManager::areaBase() {
#if PLATFORM_DEVICE
  return ThemeAreaStart;
#else
  return reinterpret_cast<uintptr_t>(Ion::Simulator::ThemeArea::data());
#endif
}

bool ThemeManager::applyPendingThemeArea() {
#if PLATFORM_DEVICE
  return false;
#else
  if (!Ion::Simulator::ThemeArea::commitPending()) {
    return false;
  }
  // Keep the selected theme if it still exists in the new Theme Area
  const int previousTheme = s_selectedTheme;
  init();
  if (s_valid && previousTheme < themeCount()) {
    s_selectedTheme = previousTheme;
  }
  invalidateCaches();
  return true;
#endif
}

const ThemeEntry* ThemeManager::entry(int index) {
  if (!s_valid || index < 0 || index >= (int)s_header->themeCount) {
    return nullptr;
  }
  const uint8_t* base =
      reinterpret_cast<const uint8_t*>(areaBase()) + sizeof(ThemeAreaHeader);
  return reinterpret_cast<const ThemeEntry*>(base) + index;
}

void ThemeManager::selectTheme(int index) {
  if (s_valid && index >= 0 && index < themeCount()) {
    s_selectedTheme = index;
  }
}

const ThemeEntry* ThemeManager::currentEntry() {
  // Index 0 is the virtual "Default" theme: no backing ThemeEntry, so every
  // hasXxx()/accessor built on top of currentEntry() naturally falls back
  // to the same "no theme" behavior as an invalid Theme Area.
  if (s_selectedTheme == 0) {
    return nullptr;
  }
  return entry(s_selectedTheme - 1);
}

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
  return reinterpret_cast<const WallpaperBlobHeader*>(ThemeManager::areaBase() +
                                                        e->wallpaperOffset);
}

// Generous upper bound so a single static buffer can serve any reasonable
// fixed icon size. Bump if the builtin icon size ever grows past this.
constexpr int k_maxIconWidth = 64;
constexpr int k_maxIconHeight = 64;

/* The wallpaper chunk and icon buffers are pure decompression caches (~18 KiB
 * together). They are linked at the very start of the external apps RAM range
 * (see userland_shared.ld) instead of in the protected bss, so that this RAM
 * is available to external apps. Anything in that range can be overwritten by
 * an external app: the caches are therefore invalidated when it exits (see
 * ThemeManager::invalidateCaches). The cache indices stay in the protected
 * area, they are only trusted while the matching buffer is known to be valid.
 */
#if PLATFORM_DEVICE
#define THEME_CACHE_SECTION __attribute__((section(".bss.$theme_cache")))
#else
#define THEME_CACHE_SECTION
#endif

THEME_CACHE_SECTION KDColor s_chunkBuffer[k_maxChunkWidth * k_maxChunkHeight];
THEME_CACHE_SECTION KDColor s_iconBuffer[k_maxIconWidth * k_maxIconHeight];

int s_cachedChunkThemeIndex = -1;
int s_cachedChunkIndex = -1;
int s_cachedChunkTop = 0;
int s_cachedIconThemeIndex = -1;
int s_cachedIconIndex = -1;

}  // namespace

bool ThemeManager::wallpaperInfo(const ThemeEntry* e, WallpaperInfo* info) {
  if (e == nullptr || !e->isWallpaper || e->wallpaperOffset == 0) {
    return false;
  }
  const WallpaperBlobHeader* header = wallpaperBlobHeader(e);
  info->width = header->width;
  info->height = header->height;
  info->chunkRows = header->chunkRows;
  info->chunkCount = header->chunkCount;
  return true;
}

void ThemeManager::invalidateCaches() {
  s_cachedChunkThemeIndex = -1;
  s_cachedChunkIndex = -1;
  s_cachedChunkTop = 0;
  s_cachedIconThemeIndex = -1;
  s_cachedIconIndex = -1;
}

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

  if (s_cachedChunkThemeIndex != s_selectedTheme ||
      s_cachedChunkIndex != chunkIndex) {
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

    s_cachedChunkThemeIndex = s_selectedTheme;
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
    // Nothing to apply for the current selection (the virtual "Default"
    // theme, or a real theme flashed without a palette): fall back to
    // Escher's compiled-in colors instead of leaving whatever a
    // previously-selected theme last wrote into them.
    Escher::SaltyPalette::Reset();
    return;
  }
  const ThemeEntry* e = currentEntry();
  // Raw uint32s written by encode_palette() in generate_theme.py: one
  // 0x00RRGGBB value per color, already in the exact format KDColor::RGB24
  // expects, so no bit-shuffling needed here.
  /* The palette can start at any address (right after a wallpaper blob), so it
   * is read byte by byte: an unaligned uint32_t access faults on the device. */
  const volatile uint8_t* rawColors =
      reinterpret_cast<const volatile uint8_t*>(areaBase() +
                                                e->paletteOffset);
  size_t count = e->paletteSize / sizeof(uint32_t);
  size_t n = count < Escher::SaltyPalette::numberOfSlots()
                 ? count
                 : Escher::SaltyPalette::numberOfSlots();

  KDColor colors[Escher::SaltyPalette::numberOfSlots()];
  for (size_t i = 0; i < n; i++) {
    const volatile uint8_t* c = rawColors + i * sizeof(uint32_t);
    colors[i] = KDColor::RGB24(static_cast<uint32_t>(c[0]) |
                               (static_cast<uint32_t>(c[1]) << 8) |
                               (static_cast<uint32_t>(c[2]) << 16) |
                               (static_cast<uint32_t>(c[3]) << 24));
  }
  Escher::SaltyPalette::Apply(colors, n);
}

int ThemeManager::defaultCircleLevel() {
  const ThemeEntry* e = currentEntry();
  return e ? e->defaultCircleLevel : 0;
}

bool ThemeManager::isAppNameBackgroundHover() {
  const ThemeEntry* e = currentEntry();
  return e ? (bool)e->isAppNameBackgroundHover : false;
}

bool ThemeManager::hasIcon(int iconIndex) {
  const ThemeEntry* e = currentEntry();
  return e != nullptr && e->isIcons && iconIndex >= 0 &&
         iconIndex < (int)ThemeIconCount && e->iconOffsets[iconIndex] != 0;
}

const KDColor* ThemeManager::iconPixels(int iconIndex, int iconWidth,
                                        int iconHeight) {
  if (!hasIcon(iconIndex) || iconWidth <= 0 || iconHeight <= 0 ||
      iconWidth > k_maxIconWidth || iconHeight > k_maxIconHeight) {
    return nullptr;
  }

  if (s_cachedIconThemeIndex != s_selectedTheme ||
      s_cachedIconIndex != iconIndex) {
    const ThemeEntry* e = currentEntry();
    uint32_t offset = e->iconOffsets[iconIndex];
    uint32_t size = e->iconSizes[iconIndex];
    int dstSize = iconWidth * iconHeight * (int)sizeof(KDColor);

    OMG::Memory::Decompress(
        reinterpret_cast<const uint8_t*>(areaBase() + offset),
        reinterpret_cast<uint8_t*>(s_iconBuffer), size, dstSize);

    s_cachedIconThemeIndex = s_selectedTheme;
    s_cachedIconIndex = iconIndex;
  }

  return s_iconBuffer;
}

void ThemeManager::refreshTheme(AppsWindow* window) {
  ThemeManager::applyPalette();

  AppsContainer* appsContainer = AppsContainer::sharedAppsContainer();
  Home::App* homeApp = Home::App::app();
  if (appsContainer != nullptr && homeApp != nullptr &&
      homeApp->snapshot() == appsContainer->homeAppSnapshot()) {
    homeApp->reloadAppCells();
  }

  if (window != nullptr) {
    window->reloadTitleBarView();
    window->redraw(true);
  }
}