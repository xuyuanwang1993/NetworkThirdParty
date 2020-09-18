######################################################################################
# Web
####################
CROSS_COMPILE ?=
CROSS_CFLAGS ?=
####################

CC:=$(CROSS_COMPILE)gcc
AR:=$(CROSS_COMPILE)ar
STRIP:=$(CROSS_COMPILE)strip
RANLIB:=$(CROSS_COMPILE)ranlib

ROOT_DIR := $(shell pwd)
SRC_DIR := $(ROOT_DIR)/src

INCLUDE_FLAG = -I./include -I./src
LIB_FLAG := -L./ -lcgic

TARGET := web_function.cgi

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))

LIB_SRCS := lib/cgic.c
LIB_OBJS := $(patsubst %.c,%.o,$(LIB_SRCS))

.PHONY: all app lib clean
all: app lib

app:$(OBJS)
	@$(CC) $(CROSS_CFLAGS) -o $(TARGET) $(OBJS) $(LIB_FLAG)
	@$(STRIP) $(TARGET)

$(OBJS): %.o:%.c
	@$(CC) -c $(CROSS_CFLAGS) $< -o $@

lib: $(LIB_OBJS)
	$(AR) rc libcgic.a $(LIB_OBJS)
	$(RANLIB) libcgic.a
	$(CC) $(CROSS_CFLAGS) -fPIC -shared $(LIB_OBJS) -o libcgic.so

$(LIB_OBJS): %.o:%.c
	@$(CC) -c $(CROSS_CFLAGS) $< -o $@

clean:
	@-rm -rf $(TARGET) libcgic.* $(OBJS) $(LIB_OBJS)
