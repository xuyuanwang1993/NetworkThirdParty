#sCROSS_PATH_PREFIX=/home/microcreat/Desktop/workdir/xyw/IOT_Gateway_V008_HI3536D_V1/.cross_tools/arm-hisiv510-linux/bin/arm-hisiv510-linux-uclibcgnueabi-
CROSS_PATH_PREFIX=
QMAKE_CC = $${CROSS_PATH_PREFIX}g++
QMAKE_CXX = $${CROSS_PATH_PREFIX}g++
QMAKE_LINK = $${CROSS_PATH_PREFIX}g++

CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
INCLUDEPATH += \
../../../MD5\
../
HEADERS += \
    ../rc4_interface.h \
    ../../../MD5/MD5.h

SOURCES += \
    ../rc4_interface.cpp \
    ../../../MD5/MD5.cpp \
    main.cpp
