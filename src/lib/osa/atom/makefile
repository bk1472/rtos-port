TOP_DIR		 = ../../..

include $(TOP_DIR)/incs.mk

TGT_LIB		 = $(ARCHIVE_DIR)/libosadap.a

SRCS_S		 =
SRCS_S		+= byteLib$(CPU).s

SRCS_C		 =
SRCS_C		+= arch_$(CPU).c
SRCS_C		+= dsm_$(CPU).c
SRCS_C		+= osadap.c
SRCS_C		+= osamem.c
SRCS_C		+= osa_backtrace.c

VPATH		 = arch/$(CPU)

OBJS		 =$(foreach src, $(SRCS_S), $(OBJ_DIR)/$(src:.s=.o))
OBJS		+=$(foreach src, $(SRCS_C), $(OBJ_DIR)/$(src:.c=.o))

all : $(TGT_LIB)

include $(TOP_DIR)/rules.mk
