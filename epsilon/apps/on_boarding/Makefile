SOURCES_apps += $(addprefix $(PATH_apps)/on_boarding/,$(addsuffix :-test, \
  app.cpp \
  splash_controller.cpp \
  splash_timer.cpp \
  splash_view.cpp \
  startup_prompt_controller.cpp \
))

# salty_os.png: plain (non themed) image inlined by the escher inliner
$(call all_objects_for,$(PATH_apps)/on_boarding/splash_controller.cpp): $(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/salty_os.cpp
$(call all_objects_for,$(PATH_apps)/on_boarding/splash_controller.cpp): SFLAGS += -I$(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding
$(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/salty_os.h: $(OUTPUT_DIRECTORY)/$(PATH_apps)/on_boarding/salty_os.cpp ;
SOURCES_apps += $(call generated_sources_for,$(PATH_apps)/on_boarding/salty_os.cpp:-test)

$(call i18n_without_universal,on_boarding/i18n/base)
