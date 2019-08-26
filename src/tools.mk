ifndef CROSS
$(error "Crosstool is not defined!")
endif
CC			 = $(CROSS)-gcc
CXX			 = $(CROSS)-g++
LD			 = $(CROSS)-ld
OC			 = $(CROSS)-objcopy
AR			 = $(CROSS)-ar
NM			 = $(CROSS)-nm


CCDV		 = $(UTIL_DIR)/ccdv$(EXT)
DUMPSYM		 = $(UTIL_DIR)/dumpsym
ECHO		 = echo
RM			 = rm -f
RMDIR		 = rm -rf
TOUCH		 = touch
FIND		 = find
