#pragma once

/* Salty-OS version, shown in the debug screen (Shortcuts::Action::OpenDebug).
 * Override it at build time, e.g.
 *   make ... SFLAGS+='-DSALTY_OS_VERSION=\"1.2.0\"' */
#ifndef SALTY_OS_VERSION
#define SALTY_OS_VERSION "dev"
#endif

// Version of Epsilon this userland is based on
#ifndef SALTY_OS_EPSILON_BASE_VERSION
#define SALTY_OS_EPSILON_BASE_VERSION "25.2.2"
#endif
