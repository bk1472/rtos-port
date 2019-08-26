###############################################################################
# Makefile Modified by contigo@lge.com
#
###############################################################################
## core information

TOP_DIR		:= ../..

include $(TOP_DIR)/incs.mk

TGT_LIB		 = $(ARCHIVE_DIR)/libapp.a

SRCS		 =
SRCS		+= root.c
SRCS		+= keyboard.c
SRCS		+= cmd_io.c
SRCS		+= cmd_debug.c
SRCS		+= resource_mgr.c

OBJS		 = $(foreach src, $(SRCS), $(OBJ_DIR)/$(src:.c=.o))

all : $(TGT_LIB)

include $(TOP_DIR)/rules.mk
