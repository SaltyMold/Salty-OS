#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* IonSoftwareVersion();
const char* IonPatchLevel();
void IonSimulatorKeyboardKeyDown(int keyIndex);
void IonSimulatorKeyboardKeyUp(int keyIndex);
void IonSimulatorShortcutPrefixKey(int which, int down);
void IonSimulatorEventsPushEvent(int eventNumber);
void IonSimulatorEventsPushEventArray(const char* eventBufferAsChar, int length,
                                      int fromIndex, int toIndex);
int IonSimulatorNumberOfEventsInArray(const char* eventBuffer, int length);
int IonSimulatorLoadThemeArea(const uint8_t* data, int length);
int IonSimulatorSelectTheme(int index);

#ifdef __cplusplus
}
#endif
