HEADERS += \
    ../ikcp.h \
    ../../LOG/c_log.h \
    ../../events_handler/trigger_event.h \
    ../../events_handler/timer_queue.h \
    ../../events_handler/thread_pool.h \
    ../../events_handler/task_scheduler.h \
    ../../events_handler/pipe.h \
    ../../events_handler/network_util.h \
    ../../events_handler/io_channel.h \
    ../../events_handler/event_loop.h \
    ../kcp_common.h \
    ../kcp_proxy_interface.h \
    ../kcp_proxy_server.h \
    ../kcp_proxy_connection.h \
    ../irudp_over_kcp_common.h \
    ../irudp_over_kcp_server.h

LIBS+= -lpthread

QMAKE_CXXFLAGS += -g

SOURCES += \
    main.cpp \
    ../ikcp.c \
    ../../LOG/c_log.cpp \
    ../../events_handler/trigger_event.cpp \
    ../../events_handler/timer_queue.cpp \
    ../../events_handler/thread_pool.cpp \
    ../../events_handler/task_scheduler.cpp \
    ../../events_handler/pipe.cpp \
    ../../events_handler/network_util.cpp \
    ../../events_handler/event_loop.cpp \
    ../kcp_common.cpp \
    ../kcp_proxy_interface.cpp \
    ../kcp_proxy_server.cpp \
    ../kcp_proxy_connection.cpp \
    ../irudp_over_kcp_common.cpp \
    ../irudp_over_kcp_server.cpp

INCLUDEPATH += ..\
../../LOG \
../../events_handler \

CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
