#include "theme_manager.h"

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