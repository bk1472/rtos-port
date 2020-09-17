TOP_DIR		 = .

include $(TOP_DIR)/incs.mk

CMD			 =
CMD			+= run
CMD			+= boot
CMD			+= boot_config

all :
	@$(MAKE) -C $(TOP_DIR)/apps/$(APPS_NAME) $@

$(CMD) :
	@$(MAKE) -C $(TOP_DIR)/apps/$(APPS_NAME) $@


ifneq ($(MAKECMDGOALS),)
clean clobber :
	@$(MAKE) -C $(TOP_DIR)/apps/$(APPS_NAME) $@

distclean : clobber
	find . -type d \( -name objs -o -name archives \) -print0 | xargs -0 rm -rf
	find . -type f \( -name apps_name.mk -o -name dummy.* -o -name endian.mk -o -name tags \) \
		-print0 \
		| xargs -0 rm -f
	@($(MAKE) -C $(DUMPSYM_DIR) clobber > /dev/null 2>&1)
	@($(MAKE) -C $(CCDV_DIR) clobber > /dev/null 2>&1)
else
include $(TOP_DIR)/rules.mk
endif
