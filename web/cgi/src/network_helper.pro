LIBS+= -lpthread
QMAKE_CXXFLAGS += -g
CONFIG-=qt
QMAKE_CXXFLAGS+= '-DDEBUG'

HEADERS += \
    unix_socket_helper.h \
    CJsonObject.hpp \
    cJSON.h \
    ../lib/cgic.h


SOURCES += \
    unix_socket_helper.cpp \
    CJsonObject.cpp \
    web_function.cpp \
    cJSON.c

INCLUDEPATH += \


