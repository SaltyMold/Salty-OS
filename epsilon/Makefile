# Import haussmann

PATH_haussmann := ../haussmann
APP_NAME := Epsilon
# Salty-OS targets the kernel 26.4.0 (the userland header must match it).
APP_VERSION := 26.4.0
OUTPUT_ROOT := output
DEBUG ?= 0
PLATFORM ?= host
STRICT_WARNINGS ?= 0

ALL_SPECIAL_SUBDIRECTORIES := bootloader kernel coverage

GCC_MINIMUM_VERSION := 12
CLANG_MINIMUM_VERSION := 17

include $(PATH_haussmann)/Makefile

# Information shown by the Salty settings app (apps/salty_settings). It all
# comes from here, from git or from the themes folder: nothing is hardcoded in
# the C++ sources (see apps/salty_settings/generate_info.sh).
# Version of Salty-OS: the git tag of the checkout (or the commit if there is no
# tag yet), "dev" outside of a git repository.
SALTY_OS_VERSION ?= $(or $(shell git describe --tags --always --dirty 2>/dev/null),dev)
# Upstream epsilon sources used for this port
SALTY_EPSILON_SOURCE_VERSION ?= 26.1.0
SALTY_THEMES_DIRECTORY ?= ../themes

# Further configuration

EXTERNAL_APPS_API_LEVEL ?= 0

SFLAGS += \
  -DASSERTIONS=$(ASSERTIONS) \
  -DOPTIMIZE_ASSERTIONS=$(OPTIMIZE_ASSERTIONS) \
  -DEXTERNAL_APPS_API_LEVEL=$(EXTERNAL_APPS_API_LEVEL)

include build/config.$(PLATFORM_TYPE).mak

# Select the font and layout
KANDINSKY_font_variant := epsilon
ION_variant := epsilon

POINCARE_variant := epsilon

# Import modules

$(call import_module,liba,../shared/liba)
$(call import_module,libaxx,../shared/libaxx)
$(call import_module,omg,../shared/omg)
$(call import_module,kandinsky,../shared/kandinsky)
$(call import_module,ion,../shared/ion)
$(call import_module,quiz,../shared/quiz)
$(call import_module,poincare,../poincare)
$(call import_module,eadk,eadk)
$(call import_module,escher,escher)
$(call import_module,python,python)
$(call import_module,apps,apps)

# Declare goals

include build/rules.mak
include build/rules.$(PLATFORM_TYPE).mak
