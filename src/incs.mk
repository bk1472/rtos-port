.PHONY : ccdv tags

-include $(TOP_DIR)/apps/apps_name.mk

ifeq ($(OSTYPE), cygwin)
EXT			 = .exe
else
EXT			 =
endif

OBJ_BASE	 = objs/$(DEST_OS).$(CPU)
OBJ_DIR		 = ./$(OBJ_BASE)
ARCHIVE_DIR	 = $(TOP_DIR)/archives/$(DEST_OS).$(CPU)

ifneq ($(APP_DIR),)
ifneq ($(APP_DIR), $(APPS_NAME))
APPS_NAME = $(APP_DIR)
endif
endif

include $(TOP_DIR)/apps/$(APPS_NAME)/config.mk

ENDIAN_TYPE_MK	 = $(TOP_DIR)/apps/$(APPS_NAME)/endian.mk
-include $(ENDIAN_TYPE_MK)

include $(TOP_DIR)/tools.mk

UTIL_DIR	 = $(TOP_DIR)/util
CCDV_DIR	 = $(UTIL_DIR)/ccdv.src
DUMPSYM_DIR	 = $(UTIL_DIR)/dumpsym.src

all: $(UTIL_DIR)/ccdv$(EXT) $(UTIL_DIR)/dumpsym$(EXT)

tags:
	@$(ECHO) "[MAKE TAGS]"
	@ctags -R
