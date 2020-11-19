HEADERS += \
    ../io_output/io_output.h \
    ../bits_helper/bits_helper.h \
    ../observer/iobserver.h \
    ../PS_Stream_Demux/ps_demux.h

SOURCES += \
    main.cpp \
    ../io_output/io_output.cpp \
    ../bits_helper/bits_helper.cpp \
    ../PS_Stream_Demux/ps_demux.cpp

 MOC_DIR +=.moc
RCC_DIR +=.rcc
OBJECTS_DIR +=.obj
LIBS += -lpthread
CONFIG-=qt
INCLUDEPATH += ../ \
../io_output \
../bits_helper/ \
../observer \

QMAKE_CXXFLAGS+= '-DDEBUG'
