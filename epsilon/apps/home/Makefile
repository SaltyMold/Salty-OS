SOURCES_apps += $(addprefix $(PATH_apps)/home/,$(addsuffix :-test, \
  app.cpp \
  app_cell.cpp \
  controller.cpp \
  apps_layout.cpp \
))

$(call i18n_without_universal,home/i18n/base)

$(OUTPUT_DIRECTORY)/$(PATH_apps)/home/apps_layout.h: $(addprefix $(PATH_apps)/home/apps_layout.,py csv) | $$(@D)/.
	$(call rule_label,APPSLYT)
	$(PYTHON) $(filter %.py,$^) --layouts $(filter %.csv,$^) --header $@ --implementation $(basename $@).cpp --apps $(EPSILON_APPS)

$(OUTPUT_DIRECTORY)/$(PATH_apps)/home/apps_layout.cpp: $(OUTPUT_DIRECTORY)/$(PATH_apps)/home/apps_layout.h

$(call all_objects_for,$(PATH_apps)/home/controller.cpp): $(OUTPUT_DIRECTORY)/$(PATH_apps)/home/apps_layout.h

# Generate LZ4-compressed wallpaper sources (header + c implementation) in small row chunks.
_ion_png_serializer := $(PATH_ion)/image/png_serializer.py

SOURCES_apps += $(PATH_apps)/home/home_wallpaper.cimpl.cpp

$(call all_objects_for,$(PATH_apps)/home/controller.cpp): $(OUTPUT_DIRECTORY)/$(PATH_apps)/home/home_wallpaper.h
$(call all_objects_for,$(PATH_apps)/home/app_cell.cpp): $(OUTPUT_DIRECTORY)/$(PATH_apps)/home/home_wallpaper.h
$(call all_objects_for,$(PATH_apps)/home/controller.cpp): SFLAGS += -I$(OUTPUT_DIRECTORY)/$(PATH_apps)/home
$(call all_objects_for,$(PATH_apps)/home/app_cell.cpp): SFLAGS += -I$(OUTPUT_DIRECTORY)/$(PATH_apps)/home

$(OUTPUT_DIRECTORY)/$(PATH_apps)/home/home_wallpaper.h: $(PATH_apps)/home/home_wallpaper.png | $$(@D)/.
	$(call rule_label,PNGSER)
	$(PYTHON) $(_ion_png_serializer) --png $< --header $@ --cimplementation $(@:.h=.cimpl.cpp) --chunk-height 16

$(OUTPUT_DIRECTORY)/$(PATH_apps)/home/home_wallpaper.cimpl.cpp: $(OUTPUT_DIRECTORY)/$(PATH_apps)/home/home_wallpaper.h
