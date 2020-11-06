######################################################################################
# Web
####################
CROSS_COMPILE ?=
CROSS_CFLAGS ?=
####################

CC:=$(CROSS_COMPILE)g++
CXX:=$(CROSS_COMPILE)g++
AR:=$(CROSS_COMPILE)ar
STRIP:=$(CROSS_COMPILE)strip
RANLIB:=$(CROSS_COMPILE)ranlib

ROOT_DIR := $(shell pwd)
SRC_DIR := $(ROOT_DIR)/src

INCLUDE_FLAG = -I./include -I./src
LIB_FLAG := -L./ -lcgic

TARGET := web_function.cgi

C_SRCS += $(wildcard $(SRC_DIR)/*.c)
CPP_SRCS += $(wildcard $(SRC_DIR)/*.cpp)

OBJS += $(patsubst %.c,%.o,$(C_SRCS))
OBJS += $(patsubst %.cpp,%.o,$(CPP_SRCS))

LIB_SRCS := lib/cgic.c
LIB_OBJS := $(patsubst %.c,%.o,$(LIB_SRCS))
CFLAGS ?=
CXXFLAGS += $(CFLAGS) -std=c++11
.PHONY: all app lib clean
all: app lib

app:$(OBJS)
	@$(CXX) $(CROSS_CFLAGS) -o $(TARGET) $(OBJS) $(LIB_FLAG)
	@$(STRIP) $(TARGET)

$(SRC_DIR)/%.o : $(SRC_DIR)/%.c
	@$(CC) -c $(CROSS_CFLAGS) $< -o $@ $(CFLAGS)
$(SRC_DIR)/%.o : $(SRC_DIR)/%.cpp
	$(CXX) -c $(CROSS_CFLAGS) $< -o $@ $(CXXFLAGS)

lib: $(LIB_OBJS)
	$(AR) rc libcgic.a $(LIB_OBJS)
	$(RANLIB) libcgic.a
	$(CC) $(CROSS_CFLAGS) -fPIC -shared $(LIB_OBJS) -o libcgic.so

$(LIB_OBJS): %.o:%.c
	@$(CC) -c $(CROSS_CFLAGS) $< -o $@

clean:
	@-rm -rf $(TARGET) libcgic.* $(OBJS) $(LIB_OBJS)
