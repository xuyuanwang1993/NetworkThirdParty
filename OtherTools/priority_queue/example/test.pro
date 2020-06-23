CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
INCLUDEPATH += \
../
HEADERS += \
    ../priority_queue.h

SOURCES += \
    main.cpp
