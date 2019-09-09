TOP_DIR		:= ../../../..

include $(TOP_DIR)/incs.mk

TGT_LIB		 = $(ARCHIVE_DIR)/lib$(DEST_OS).a

SRCS		 =
SRCS		+= atomkernel.c
SRCS		+= atommutex.c
SRCS		+= atomqueue.c
SRCS		+= atomsem.c
SRCS		+= atomtimer.c

OBJS	 = $(foreach src, $(SRCS), $(OBJ_DIR)/$(src:.c=.o))

all : $(TGT_LIB)

include $(TOP_DIR)/rules.mk
