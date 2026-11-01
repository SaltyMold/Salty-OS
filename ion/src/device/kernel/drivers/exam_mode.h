#ifndef ION_DEVICE_KERNEL_DRIVERS_EXAM_MODE_H
#define ION_DEVICE_KERNEL_DRIVERS_EXAM_MODE_H

#include <stddef.h>
#include <stdint.h>

namespace Ion {
namespace Device {
namespace ExamMode {

/* The exam mode state is stored by the userland in the ExamBytes entry of the
 * persisting bytes (see Ion::ExamMode::Configuration):
 *  - bit 0     : "configurable" (press-to-test flags)
 *  - bits 1-14 : index of the preset rules (0 = Off, 12 = Uninitialized) when
 *                the configuration is not "configurable"
 *  - bit 15    : "cleared", always 0 for a valid configuration
 * The kernel only needs to know whether an exam mode is active. */
constexpr uint16_t k_blankEntry = 0xFFFF;
constexpr uint16_t k_uninitializedRuleset = 12;  // Ion::ExamMode::Ruleset::Uninitialized

// Pure functions (no hardware access)
bool rawIsActive(uint16_t raw);
/* Returns the current value of the ExamBytes entry: the last non-blank 2-bytes
 * entry of the sector, or k_blankEntry if the sector is blank. */
uint16_t currentRawValue(const uint8_t * sectorStart, const uint8_t * sectorEnd);

// Reads the persisting bytes of the running slot.
bool isActive();

}
}
}

#endif
