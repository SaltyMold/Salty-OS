#pragma once
#include <stdint.h>

namespace Ion {
namespace Device {

struct SaltyHeader {
  uint32_t header;
  char version[8];
  char commit[8];
  uint32_t themesAreaFlashStartA;
  uint32_t themesAreaFlashEndA;
  uint32_t themesAreaFlashStartB;
  uint32_t themesAreaFlashEndB;
  uint32_t footer;
};

constexpr uint32_t k_saltyMagic = 0x79746C53;

}  // namespace Device
}  // namespace Ion