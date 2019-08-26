.PHONY : tags

OBJ_DIR		 = $(SRC_DIR)/objs
INC_DIR		 = $(SRC_DIR)/include
MKD_ERR		 = $(SRC_DIR)/.d.err
SCRPT_DIR	 = $(SRC_DIR)/script

CC			 = gcc

CFLAGS		 =
CFLAGS		+= -O2
CFLAGS		+= -I$(INC_DIR) -I.
CFLAGS		+= -DUNIX
CFLAGS		+= -DMKSYM
CFLAGS		+= -DBIT_WIDTH=$(BIT_SZ)
CFLAGS		+= -Wno-format
CFLAGS		+= -Wno-unused-result
CFLAGS		+= -Wno-incompatible-pointer-types
CFLAGS		+= -Wno-pointer-to-int-cast

all :
