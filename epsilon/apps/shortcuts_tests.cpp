#include <apps/shortcuts.h>
#include <quiz.h>

#include <initializer_list>

using namespace Shortcuts;

static Ion::Keyboard::State stateWithKeys(
    std::initializer_list<Ion::Keyboard::Key> keys) {
  Ion::Keyboard::State state;
  for (Ion::Keyboard::Key key : keys) {
    state.setKey(key);
  }
  return state;
}

QUIZ_CASE(apps_shortcuts_need_both_prefix_keys) {
  quiz_assert(!IsActive(Ion::Keyboard::State(), Action::NextTheme));
  // Prefix keys alone trigger nothing
  Ion::Keyboard::State prefixOnly = stateWithKeys({k_prefixKey1, k_prefixKey2});
  for (int i = 0; i < k_numberOfActions; i++) {
    quiz_assert(!IsActive(prefixOnly, static_cast<Action>(i)));
  }
  // One prefix key is not enough, nor is the feature key alone
  for (int i = 0; i < k_numberOfActions; i++) {
    Action action = static_cast<Action>(i);
    quiz_assert(!IsActive(stateWithKeys({KeyForAction(action)}), action));
    quiz_assert(
        !IsActive(stateWithKeys({k_prefixKey1, KeyForAction(action)}), action));
    quiz_assert(
        !IsActive(stateWithKeys({k_prefixKey2, KeyForAction(action)}), action));
  }
}

QUIZ_CASE(apps_shortcuts_trigger_their_own_action_only) {
  for (int i = 0; i < k_numberOfActions; i++) {
    Action action = static_cast<Action>(i);
    ActiveSet active(
        stateWithKeys({k_prefixKey1, k_prefixKey2, KeyForAction(action)}));
    for (int j = 0; j < k_numberOfActions; j++) {
      quiz_assert(active.isActive(static_cast<Action>(j)) == (i == j));
    }
  }
}

QUIZ_CASE(apps_shortcuts_newly_active) {
  Action action = Action::ToggleExternalApps;
  quiz_assert(FiresOncePerPress(action));
  ActiveSet released;
  ActiveSet held(
      stateWithKeys({k_prefixKey1, k_prefixKey2, KeyForAction(action)}));
  quiz_assert(held.isNewlyActive(action, released));
  // Still held: not a new press
  quiz_assert(!held.isNewlyActive(action, held));
  // Released: nothing to fire
  quiz_assert(!released.isNewlyActive(action, held));
}
