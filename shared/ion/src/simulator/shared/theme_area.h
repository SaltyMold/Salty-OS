#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Ion {
namespace Simulator {
namespace ThemeArea {

constexpr uint32_t k_capacity = 0x180000;  // ThemeAreaSize (1,5 Mio)
constexpr uint32_t k_magic = 0x87654321;   // ThemeAreaHeaderMagic
constexpr uint32_t k_version = 4;          // ThemeAreaHeaderVersion
constexpr uint32_t k_maxThemes = 256;      // ThemeCount
constexpr uint32_t k_numberOfIcons = 12;   // ThemeIconCount
constexpr uint32_t k_headerSize = 12;      // sizeof(ThemeAreaHeader)
constexpr uint32_t k_entrySize = 136;      // sizeof(ThemeEntry)

enum LoadResult : int {
  Ok = 0,
  TooBig = -1,
  BadHeader = -2,
  BadThemeCount = -3,
  OutOfMemory = -4,
  Truncated = -5,
  BadOffset = -6,
};

int load(const uint8_t* data, size_t length);

bool commitPending();

const uint8_t* data();
size_t size();

}  // namespace ThemeArea
}  // namespace Simulator
}  // namespace Ion