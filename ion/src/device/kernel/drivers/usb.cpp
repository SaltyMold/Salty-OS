#include <shared/drivers/usb.h>
#include <kernel/drivers/board.h>
#include <kernel/drivers/keyboard.h>
#include <string.h>

namespace Ion {
namespace Device {
namespace USB {

SlotInfo * slotInfo() {
  return reinterpret_cast<SlotInfo *>(Board::Config::SRAMAddress);
}

void willExecuteDFU() {
  /* 1- Configure the keyboard to detect back event to leave DFU mode if the
   * Back key is pressed, the calculator unplugged or the USB core
   * soft-disconnected. */
  // TODO: interrupt on 'home', 'power'
  // TODO: try to use Keyboard::scan? Is it too slow?
  /* WARNING: we can't use hardware interruptions are their code is within the
   * kernel which might be being rewritter when USB is active... */
  Keyboard::activateRow(Keyboard::rowForKey(Ion::Keyboard::Key::Back));

  /* 2- Disable all interrupts
   * The interrupt service routines live in the Flash and could be overwritten
   * by garbage during a firmware upgrade opration, so we disable them. */
  Board::shutdownInterruptions();

  //3- Keep usefull information about the currently running slot
  slotInfo()->updateKernelHeader();
}

void didExecuteDFU() {
  Board::initInterruptions();
  Keyboard::activateAllRows();
}

/* The userland chooses which keys leave the DFU mode (exitKeys).
 * bubbleUpEvents: if false, the keyboard state seen by the scan is consumed so
 * that it is not delivered to the userland once the DFU is over. */
bool shouldInterruptDFU(Ion::Keyboard::State exitKeys, bool bubbleUpEvents) {
  const Ion::Keyboard::Key back = Ion::Keyboard::Key::Back;
  if (static_cast<uint64_t>(exitKeys) == static_cast<uint64_t>(Ion::Keyboard::State(back))) {
    /* Usual case: only Back leaves the DFU. willExecuteDFU() activated its row,
     * no need to scan the whole keyboard. */
    return Keyboard::columnIsActive(Keyboard::columnForKey(back));
  }
  Ion::Keyboard::State state = Keyboard::scan();
  // scan() activates all the rows: restore the DFU configuration (Back row only)
  Keyboard::activateRow(Keyboard::rowForKey(back));
  if (!bubbleUpEvents) {
    Ion::Keyboard::popState();
  }
  return (static_cast<uint64_t>(state) & static_cast<uint64_t>(exitKeys)) != 0;
}

void SlotInfo::updateKernelHeader() {
  m_kernelHeaderAddress = Board::kernelHeader();
}

}
}
}
