TARGET = core_test
HEADERS += \
    timer_queue.h \
    thread_pool.h \
    trigger_event.h \
    network_util.h \
    pipe.h \
    io_channel.h \
    task_scheduler.h \
    event_loop.h \
    example/test_server.h \
    ../LOG/c_log.h \
    timeout_session_task.h

SOURCES += \
    timer_queue.cpp \
    thread_pool.cpp \
    example/test_main.cpp \
    trigger_event.cpp \
    network_util.cpp \
    pipe.cpp \
    task_scheduler.cpp \
    event_loop.cpp \
    example/test_server.cpp \
    ../LOG/c_log.cpp \
    timeout_session_task.cpp

INCLUDEPATH+= \
../LOG

CONFIG-=qt
QMAKE_CXXFLAGS+= '-std=c++11' -g -DDEBUG

INCLUDEPATH += example
LIBS+=-lpthread

DISTFILES +=
