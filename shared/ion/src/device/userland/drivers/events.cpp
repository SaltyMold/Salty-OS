#include <ion/events.h>
#include <ion/keyboard/layout_events.h>

#include <shared/drivers/board_shared.h>

#include "svcall.h"

namespace Ion {
namespace Events {

size_t SVC_ATTRIBUTES copyText(uint8_t eventId, char* buffer,
                               size_t bufferSize) {
  SVC_RETURNING_R0(SVC_EVENTS_COPY_TEXT, size_t)
}

bool SVC_ATTRIBUTES isDefined(uint8_t eventId) {
  SVC_RETURNING_R0(SVC_EVENTS_IS_DEFINED, bool)
}

void SVC_ATTRIBUTES setShiftAlphaStatus(ShiftAlphaStatus s){
    SVC_RETURNING_VOID(SVC_EVENTS_SET_SHIFT_ALPHA_STATUS)}

ShiftAlphaStatus SVC_ATTRIBUTES shiftAlphaStatus() {
  SVC_RETURNING_R0(SVC_EVENTS_SHIFT_ALPHA_STATUS, ShiftAlphaStatus)
}

int SVC_ATTRIBUTES longPressCounter(){
    SVC_RETURNING_R0(SVC_EVENTS_LONG_PRESS_COUNTER, int)}

Event SVC_ATTRIBUTES getEvent(int* timeout) {
  SVC_RETURNING_R0(SVC_EVENTS_GET_EVENT, Event)
}

void SVC_ATTRIBUTES setSpinner(bool spiner) {
  SVC_RETURNING_VOID(SVC_EVENTS_SET_SPINNER)
}

void setSpinnerColors(KDColor background, KDColor foreground) {
  static bool s_kernelChecked = false;
  static bool s_kernelSupportsIt = false;
  static bool s_hasSent = false;
  static uint32_t s_lastSent = 0;
  if (!s_kernelChecked) {
    s_kernelSupportsIt = Device::Board::isSaltyKernel();
    s_kernelChecked = true;
  }
  if (!s_kernelSupportsIt) {
    // Official kernel: never issue an SVCall it does not know.
    return;
  }
  // background << 16 | foreground, both RGB565 (one register, one SVCall).
  uint32_t packed = (static_cast<uint32_t>(static_cast<uint16_t>(background))
                     << 16) |
                    static_cast<uint16_t>(foreground);
  if (s_hasSent && packed == s_lastSent) {
    return;
  }
  /* The kernel overwrites r0-r3 of the caller with whatever its handler left
   * in them (svcall_handler, step 7). SVC_RETURNING_VOID declares no clobber:
   * that only works for a wrapper defined in another translation unit than its
   * caller, where the compiler assumes the standard calling convention. Here
   * caller and SVC would share a file, GCC would see that nothing is modified
   * and keep r2/r3 (the addresses of the statics below) alive across the call.
   * So the argument is bound to r0 and r1-r3 are declared clobbered. */
  register uint32_t r0 asm("r0") = packed;
  asm volatile("svc %[immediate]"
               : "+r"(r0)
               : [immediate] "I"(SVC_EVENTS_SET_SPINNER_COLORS)
               : "r1", "r2", "r3", "memory");
  s_lastSent = packed;
  s_hasSent = true;
}

void openURL(const char* url) {}

const char* Event::text() const { return defaultText(); }

}  // namespace Events
}  // namespace Ion
