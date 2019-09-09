TOP_DIR		:= ../../../..

include $(TOP_DIR)/incs.mk

TGT_LIB		 = $(ARCHIVE_DIR)/lib$(DEST_OS).a

SRCS		 =
SRCS		+= os_cfg_app.c
SRCS		+= os_core.c
SRCS		+= os_dbg.c
SRCS		+= os_flag.c
SRCS		+= os_int.c
SRCS		+= os_mem.c
SRCS		+= os_mon.c
SRCS		+= os_msg.c
SRCS		+= os_mutex.c
SRCS		+= os_pend_multi.c
SRCS		+= os_prio.c
SRCS		+= os_q.c
SRCS		+= os_sem.c
SRCS		+= os_stat.c
SRCS		+= os_task.c
SRCS		+= os_tick.c
SRCS		+= os_time.c
SRCS		+= os_tmr.c
SRCS		+= os_var.c

OBJS	 = $(foreach src, $(SRCS), $(OBJ_DIR)/$(src:.c=.o))

all : $(TGT_LIB)

include $(TOP_DIR)/rules.mk
