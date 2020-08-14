TARGET = core_test
HEADERS += \
    ../CJsonObject.hpp \
    ../cJSON.h \
    ../ConfigUtils.h

SOURCES += \
    demo.cpp \
    ../CJsonObject.cpp \
    ../cJSON.c \
    ../ConfigUtils.cpp


CONFIG-=qt
QMAKE_CXXFLAGS+= '-std=c++11' -g -DDEBUG

INCLUDEPATH += ../

DISTFILES +=
