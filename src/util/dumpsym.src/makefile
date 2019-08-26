BIT_SZ		 = 64
SRC_DIR		 = .
REL_DIR		 = ..

ifeq ($(HOSTTYPE), x86_64)
HOST_RULE	 = -m$(BIT_SZ)
else
HOST_RULE	 =
endif

include		$(SRC_DIR)/incs.mk


TGT			 = $(REL_DIR)/dumpsym

SRCS		 =
SRCS		+= main.c
SRCS		+= elf.c
SRCS		+= process.c
SRCS		+= process32.c
SRCS		+= process64.c
SRCS		+= addr2line.c
SRCS		+= decode_frame.c
SRCS		+= dwarf.c
SRCS		+= hist.c
SRCS		+= util.c

OBJS		 = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.c=.o)))

all : $(TGT)

$(TGT) : $(OBJS)
	@$(CC) $(HOST_RULE) -o $@ $(OBJS) -lz

clean :
	@rm -f $(OBJ_DIR)/*.o
	@rm -f $(TGT)

clobber : clean
	@rm -rf $(OBJ_DIR)

tags :
	@ctags -R

include		$(SRC_DIR)/rules.mk

.PHONY: $(TGT)
