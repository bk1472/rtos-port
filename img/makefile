#######################################################################
#
# Directories definition
#
#######################################################################
TOP_DIR		 = .
QEMU_VER	 = w32
QEMU_DIR	 = $(TOP_DIR)/qemu.$(QEMU_VER)

#######################################################################
#
# TOOL definition
#
#######################################################################
ifeq ($(OSTYPE), linux-gnu)
QEMU		 = qemu-system-arm
else
QEMU		 = $(QEMU_DIR)/qemu-system-arm
endif
ifeq ($(APPS_NAME),)
$(error "Application image is not defined!")
endif
IMAGE		 = $(DEST_OS)img
MKIMG		 = $(TOP_DIR)/../uboot/tools/mkimage
MKFS		 = /usr/sbin/mkfs.jffs2

#######################################################################
#
# Variables definition
#
#######################################################################
START_ADDR	 = 0xa0010000

run : $(IMAGE)
	@echo "Run QEMU machine"
	@$(QEMU) -M connex -pflash $< -serial telnet::2848,server,nowait,nodelay&
	@sleep 3
	@telnet localhost 2848

$(IMAGE): pre-img
	dd of=$@ bs=1k count=16k if=/dev/zero
	dd of=$@ bs=1k conv=notrunc if=u-boot.bin
	dd of=$@ bs=1k conv=notrunc seek=180 if=$(DEST_OS).jffs2

pre-img:
	@rm -f $(IMAGE)
	@rm -f $(DEST_OS).jffs2
	@cp $(TOP_DIR)/../src/apps/$(APPS_NAME)/$(DEST_OS).bin $(TOP_DIR)
	@cp $(TOP_DIR)/../uboot/u-boot.bin $(TOP_DIR)
	@$(MKIMG) -A arm -O linux -T kernel -C none -a $(START_ADDR) -e $(START_ADDR) -n '$(DEST_OS) 0.0.0.1' -d $(DEST_OS).bin uImage
	@test -d $(DEST_OS).img/boot || mkdir -p $(DEST_OS).img/boot
	@mv uImage $(DEST_OS).img/boot/
	@$(MKFS) -e 0x20000 -d $(DEST_OS).img -p -o $(DEST_OS).jffs2
