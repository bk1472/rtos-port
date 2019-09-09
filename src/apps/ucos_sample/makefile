TOP_DIR		 = ../..
APP_DIR		 = $(notdir $(shell pwd))

include $(TOP_DIR)/incs.mk
ifneq ($(DEST_OS), ucos)
$(error "Currnet environment for '$(DEST_OS)' is not for '$(ALLOW_OS)'")
endif
export APP_DIR

START_OBJS	 =
START_OBJS	+= $(TOP_DIR)/bsp/$(BSP_NAME)/$(OBJ_BASE)/start.o

SUB_DIRS	 =
SUB_DIRS	+= $(TOP_DIR)/bsp/$(BSP_NAME)
SUB_DIRS	+= $(TOP_DIR)/apps/cmn
SUB_DIRS	+= $(TOP_DIR)/lib/osa/$(DEST_OS)/os
#SUB_DIRS	+= $(TOP_DIR)/drv
SUB_DIRS	+= $(TOP_DIR)/lib/xlibc
SUB_DIRS	+= $(TOP_DIR)/lib/osa/$(DEST_OS)

LIBS		 =
LIBS		+= bsp
LIBS		+= app
LIBS		+= $(DEST_OS)
#LIBS		+= drv
LIBS		+= xlibc
LIBS		+= osadap
PRJS_LIB	 = $(foreach libs, $(LIBS), $(ARCHIVE_DIR)/lib$(libs).a)


all : appName check_endian $(DEST_OS).bin

$(DEST_OS).bin: $(DEST_OS)
	@$(OC) -O binary $< $@ && mv $(DEST_OS).bin $(DEST_OS).img
	@$(DUMPSYM) $<
	@dd if=$(DEST_OS).img of=$(DEST_OS).bin bs=1M conv=sync
	@dd if=$(DEST_OS).sym of=$(DEST_OS).bin oflag=append conv=notrunc
	@$(RM) $(DEST_OS).img

$(DEST_OS) : subdirs $(START_OBJS) $(PRJS_LIB)
	@$(CCDV) $(LD) -e _start_code -Ttext 0xa0010000 $(LDFLAGS) $(START_OBJS) $(PRJS_LIB) $(PRJS_LIB) -o $(DEST_OS) -L$(TOP_DIR)/../ext_lib -lgcc
#	@$(CCDV) $(LD) -e _start_code -Ttext 0xa0010000 $(LDFLAGS) $(START_OBJS) $(PRJS_LIB) $(PRJS_LIB) -o $(DEST_OS) \
		-L $(shell `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc

boot_config:
	@$(MAKE) -C $(TOP_DIR)/../uboot distclean
	@$(MAKE) -C $(TOP_DIR)/../uboot gumstix_config

boot:
	@$(MAKE) -C $(TOP_DIR)/../uboot CROSS_COMPILE=$(CROSS)-

run:
	@$(MAKE) -C $(TOP_DIR)/../img

include $(TOP_DIR)/rules.mk
