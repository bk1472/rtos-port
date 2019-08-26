$(OBJ_DIR)/%.o : %.c
	@$(CC) $(CFLAGS) $(HOST_RULE) -o $@ -c $<

$(OBJ_DIR)/%.d : %.c
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@(	$(CC) -MM $(CFLAGS) $<												\
	   | sed     -f $(SCRPT_DIR)/mkdep.sed									\
	   | grep    -v "^ \\\\"												\
	   | sed     -e "s\$(<:.c=.o)\$@ $(OBJ_DIR)/$(<:.c=.o)\g"				\
	) > $@

ifeq ($(OBJS),)
INCLUDE_DEPEND	?= 0
else
INCLUDE_DEPEND	?= 1
endif

ifeq ($(MAKECMDGOALS),)
ifeq ($(INCLUDE_DEPEND),1)
-include $(OBJS:.o=.d)
$(TGT) : $(OBJS:.o=.d)
endif
endif
