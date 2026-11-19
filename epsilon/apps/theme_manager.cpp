#include "theme_manager.h"

#include <escher/theme/salty_palette.h>
#include <omg/memory.h>
#include <string.h>

#include "apps/apps_container.h"
#include "apps/home/app.h"

#if !PLATFORM_DEVICE
#include "../../shared/ion/src/simulator/shared/theme_area.h"
#endif

#if PLATFORM_DEVICE
namespace Ion {
namespace Device {
namespace Board {
bool isRunningSlotA();  // defined in ion/src/device/userland/drivers/board_dual_slots.cpp
}
}
}
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
  return Ion::Device::Board::isRunningSlotA() ? ThemeAreaStartSlotA: ThemeAreaStartSlotB;
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

/* Matches the blob layout written by generate_themes.py's
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
// generate_themes.py). Bump both if you ever generate taller chunks.
constexpr int k_maxChunkWidth = 320;
constexpr int k_maxChunkHeight = 16;

const WallpaperBlobHeader* wallpaperBlobHeader(const ThemeEntry* e) {
  return reinterpret_cast<const WallpaperBlobHeader*>(ThemeManager::areaBase() +
                                                        e->wallpaperOffset);
}

// Icons all have the same fixed size (the one of the home screen's AppCell).
// The buffer is only used for LZ4 icons, see ThemeIconFormatLZ4.
constexpr int k_iconBufferPixels =
    Home::AppCell::k_iconWidth * Home::AppCell::k_iconHeight;

/* The wallpaper chunk and icon buffers are pure decompression caches (~16 KiB
 * together). They are linked at the very start of the external apps RAM range
 * (see userland_shared.ld) instead of in the protected bss, so that this RAM is
 * available to external apps. Anything in that range can be overwritten by an
 * external app: the caches are therefore invalidated when it exits (see
 * ThemeManager::invalidateCaches). The cache indices stay in the protected
 * area, they are only trusted while the matching buffer is known to be valid.
 * The icon buffer is only written for themes whose icons are LZ4 compressed:
 * raw icons are read straight from flash and never go through it. */
#if PLATFORM_DEVICE
#define THEME_CACHE_SECTION __attribute__((section(".bss.$theme_cache")))
#else
#define THEME_CACHE_SECTION
#endif

THEME_CACHE_SECTION KDColor s_chunkBuffer[k_maxChunkWidth * k_maxChunkHeight];
THEME_CACHE_SECTION KDColor s_iconBuffer[k_iconBufferPixels];

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
 * Icon blobs have NO header at all, only pixels. How they are stored is given
 * by ThemeEntry::iconFormat (generate_themes.py's png_to_icon_blob()):
 *
 * - ThemeIconFormatRaw565: width * height RGB565 pixels, stored at a 4-byte
 *   aligned offset of the Theme Area. The flash is memory mapped, so
 *   iconPixels() hands out a pointer straight into it: no decompression, no
 *   copy, no RAM buffer. The size of the blob is checked against the size
 *   asked by the caller.
 * - ThemeIconFormatLZ4: a single flat LZ4 block of those same pixels, made
 *   with the same lz4_compress() as the wallpaper chunks. It is smaller in
 *   flash, but iconPixels() has to decompress it into a RAM buffer (cached
 *   until another icon is asked for).
 *
 * Width/height are not stored in the blob - the firmware and the theme
 * generator both assume the fixed builtin icon size (passed in by the
 * caller, e.g. AppCell's k_iconWidth / k_iconHeight). */

int ThemeManager::iconIndexForApp(I18n::Message appName) {
  // Order must match FIXED_ICON_FILES in generate_themes.py exactly.
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

namespace {

// The assets are not aligned: an unaligned uint32_t access faults on the device
uint32_t readUnaligned32(uintptr_t address) {
  const volatile uint8_t* p = reinterpret_cast<const volatile uint8_t*>(address);
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

}  // namespace

uint32_t ThemeManager::paletteColorCount(const ThemeEntry* e) {
  if (e == nullptr || !e->isPalette || e->paletteOffset == 0 ||
      e->paletteSize < k_paletteHeaderSize) {
    return 0;
  }
  const uintptr_t palette = areaBase() + e->paletteOffset;
  if (readUnaligned32(palette) != k_paletteMagic) {
    return 0;
  }
  const uint32_t count = readUnaligned32(palette + 4);
  if (count > (e->paletteSize - k_paletteHeaderSize) / k_paletteRecordSize) {
    return 0;
  }
  return count;
}

bool ThemeManager::hasPalette() {
  return paletteColorCount(currentEntry()) > 0;
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
  const uintptr_t records =
      areaBase() + e->paletteOffset + k_paletteHeaderSize;
  const uint32_t count = paletteColorCount(e);

  /* Colors are given by name, as the theme describes them: this build only
   * knows what each name means, not which colors a theme has. A color of the
   * theme this build has no name for is ignored, one the theme does not
   * provide keeps its default value. */
  Escher::SaltyPalette::Begin();
  for (uint32_t i = 0; i < count; i++) {
    const uintptr_t record = records + i * k_paletteRecordSize;
    char name[k_paletteNameSize];
    const volatile uint8_t* raw = reinterpret_cast<const volatile uint8_t*>(record);
    for (size_t j = 0; j < k_paletteNameSize; j++) {
      name[j] = static_cast<char>(raw[j]);
    }
    name[k_paletteNameSize - 1] = 0;
    Escher::SaltyPalette::Set(
        name, KDColor::RGB24(readUnaligned32(record + k_paletteNameSize)));
  }
  Escher::SaltyPalette::End();
}

int ThemeManager::defaultCircleLevel() {
  const ThemeEntry* e = currentEntry();
  return e ? e->defaultCircleLevel : 0;
}

bool ThemeManager::isAppNameBackgroundHover() {
  const ThemeEntry* e = currentEntry();
  return e ? (bool)e->isAppNameBackgroundHover : false;
}

static bool isKnownIconFormat(uint8_t format) {
  return format == ThemeIconFormatRaw565 || format == ThemeIconFormatLZ4;
}

bool ThemeManager::hasIcon(int iconIndex) {
  const ThemeEntry* e = currentEntry();
  /* A Theme Area with an unknown iconFormat can't be read as pixels: behave as
   * if it had no icon, the builtin ones are displayed. */
  return e != nullptr && e->isIcons && isKnownIconFormat(e->iconFormat) &&
         iconIndex >= 0 && iconIndex < (int)ThemeIconCount &&
         e->iconOffsets[iconIndex] != 0;
}

const KDColor* ThemeManager::iconPixels(int iconIndex, int iconWidth,
                                        int iconHeight) {
  if (!hasIcon(iconIndex) || iconWidth <= 0 || iconHeight <= 0) {
    return nullptr;
  }
  const ThemeEntry* e = currentEntry();
  const uint32_t offset = e->iconOffsets[iconIndex];
  const uint32_t size = e->iconSizes[iconIndex];
  if (offset > ThemeAreaSize || size > ThemeAreaSize - offset) {
    return nullptr;
  }
  const uint32_t rawSize =
      static_cast<uint32_t>(iconWidth * iconHeight) * sizeof(KDColor);

  if (e->iconFormat == ThemeIconFormatRaw565) {
    // Zero copy: the pixels are read where they are in flash.
    if (size != rawSize) {
      return nullptr;
    }
    const uintptr_t address = areaBase() + offset;
    if (address % k_iconAlignment != 0) {
      return nullptr;
    }
    return reinterpret_cast<const KDColor*>(address);
  }

  // ThemeIconFormatLZ4: decompress into the shared icon buffer, which has
  // exactly the size of an icon.
  if (size == 0 || rawSize != sizeof(s_iconBuffer)) {
    return nullptr;
  }
  if (s_cachedIconThemeIndex != s_selectedTheme ||
      s_cachedIconIndex != iconIndex) {
    OMG::Memory::Decompress(reinterpret_cast<const uint8_t*>(areaBase() + offset),
                            reinterpret_cast<uint8_t*>(s_iconBuffer), size,
                            rawSize);
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