TOP_DIR		:= ../../../..

include $(TOP_DIR)/incs.mk

TGT_LIB		 = $(ARCHIVE_DIR)/lib$(DEST_OS).a

SRCS		 =
SRCS		+= croutine.c
SRCS		+= event_groups.c
SRCS		+= list.c
SRCS		+= queue.c
SRCS		+= stream_buffer.c
SRCS		+= tasks.c
SRCS		+= timers.c

OBJS	 = $(foreach src, $(SRCS), $(OBJ_DIR)/$(src:.c=.o))

all : $(TGT_LIB)

include $(TOP_DIR)/rules.mk
