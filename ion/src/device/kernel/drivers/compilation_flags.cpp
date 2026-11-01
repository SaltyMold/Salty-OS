#include <kernel/drivers/compilation_flags.h>
#include <shared/drivers/crc32.h>
#include <shared/drivers/config/board.h>
#include <drivers/config/board.h>

namespace Ion {
namespace Device {

uint16_t compilationFlags() {
  uint16_t flags = 0;
#if DEBUG
  flags |= 1 << 0;
#endif
#if ASSERTIONS
  flags |= 1 << 1;
#endif
  return flags;
}

uint32_t bootloaderCRC32() {
  /* Same range as the official kernel: the bootloader code, from the start of
   * the internal flash up to the trampoline (excluded). */
  constexpr uint32_t length = Board::Config::BootloaderTrampolineAddress - Board::Config::BootloaderStartAddress;
  return crc32Word(reinterpret_cast<const uint32_t *>(Board::Config::BootloaderStartAddress), length / sizeof(uint32_t));
}

}
}
