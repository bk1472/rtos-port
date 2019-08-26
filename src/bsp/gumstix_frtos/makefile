TOP_DIR		:= ../..
include $(TOP_DIR)/incs.mk

TGT_LIB		 = $(ARCHIVE_DIR)/libbsp.a

SRCS_S		 =
SRCS_S		+= start.s
SRCS_S		+= port_s.s
SRCS_S		+= exception_handler_a.s
SRCS_S		+= cache_a.s
SRCS_S		+= mmu_a.s

SRCS_C		 =
SRCS_C		+= main.c
SRCS_C		+= port.c
SRCS_C		+= portISR.c
SRCS_C		+= kheap.c
SRCS_C		+= pxa255_serial.c
SRCS_C		+= exception_handler_c.c
SRCS_C		+= interrupts.c
SRCS_C		+= timer.c
SRCS_C		+= cache_c.c
SRCS_C		+= mmu_c.c

OBJS		 =$(foreach src, $(SRCS_S), $(OBJ_DIR)/$(src:.s=.o))
OBJS		+=$(foreach src, $(SRCS_C), $(OBJ_DIR)/$(src:.c=.o))

all : $(TGT_LIB)

include $(TOP_DIR)/rules.mk
