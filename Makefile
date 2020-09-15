SHELL = /bin/bash
#交叉编译工具前缀
CROSS_COMPILE ?=
#安装目录前缀
PREFIX ?= /usr/local
#编译输出目录
OUT_PATH ?= $(shell pwd)/out

#库前缀
LIB_PREFIX ?= lib
#静态库后缀
STATIC_LIB_SUFIX ?= a
#动态库后缀
DLL_LIB_SUFIX ?= so
#链接选项
LINK_OPTS ?= -rdynamic 
#C编译选项
CFLAGS ?= -DDEBUG
CFLAGS += -O2 -Wl,--gc-sections -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 -fPIC
#C++编译选项
CXXFLAGS ?=
CXXFLAGS +=$(CFLAGS) -std=c++11 -ffunction-sections -fdata-sections
#覆盖下层配置
export
#工作子目录
SUBDIR += LOG \
events_handler \
json_config \
kcp \
OtherTools \
OtherTools/UPNP \
OtherTools/daemon \
load_balancing_mode/client \
load_balancing_mode/server \
my_ddns/dns_client \
my_ddns/dns_server \
network_helper \
RtspServer \
RtspProxy \
utils \
applications/core_server \
sqlite \
web \

.PHONY :all  lib app clean help distclean install uninstall
all : lib app
	@for subdir in $(SUBDIR); \
	do \
		pushd $$subdir >/dev/null; \
		make  -e -f Makefile.cross; \
		popd >/dev/null; \
	done 
app : lib
	@for subdir in $(SUBDIR); \
	do \
		pushd $$subdir >/dev/null; \
		make  -e -f Makefile.cross app; \
		popd >/dev/null; \
	done 
lib :  
	@for subdir in $(SUBDIR); \
	do \
		pushd $$subdir >/dev/null; \
		make  -e -f Makefile.cross lib; \
		popd >/dev/null; \
	done 
help :
	@echo "subdir=$(SUBDIR)"
	@echo "support cmd {'prepare' 'lib' 'app' 'clean' 'distclean' 'install' 'uninstall'}"
	@echo '#####################################'
	@echo -e "\033[1;31moption_name \t default\t 功能描述\033[m"
	@echo '#####################################'
	@echo -e "CROSS_COMPILE\tnull \t 交叉编译工具前缀"
	@echo -e "PREFIX \t /usr/local \t 安装目录前缀"
	@echo -e "OUT_PATH \t ../out \t 编译输出目录"
	@echo -e "LIB_PREFIX \t lib \t 库前缀"
	@echo -e "STATIC_LIB_SUFIX \t .a \t 静态库后缀"
	@echo -e "SHARED_LIB_COMPILE \t .so \t 动态库后缀"
	@echo -e "LINK_OPTS \t -rdynamic \t 链接选项"
	@echo -e "CFLAGS \t -DDEBUG \t C编译选项"
	@echo -e "CXXFLAGS \t -DDEBUG \t C++编译选项"
clean :
	@for subdir in $(SUBDIR); \
	do \
		pushd $$subdir >/dev/null; \
		make  -e -f Makefile.cross clean; \
		popd >/dev/null; \
	done 
distclean :
	-rm $(OUT_PATH) -rf
install : lib app
	@for subdir in $(SUBDIR); \
	do \
		pushd $$subdir >/dev/null; \
		make  -e -f Makefile.cross install; \
		popd >/dev/null; \
	done 
uninstall :
	@for subdir in $(SUBDIR); \
	do \
		pushd $$subdir >/dev/null; \
		make  -e -f Makefile.cross uninstall; \
		popd >/dev/null; \
	done 
