HEADERS += \
    ../io_output/io_output.h

SOURCES += \
    main.cpp \
    ../io_output/io_output.cpp

 MOC_DIR +=.moc
RCC_DIR +=.rcc
OBJECTS_DIR +=.obj
LIBS += -lpthread
CONFIG-=qt
INCLUDEPATH += ../ \
../io_output \

QMAKE_CXXFLAGS+= '-DDEBUG'
