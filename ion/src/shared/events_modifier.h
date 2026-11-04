#ifndef ION_SHARED_EVENTS_MODIFIER_H
#define ION_SHARED_EVENTS_MODIFIER_H

#include <ion/events.h>

namespace Ion {
namespace Events {

void removeShift();
void removeAlpha();
bool isShiftActive();
bool isAlphaActive();
bool isLockActive();
void updateModifiersFromEvent(Event e, Keyboard::State state);
/* Shift (or Alpha) was held while other keys were used. When it is released, it
 * is removed and these functions return true: the caller should return a plain
 * Shift (or Alpha) event to notify the userland. */
bool wasShiftReleased(Keyboard::State state);
bool wasAlphaReleased(Keyboard::State state);
int longPressCounter();
int longPressFactor();
void resetLongPress();
void resetRepetition();
void incrementLongPress();
void incrementRepetition();

}
}

#endif
