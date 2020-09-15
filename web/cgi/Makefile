######################################################################################
# Web
####################
CROSS_COMPILE ?= 
CROSS_CFLAGS ?=
####################

CC:=$(CROSS_COMPILE)gcc
AR:=$(CROSS_COMPILE)ar
STRIP:=$(CROSS_COMPILE)strip

ROOT_DIR := $(shell pwd)
SRC_DIR =  $(ROOT_DIR)/src

INCLUDE_FLAG = -I./include -I./src
LIB_FLAG = ./lib/libcgic.a

TARGET := web_function.cgi

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))


.PHONY: all clean
all:$(OBJS)
	@$(CC) $(CROSS_CFLAGS) -o $(TARGET) $(OBJS) $(LIB_FLAG)
	@$(STRIP) $(TARGET)
	
$(OBJS): %.o:%.c
	@$(CC) -c $(CROSS_CFLAGS) $< -o $@

clean:
	@-rm -rf $(TARGET) $(OBJS)
