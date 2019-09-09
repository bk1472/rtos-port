CPU			 = arm
BSP_NAME	 = gumstix_$(DEST_OS)

INCS		 =
INCS		+= -I$(TOP_DIR)
INCS		+= -I$(TOP_DIR)/include
INCS		+= -I$(TOP_DIR)/include/asm/$(CPU)
INCS		+= -I$(TOP_DIR)/include/asm/$(CPU)/$(DEST_OS)
INCS		+= -I$(TOP_DIR)/include/osa
INCS		+= -I$(TOP_DIR)/include/osa/$(DEST_OS)
INCS		+= -I$(TOP_DIR)/bsp/$(BSP_NAME)
INCS		+= -I.

DEFINES		 =
DEFINES		+= -DIN_GUMSTIX
DEFINES		+= -DDEST_OS=$(DEST_OS)
DEFINES		+= -Datom=0x1000
DEFINES		+= -DLITTLE_ENDIAN=0x1001
DEFINES		+= -DBIG_ENDIAN=0x1002
DEFINES		+= -DENDIAN_TYPE=$(ENDIAN_TYPE)
DEFINES		+= -D_ATOM_=1
DEFINES		+= -D_EMBEDDED_=1
DEFINES		+= -DUSE_OSA=1
DEFINES		+= -DUSE_OSAMEM=1
DEFINES		+= -D__ARM__
DEFINES		+= -DUSE_HIGH_VECTOR=0
DEFINES		+= -DHAS_ASSEM_CLZ=1

CC_OPTIM	?= -gdwarf-2 -O2
CC_ARCH_SPEC = -mapcs -march=armv5 -mtune=xscale
CC_ARCH_SPEC+= -msoft-float

CFLAGS		 =
CFLAGS		+= $(CC_OPTIM) $(CC_ARCH_SPEC)
CFLAGS		+= -nostdinc
CFLAGS		+= $(INCS)
CFLAGS		+= -Wall
#CFLAGS		+= -Wstrict-prototypes
#CFLAGS		+= -Wparentheses
CFLAGS		+= -Wno-trigraphs -O0
CFLAGS		+= -fno-common
CFLAGS		+= -Wno-unused-but-set-variable
#CFLAGS		+= -Wreturn-type
#CFLAGS		+= -Wundef
#CFLAGS		+= -Wuninitialized
#CFLAGS		+= -Wunused-function
#CFLAGS		+= -Wunused-value
#CFLAGS		+= -Wunused-variable
#CFLAGS		+= -Werror-implicit-function-declaration
CFLAGS		+= -Wno-maybe-uninitialized
CFLAGS		+= -pipe
#CFLAGS		+= -mshort-load-bytes
CFLAGS		+= -fno-builtin
CFLAGS		+= -fno-strict-aliasing
CFLAGS		+= -fno-stack-protector
CFLAGS		+= -fshort-enums
CFLAGS		+= $(DEFINES)

CFLAGS_AS	 =
CFLAGS_AS	+= $(CC_OPTIM) $(CC_ARCH_SPEC)
CFLAGS_AS	+= $(INCS)
CFLAGS_AS	+= -fno-builtin -P
CFLAGS_AS	+= -xassembler-with-cpp
CFLAGS_AS	+= $(DEFINES)
CFLAGS_AS	+= -D__ASSEMBLY__

CXXFLAGS	 =
CXXFLAGS	+= $(CFLAGS)

LDFLAGS		 = -static -nostdlib -nostartfiles -nodefaultlibs -p -X -T $(TOP_DIR)/bsp/$(BSP_NAME)/$(DEST_OS).x

OCFLAGS		 = -O binary -R .note -R .comment -S
