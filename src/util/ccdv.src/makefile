CC		 = gcc
RM		 = rm -f
TARGET	 = ../ccdv

OBJ_DIR	 = .

SRCS	 =
SRCS	+= ccdv.c
SRCS	+= sift-warn.c

OBJS	 = $(foreach src, $(SRCS), $(OBJ_DIR)/$(src:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) -o $@ $^

%.o : %.c
	@$(CC) -Wno-format-security -Wno-format -c -o $@ $<

clean:
	@$(RM) $(OBJS)

clobber: clean
	@$(RM) $(TARGET)
