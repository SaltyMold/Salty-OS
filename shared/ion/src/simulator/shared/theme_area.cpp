#include "theme_area.h"

#include <stdlib.h>
#include <string.h>

namespace Ion {
namespace Simulator {
namespace ThemeArea {

constexpr uint32_t k_wallpaperField = 24;
constexpr uint32_t k_paletteField = 32;
constexpr uint32_t k_iconOffsetsField = 40;
constexpr uint32_t k_iconSizesField = 88;

static uint8_t* s_staging = nullptr;
static size_t s_stagingSize = 0;

static uint8_t* s_area = nullptr;
static size_t s_areaSize = 0;

static int s_requestedSelection = -1;

static uint32_t read32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}


static bool blobIsValid(uint32_t offset, uint32_t size, uint32_t tableEnd,
                        size_t length) {
  if (offset == 0) {
    return true;
  }
  return offset >= tableEnd &&
         static_cast<uint64_t>(offset) + size <= static_cast<uint64_t>(length);
}

int load(const uint8_t* data, size_t length) {
  if (data == nullptr || length < k_headerSize) {
    return BadHeader;
  }
  if (length > k_capacity) {
    return TooBig;
  }
  if (read32(data) != k_magic || read32(data + 4) != k_version) {
    return BadHeader;
  }
  const uint32_t count = read32(data + 8);
  if (count > k_maxThemes) {
    return BadThemeCount;
  }
  const uint64_t tableEnd64 =
      k_headerSize + static_cast<uint64_t>(count) * k_entrySize;
  if (tableEnd64 > length) {
    return Truncated;
  }
  const uint32_t tableEnd = static_cast<uint32_t>(tableEnd64);

  for (uint32_t t = 0; t < count; t++) {
    const uint8_t* e = data + k_headerSize + t * k_entrySize;
    bool valid = blobIsValid(read32(e + k_wallpaperField),
                             read32(e + k_wallpaperField + 4), tableEnd,
                             length) &&
                 blobIsValid(read32(e + k_paletteField),
                             read32(e + k_paletteField + 4), tableEnd, length);
    for (uint32_t i = 0; valid && i < k_numberOfIcons; i++) {
      valid = blobIsValid(read32(e + k_iconOffsetsField + 4 * i),
                          read32(e + k_iconSizesField + 4 * i), tableEnd,
                          length);
    }
    if (!valid) {
      return BadOffset;
    }
  }

  uint8_t* copy = static_cast<uint8_t*>(malloc(length));
  if (copy == nullptr) {
    return OutOfMemory;
  }
  memcpy(copy, data, length);
  free(s_staging);
  s_staging = copy;
  s_stagingSize = length;
  return Ok;
}

bool commitPending() {
  if (s_staging == nullptr) {
    return false;
  }
  if (s_area == nullptr) {
    s_area = static_cast<uint8_t*>(calloc(1, k_capacity));
  }
  if (s_area != nullptr) {
    memcpy(s_area, s_staging, s_stagingSize);
    memset(s_area + s_stagingSize, 0, k_capacity - s_stagingSize);
    s_areaSize = s_stagingSize;
  }
  const bool committed = s_area != nullptr;
  free(s_staging);
  s_staging = nullptr;
  s_stagingSize = 0;
  return committed;
}

const uint8_t* data() { return s_area; }
size_t size() { return s_areaSize; }

void requestSelection(int index) { s_requestedSelection = index; }

int takeRequestedSelection() {
  int index = s_requestedSelection;
  s_requestedSelection = -1;
  return index;
}

}  // namespace ThemeArea
}  // namespace Simulator
}  // namespace Ion