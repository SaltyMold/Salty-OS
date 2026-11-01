#ifndef ION_DEVICE_KERNEL_DRIVERS_BOARD_H
#define ION_DEVICE_KERNEL_DRIVERS_BOARD_H

#include <stdint.h>
#include <shared/drivers/board_frequency.h>
#include <shared/drivers/board_privileged.h>
#include <shared/drivers/board_unprivileged.h>

namespace Ion {
namespace Device {
namespace Board {

void initSystemClocks();
void initPeripheralsClocks();
void shutdownPeripheralsClocks(bool keepLEDAwake = false);

void initInterruptions();
void shutdownInterruptions();

void initPeripherals(bool fromBootloader);
void shutdownPeripherals(bool keepLEDAwake = false);

void initInterruptionPriorities();
void setDefaultGPIO();

/* No signature verification: the clearance level never changes. These two
 * entry points are kept because the userland still calls them (SVC 6 and 7). */
void updateClearanceLevelForExternalApps();
void updateClearanceLevelForUnauthenticatedUserland();

bool addressInUserlandRAM(void * address);

}
}
}

#endif

