#ifndef ION_DEVICE_SHARED_DRIVERS_RESET_H
#define ION_DEVICE_SHARED_DRIVERS_RESET_H

#include <stdint.h>

namespace Ion {
namespace Device {
namespace Reset {

void core();

enum class ResetType : bool {
  Hardware,  // POWER on, NRST pin
  Software   // crash or Reset::core()
};
/* Cause of the reset that started the current execution. Captured once at
 * boot by captureLastResetType() (kernel only). */
void captureLastResetType();
ResetType lastResetType();
void jump(uint32_t jumpIsrVectorAddress, bool useMainStack = true);

}
}
}

#endif
