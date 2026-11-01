#include <kernel/drivers/exam_mode.h>

namespace Ion {
namespace Device {
namespace ExamMode {

bool rawIsActive(uint16_t raw) {
  if (raw == k_blankEntry) {
    return false;  // never initialized: exam mode off
  }
  if ((raw & (1 << 15)) != 0) {
    return false;  // cleared
  }
  if ((raw & 1) != 0) {
    return true;  // configurable (press-to-test): active
  }
  uint16_t ruleset = (raw >> 1) & 0x3FFF;
  if (ruleset == k_uninitializedRuleset) {
    return false;
  }
  return ruleset != 0;  // 0 = Off
}

uint16_t currentRawValue(const uint8_t * sectorStart, const uint8_t * sectorEnd) {
  /* Entries are appended in the sector: the current value is the last entry
   * that is not blank. A valid value never equals 0xFFFF (bit 15 is 0). */
  uint16_t current = k_blankEntry;
  for (const uint8_t * p = sectorStart; p + 2 <= sectorEnd; p += 2) {
    uint16_t value = static_cast<uint16_t>(p[0] | (p[1] << 8));
    if (value == k_blankEntry) {
      break;
    }
    current = value;
  }
  return current;
}

}
}
}
