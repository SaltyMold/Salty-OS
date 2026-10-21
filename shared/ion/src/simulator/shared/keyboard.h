#pragma once

#include <SDL.h>
#include <ion/keyboard.h>

namespace Ion {
namespace Simulator {
namespace Keyboard {

void keyDown(Ion::Keyboard::Key k);
void keyUp(Ion::Keyboard::Key k);
void setPrefixKeyHeld(int which, bool down);
bool scanHandlesSDLKey(SDL_Scancode key);
void didScan();

}  // namespace Keyboard
}  // namespace Simulator
}  // namespace Ion
