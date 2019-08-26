TOP_DIR		:= ../..

include $(TOP_DIR)/incs.mk

TGT_LIB		 = $(ARCHIVE_DIR)/libxlibc.a

SRCS_S		 =

SRCS_C		 =
SRCS_C		+= xlibc_string.c
SRCS_C		+= addr2line.c
SRCS_C		+= xlibc_ctype.c
SRCS_C		+= dbgprint.c
SRCS_C		+= xlibc_utils.c
SRCS_C		+= xlibc_snprintf.c
SRCS_C		+= xlibc_vsprintf.c
SRCS_C		+= xlibc_basic.c

OBJS		 =$(foreach src, $(SRCS_S), $(OBJ_DIR)/$(src:.s=.o))
OBJS		+=$(foreach src, $(SRCS_C), $(OBJ_DIR)/$(src:.c=.o))

all : $(TGT_LIB)

include $(TOP_DIR)/rules.mk
