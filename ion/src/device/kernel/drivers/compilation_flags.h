#ifndef ION_DEVICE_KERNEL_DRIVERS_COMPILATION_FLAGS_H
#define ION_DEVICE_KERNEL_DRIVERS_COMPILATION_FLAGS_H

#include <stdint.h>

namespace Ion {
namespace Device {

/* bit 0: kernel DEBUG
 * bit 1: kernel ASSERTIONS
 * bit 2: kernel IN_FACTORY
 * bit 3: kernel EMBED_EXTRA_DATA (always 0: the bootloader is NOT embedded) */
uint16_t compilationFlags();

/* CRC32 of the bootloader (internal flash, without the trampoline sector) */
uint32_t bootloaderCRC32();

}
}

#endif
