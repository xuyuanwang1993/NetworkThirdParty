HEADERS += \
    ../proxy_protocol.h \
    ../network_util.h \
    ../c_log.h


LIBS+= -lpthread

QMAKE_CXXFLAGS += -g

SOURCES += \
    ../proxy_protocol.cpp \
    ../network_util.cpp \
    ../c_log.cpp \
    ../main.cpp


INCLUDEPATH += ..\

CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
