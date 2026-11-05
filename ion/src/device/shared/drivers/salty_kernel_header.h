#ifndef ION_DEVICE_SHARED_DRIVERS_SALTY_KERNEL_HEADER_H
#define ION_DEVICE_SHARED_DRIVERS_SALTY_KERNEL_HEADER_H

#include <stdint.h>

/* Header of the salty kernel, placed at a fixed offset of the kernel image,
 * right after the vector table: slot + SaltyKernelHeader::Offset.
 *
 * The Epsilon kernel header (magic 0xDEC00DF0, version, patch level) and the
 * vector table (SP, Reset) are untouched, so a bootloader that only knows the
 * Epsilon layout keeps working.
 *
 */
namespace Ion {
namespace Device {

class SaltyKernelHeader {
public:
  constexpr static uint32_t Magic = 0x59544C53;  // SLTY
  constexpr static uint32_t Offset = 0x200;

  constexpr SaltyKernelHeader() : m_header(Magic), m_footer(Magic) {}

  bool isValid() const { return m_header == Magic && m_footer == Magic; }

private:
  uint32_t m_header;
  uint32_t m_footer;
};

static_assert(sizeof(SaltyKernelHeader) == 8, "The salty kernel header is 8 bytes long");

}
}

#endif
