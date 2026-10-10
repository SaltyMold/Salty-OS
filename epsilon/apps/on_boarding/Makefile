SOURCES_apps += $(addprefix $(PATH_apps)/on_boarding/,$(addsuffix :-test, \
  app.cpp \
  salty_os.png \
  splash_controller.cpp \
  splash_timer.cpp \
  splash_view.cpp \
  startup_prompt_controller.cpp \
))

$(call depends_on_image,$(PATH_apps)/on_boarding/splash_controller.cpp,$(PATH_apps)/on_boarding/salty_os.png)

$(call i18n_without_universal,on_boarding/i18n/base)
