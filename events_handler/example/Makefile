TARGET=io_core_test
DEBUG ?=
OBJ_DIR = .OBJS
LD_FLAGS = -lpthread
CFLAGS += -g -DDEBUG
CXX_FLAGS += -std=c++11 
WORK_DIR = .\
	..\

LOG_PATH = ../../LOG
#define include path
INCLUDE_PATH = $(patsubst %,-I%,$(WORK_DIR))

SOURCE += $(patsubst %,%/*.cpp,$(WORK_DIR))
SOURCE += $(patsubst %,%/*.c,$(WORK_DIR))
SOURCE += $(patsubst %,%/*.h,$(WORK_DIR))
#define all source
APP_SOURCE = $(filter %.c %.cpp ,$(wildcard $(SOURCE)))
#define all objects target
ALL_OBJECTS = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(basename $(notdir $(APP_SOURCE)))))
ALL_OBJECTS += $(OBJ_DIR)/c_log.o
ALL: prepare $(TARGET)
.PHONY : prepare
prepare:
	@-mkdir -p $(OBJ_DIR)
	@echo "\033[32mIt's creating object files,please wait some seconds!\r\n\033[0m"
.PHONY : target
$(TARGET) : $(ALL_OBJECTS)
	g++ $^ -o $@ $(INCLUDE_PATH)-I$(LOG_PATH) $(LD_FLAGS) $(CFLAGS)
$(OBJ_DIR)/%.o : ./%.cpp
	@g++ -c $< -o $@ $(CXX_FLAGS) $(CFLAGS) $(INCLUDE_PATH) -I$(LOG_PATH)
$(OBJ_DIR)/%.o : ../%.cpp
	@g++ -c $< -o $@ $(CXX_FLAGS) $(CFLAGS) $(INCLUDE_PATH) -I$(LOG_PATH)
$(OBJ_DIR)/c_log.o : ../../LOG/c_log.cpp
	g++ -c $< -o $@ $(CXX_FLAGS) $(CFLAGS) -I$(LOG_PATH)
clean:
	@-rm $(TARGET)
	@-rm -rf $(OBJ_DIR)
