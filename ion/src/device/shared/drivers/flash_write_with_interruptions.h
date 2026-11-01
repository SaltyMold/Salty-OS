#ifndef ION_DEVICE_SHARED_DRIVERS_FLASH_WRITE_WITH_INTERRUPTIONS_H
#define ION_DEVICE_SHARED_DRIVERS_FLASH_WRITE_WITH_INTERRUPTIONS_H

#include <stddef.h>
#include <stdint.h>

namespace Ion {
namespace Device {
namespace Flash {

/* When handleInterrupts is true, the kernel shuts the interruptions down before
 * touching the external flash (their handlers live in this flash) and turns
 * them back on afterwards. Otherwise the caller already did it. */
void MassEraseWithInterruptions(bool handleInterrupts);
bool EraseSectorWithInterruptions(int i, bool handleInterrupts);
bool WriteMemoryWithInterruptions(uint8_t * destination, const uint8_t * source,
                                  size_t length, bool handleInterrupts);

}
}
}

#endif
