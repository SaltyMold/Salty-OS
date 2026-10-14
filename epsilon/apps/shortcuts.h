#pragma once

#include <ion/keyboard.h>
#include <stdint.h>

/* Salty-OS keybinds.
 *
 * Every custom feature of the OS is triggered by holding the two PREFIX keys
 * together with the key of the feature. This file is the single place where
 * keys are assigned to features: the rest of the code only ever manipulates
 * Shortcuts::Action values.
 *
 * Adding a shortcut:
 *   1. #define its key below,
 *   2. add an Action,
 *   3. map them in KeyForAction() and FiresOncePerPress(),
 *   4. handle the Action in AppsContainer::handleShortcuts()
 *      (apps_container_shortcuts.cpp),
 *   5. document it in the README.
 *
 * Every key can be remapped at compile time without touching the code, e.g.
 *   make ... SFLAGS+="-DSALTY_SHORTCUT_KEY_NEXT_THEME=Ion::Keyboard::Key::Down"
 */

// Keys that must both be held for any shortcut to be considered
#ifndef SALTY_SHORTCUT_PREFIX_KEY_1
#define SALTY_SHORTCUT_PREFIX_KEY_1 Ion::Keyboard::Key::LeftParenthesis
#endif
#ifndef SALTY_SHORTCUT_PREFIX_KEY_2
#define SALTY_SHORTCUT_PREFIX_KEY_2 Ion::Keyboard::Key::RightParenthesis
#endif

// Key of each feature (to be held together with the prefix keys)
#ifndef SALTY_SHORTCUT_KEY_PREVIOUS_THEME
#define SALTY_SHORTCUT_KEY_PREVIOUS_THEME Ion::Keyboard::Key::Shift
#endif
#ifndef SALTY_SHORTCUT_KEY_NEXT_THEME
#define SALTY_SHORTCUT_KEY_NEXT_THEME Ion::Keyboard::Key::Alpha
#endif
#ifndef SALTY_SHORTCUT_KEY_TOGGLE_EXTERNAL_APPS
#define SALTY_SHORTCUT_KEY_TOGGLE_EXTERNAL_APPS Ion::Keyboard::Key::XNT
#endif
#ifndef SALTY_SHORTCUT_KEY_OPEN_SETTINGS
#define SALTY_SHORTCUT_KEY_OPEN_SETTINGS Ion::Keyboard::Key::Backspace
#endif

namespace Shortcuts {

enum class Action : uint8_t {
  PreviousTheme,
  NextTheme,
  ToggleExternalApps,
  OpenSettings,
  NumberOfActions,
};

constexpr int k_numberOfActions = static_cast<int>(Action::NumberOfActions);
static_assert(k_numberOfActions <= 8, "ActiveSet stores its actions on 8 bits");

constexpr Ion::Keyboard::Key k_prefixKey1 = SALTY_SHORTCUT_PREFIX_KEY_1;
constexpr Ion::Keyboard::Key k_prefixKey2 = SALTY_SHORTCUT_PREFIX_KEY_2;
static_assert(k_prefixKey1 != k_prefixKey2,
              "The two shortcut prefix keys must be different");

constexpr Ion::Keyboard::Key KeyForAction(Action action) {
  switch (action) {
    case Action::PreviousTheme:
      return SALTY_SHORTCUT_KEY_PREVIOUS_THEME;
    case Action::NextTheme:
      return SALTY_SHORTCUT_KEY_NEXT_THEME;
    case Action::ToggleExternalApps:
      return SALTY_SHORTCUT_KEY_TOGGLE_EXTERNAL_APPS;
    case Action::OpenSettings:
      return SALTY_SHORTCUT_KEY_OPEN_SETTINGS;
    default:
      return Ion::Keyboard::Key::None;
  }
}

/* An action that fires once per press (and not at every event while the keys
 * are held) must be handled with ActiveSet::isNewlyActive(). */
constexpr bool FiresOncePerPress(Action action) {
  return action == Action::ToggleExternalApps || action == Action::OpenSettings;
}

inline bool IsPrefixHeld(Ion::Keyboard::State keyboardState) {
  return keyboardState.keyDown(k_prefixKey1) &&
         keyboardState.keyDown(k_prefixKey2);
}

inline bool IsActive(Ion::Keyboard::State keyboardState, Action action) {
  return IsPrefixHeld(keyboardState) &&
         keyboardState.keyDown(KeyForAction(action));
}

// Snapshot of the shortcuts held down at a given time
class ActiveSet {
 public:
  constexpr ActiveSet() : m_bits(0) {}
  explicit ActiveSet(Ion::Keyboard::State keyboardState) : m_bits(0) {
    for (int i = 0; i < k_numberOfActions; i++) {
      if (IsActive(keyboardState, static_cast<Action>(i))) {
        m_bits |= static_cast<uint8_t>(1 << i);
      }
    }
  }
  constexpr bool isActive(Action action) const {
    return (m_bits >> static_cast<int>(action)) & 1;
  }
  // Active now, but not in `previous`
  constexpr bool isNewlyActive(Action action, ActiveSet previous) const {
    return isActive(action) && !previous.isActive(action);
  }

 private:
  uint8_t m_bits;
};

}  // namespace Shortcuts
