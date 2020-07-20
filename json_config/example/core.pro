TARGET = core_test
HEADERS += \
    ../CJsonObject.hpp \
    ../cJSON.h

SOURCES += \
    demo.cpp \
    ../CJsonObject.cpp \
    ../cJSON.c


CONFIG-=qt
QMAKE_CXXFLAGS+= '-std=c++11' -g -DDEBUG

INCLUDEPATH += ../

DISTFILES +=
