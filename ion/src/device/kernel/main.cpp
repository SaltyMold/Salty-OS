#include <drivers/reset.h>
#include <kernel/drivers/authentication.h>
#include <kernel/drivers/board.h>
#include <kernel/drivers/cortex_control.h>
#include <assert.h>
#include <string.h>

void kernel_main() {
  // Remember why we booted (SVC_RESET_LAST_RESET_TYPE) before anything else.
  Ion::Device::Reset::captureLastResetType();

  switch_to_unpriviledged();

  // Jump to userland
  Ion::Device::Reset::jump(Ion::Device::Board::userlandStart(), false);
}
