#Makefile

CROSS_COMPILE ?=/home/microcreat/Desktop/workdir/HI3616A/arm-hisiv300-linux/bin/arm-hisiv300-linux-uclibcgnueabi-
#CROSS_COMPILE ?= /workdir_ext/hi3516/Input_hi3516A_Distributed_Processor/MA_M_V0005R0001/.cross_tools/arm-hisiv300-linux/bin/arm-hisiv300-linux-uclibcgnueabi-
LIB_OUT_DIR=out
LIB_SUFIX=.a
DLL_SUFIX=.so
LINK_OPTS=
OBJS_PATH = $(LIB_OUT_DIR)/lib_objs
CXX   = $(CROSS_COMPILE)g++
CC    = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip
#
LIBRARY_LINK =		$(CROSS_COMPILE)ar -cr 
LIBRARY_LINK_OPTS =	$(LINK_OPTS)
SHARED_LIB_COMPILE= $(CXX) -shared -fPIC
TARGET = libRtspserver



INC  = -I./LOG -I./events_handler -I./network_helper -I./RtspServer/rtsp -I./OtherTools/MD5 -I./RtspServer/API
LIB  =

LD_FLAGS  = -Wl,--gc-sections -lrt -pthread -lpthread -ldl -lm $(DEBUG)
CXX_FLAGS = -O2 -std=c++11 -ffunction-sections -fdata-sections -fPIC

SRC  = $(notdir $(wildcard ./LOG/*.cpp))
SRC  += $(notdir $(wildcard ./events_handler/*.cpp))
SRC  += $(notdir $(wildcard ./network_helper/*.cpp))
SRC  += $(notdir $(wildcard ./RtspServer/rtsp/*.cpp))
SRC  += $(notdir $(wildcard ./OtherTools/MD5/*.cpp))
SRC  += $(notdir $(wildcard ./RtspServer/API/*.cpp))
OBJS = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC))

all:  COPY

BUILD_DIR:
	@-mkdir -p $(LIB_OUT_DIR)
	@-mkdir -p $(OBJS_PATH)
	@-mkdir -p $(LIB_OUT_DIR)/lib
	@-mkdir -p $(LIB_OUT_DIR)/include
	@echo "It's creating object files,please wait some seconds!"
$(TARGET) : $(OBJS)
	-$(SHARED_LIB_COMPILE) -o $(LIB_OUT_DIR)/lib/$@$(DLL_SUFIX) $^ 
	-$(LIBRARY_LINK) $(LIB_OUT_DIR)/lib/$@$(LIB_SUFIX) $^
$(OBJS_PATH)/%.o : ./LOG/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./events_handler/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./network_helper/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./RtspServer/rtsp/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./OtherTools/MD5/%.cpp
	@$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./RtspServer/API/%.cpp
	@$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
COPY : BUILD_DIR $(TARGET)
	@-cp -rf ./LOG/*.h $(LIB_OUT_DIR)/include/net
	@-cp -rf ./events_handler/*.h $(LIB_OUT_DIR)/include/
	@-cp -rf ./network_helper/*.h $(LIB_OUT_DIR)/include/
	@-cp -rf ./RtspServer/rtsp/*.h $(LIB_OUT_DIR)/include/
	@-cp -rf ./OtherTools/MD5/*.h $(LIB_OUT_DIR)/include/
	@-cp -rf ./RtspServer/API/*.h 	  $(LIB_OUT_DIR)/include
clean:
	-rm -rf $(LIB_OUT_DIR) 
