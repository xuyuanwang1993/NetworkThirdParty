CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
INCLUDEPATH += \
../../../MD5\
../
HEADERS += \
    ../rc4.h \
    ../../../MD5/MD5.h

SOURCES += \
    ../rc4.cpp \
    ../../../MD5/MD5.cpp \
    main.cpp
