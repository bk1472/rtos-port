#----------------------------------------------------------------------------+
#	TOOL 이 정의되지 않은 경우, 컴파일 환경이 제대로 설정되지 않은 것으로
#	해석한다. 필요한 경우, 다른 변수들도 참조하는 것이 좋을 듯.
#
#	하위 Make 화일에서 정의된 것을 참조하는 경우.
#
#	  SRCS     : .c, .s 소스 리스트
#	  OBJS     : object file list
#	  TGT_LIB  : Archive Name
#	  SUB_DIRS : 하위 디렉토리 이름들
#
#----------------------------------------------------------------------------+
ifndef TOOL
$(error TOOL is not defined, Please include 'inc.mk' first)
endif
MKD_ERR		 = $(UTIL_DIR)/$(APPS_NAME).err.log

#=========================================================================================
# Rules for cleanup and dependency.
#=========================================================================================
.PHONY : depend clean clobber clean_dep

depend: .depend_sub .depend_ext $(OBJS:.o=.d)

clean: .clean_sub .clean_ext
	@$(RM) $(OBJS)

clobber: .clobber_sub .clobber_ext
	@$(RM) $(OBJS) $(OBJS:.o=.d) $(TGT_LIB) $(MKD_ERR)
	@$(RM) $(DEST_OS) $(DEST_OS).biz $(DEST_OS).bin $(DEST_OS).sym

clean_dep:
	@$(ECHO) "cleaning dependency files"
	@find . -name "*.d" -exec rm {} \;

.clean_sub:
	@( $(foreach dn, $(SUB_DIRS), $(MAKE) -C $(dn) clean   &&) $(ECHO) -n ) || exit 1

.clobber_sub:
	@( $(foreach dn, $(SUB_DIRS), $(MAKE) -C $(dn) clobber &&) $(ECHO) -n ) || exit 1

.depend_sub:
	@( $(foreach dn, $(SUB_DIRS), $(MAKE) -C $(dn) depend  &&) $(ECHO) -n ) || exit 1

subdirs: subdirs_ext
	@( $(foreach dn, $(SUB_DIRS), $(MAKE) -C $(dn)         &&) $(ECHO) -n ) || exit 1

.clean_ext .clobber_ext .depend_ext subdirs_ext:

#=========================================================================================
# Override implicit rules to generate .o files
#=========================================================================================
$(OBJ_DIR)/%.d : %.c
	@$(ECHO) "++++++++ Making $(notdir $@)"
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@( $(CC) -MM $(CFLAGS) $<											\
	  | sed  -f $(UTIL_DIR)/mkdep.sed									\
	  | grep -v "^  \\\\"												\
	  | sed  -e "s\$(<:.c=.o)\$@ $(OBJ_DIR)/$(<:.c=.o)\g"				\
	) > $@ 2>$(MKD_ERR) || (cat $(MKD_ERR); rm $(MKD_ERR) $@; exit 1)	\
	$(NULL)

$(OBJ_DIR)/%.d : %.S
	@$(ECHO) "++++++++ Making $(notdir $@)"
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@( $(CC) -MM $(CFLAGS_AS) $<										\
	  | sed  -f $(UTIL_DIR)/mkdep.sed									\
	  | grep -v "^  \\\\"												\
	  | sed  -e "s\$(<:.S=.o)\$@ $(OBJ_DIR)/$(<:.S=.o)\g"				\
	) > $@ 2>$(MKD_ERR) || (cat $(MKD_ERR); rm $(MKD_ERR) $@; exit 1)	\
	$(NULL)

$(OBJ_DIR)/%.d : %.s
	@$(ECHO) "++++++++ Making $(notdir $@)"
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@( $(CC) -MM $(CFLAGS_AS) $<										\
	  | sed  -f $(UTIL_DIR)/mkdep.sed									\
	  | grep -v "^  \\\\"												\
	  | sed  -e "s\$(<:.s=.o)\$@ $(OBJ_DIR)/$(<:.s=.o)\g"				\
	) > $@ 2>$(MKD_ERR) || (cat $(MKD_ERR); rm $(MKD_ERR) $@; exit 1)	\
	$(NULL)

$(OBJ_DIR)/%.d : %.cpp
	@$(ECHO) "++++++++ Making $(notdir $@)"
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@( $(CC) -MM $(CXXFLAGS) $<											\
	  | sed  -f $(UTIL_DIR)/mkdep.sed									\
	  | grep -v "^  \\\\"												\
	  | sed  -e "s\$(<:.cpp=.o)\$@ $(OBJ_DIR)/$(<:.cpp=.o)\g"			\
	) > $@ 2>$(MKD_ERR) || (cat $(MKD_ERR); rm $(MKD_ERR) $@; exit 1)	\
	$(NULL)

$(OBJ_DIR)/%.o:%.c
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@$(CCDV) $(CC) -c $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o:%.s
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@$(CCDV) $(CXX) -c $(CFLAGS_AS) -o $@ $<

$(OBJ_DIR)/%.o:%.cpp
	@test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR)
	@$(CCDV) $(CXX) -c $(CXXFLAGS) -o $@ $<

ifdef TGT_LIB
$(TGT_LIB) : $(OBJS)
	@test -d $(ARCHIVE_DIR) || mkdir -p $(ARCHIVE_DIR)
	@$(RM) $(TGT_LIB)
	@$(CCDV) $(AR) crs $(TGT_LIB) $(OBJS) || exit 1
endif

all: $(UTIL_DIR)/ccdv$(EXT)
#=========================================================================================
# define application name
#=========================================================================================
appName:
	@$(ECHO) "# Do not modify this file"			  > $(TOP_DIR)/apps/apps_name.mk
	@$(ECHO) "# This file is modified automatically" >> $(TOP_DIR)/apps/apps_name.mk
	@$(ECHO) "APPS_NAME ?= $(notdir $(shell pwd))"	 >> $(TOP_DIR)/apps/apps_name.mk
	@$(ECHO) "export APPS_NAME"	                     >> $(TOP_DIR)/apps/apps_name.mk

#=========================================================================================
# check endianess
#=========================================================================================
ENDIAN_SRC	 = dummy.c
ENDIAN_OBJ	 = dummy.o

check_endian :
ifeq "$(ENDIAN_TYPE)" ""
	@if [ ! -f $(ENDIAN_OBJ) ]; then												\
		$(TOUCH) $(ENDIAN_SRC);														\
		$(CC) $(CFLAGS) -o $(ENDIAN_OBJ) -c $(ENDIAN_SRC);							\
		$(ECHO) "# Do not modify this file"					 > $(ENDIAN_TYPE_MK);	\
		$(ECHO) "# This file is modified automatically"		>> $(ENDIAN_TYPE_MK);	\
		file $(ENDIAN_OBJ) | grep LSB > /dev/null;									\
		if [ $$? -eq 0 ]; then														\
			$(ECHO) "ENDIAN_TYPE = LITTLE_ENDIAN"			>> $(ENDIAN_TYPE_MK);	\
		else																		\
			$(ECHO) "ENDIAN_TYPE = BIG_ENDIAN"				>> $(ENDIAN_TYPE_MK);	\
		fi																			\
	fi
ENDIAN_TYPE = $(shell grep ENDIAN_TYPE $(ENDIAN_TYPE_MK) 2> /dev/null | awk '{print $$3}')
endif

#=========================================================================================
# Conditional directive to include dependency list itself
#=========================================================================================
ifeq ($(OBJS),)
INCLUDE_DEPEND	?= 0
else
INCLUDE_DEPEND	?= 1
ifneq ($(MAKECMDGOALS),)
ifneq ($(MAKECMDGOALS),depend)
INCLUDE_DEPEND	 = 0
endif
endif
endif

ifeq ($(INCLUDE_DEPEND), 1)
-include $(OBJS:.o=.d)
ifdef TGT_LIB
$(TGT_LIB) : $(OBJS:.o=.d)
endif
endif

$(UTIL_DIR)/ccdv$(EXT): $(CCDV_DIR)/ccdv.c $(CCDV_DIR)/sift-warn.c
ifeq ($(MAKELEVEL), 0)
	@$(MAKE) -C $(CCDV_DIR)
endif

$(UTIL_DIR)/dumpsym$(EXT):
ifeq ($(MAKELEVEL), 0)
	@$(MAKE) -C $(DUMPSYM_DIR) BUILD_MACH=$(BUILD_MACH)
endif
