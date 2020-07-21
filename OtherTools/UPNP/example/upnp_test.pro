HEADERS += \
    ../upnpmapper.h \
    ../../../LOG/c_log.h \
    ../../../events_handler/trigger_event.h \
    ../../../events_handler/timer_queue.h \
    ../../../events_handler/thread_pool.h \
    ../../../events_handler/task_scheduler.h \
    ../../../events_handler/pipe.h \
    ../../../events_handler/network_util.h \
    ../../../events_handler/io_channel.h \
    ../../../events_handler/event_loop.h \
    ../upnpmapper_mode.h
SOURCES += \
    ../upnpmapper.cpp \
    ../../../LOG/c_log.cpp \
    ../../../events_handler/trigger_event.cpp \
    ../../../events_handler/timer_queue.cpp \
    ../../../events_handler/thread_pool.cpp \
    ../../../events_handler/task_scheduler.cpp \
    ../../../events_handler/pipe.cpp \
    ../../../events_handler/network_util.cpp \
    ../../../events_handler/event_loop.cpp \
    main.cpp \
    ../upnpmapper_mode.cpp
 MOC_DIR +=.moc
RCC_DIR +=.rcc
OBJECTS_DIR +=.obj
LIBS += -lpthread
CONFIG-=qt
INCLUDEPATH += ../ \
../../../LOG \
../../../events_handler \

QMAKE_CXXFLAGS+= '-DDEBUG'
