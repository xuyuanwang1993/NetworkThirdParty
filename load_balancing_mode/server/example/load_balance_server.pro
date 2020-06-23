SOURCES += \
    ../load_balance_server.cpp \
    ../../../events_handler/trigger_event.cpp \
    ../../../events_handler/timer_queue.cpp \
    ../../../events_handler/thread_pool.cpp \
    ../../../events_handler/task_scheduler.cpp \
    ../../../events_handler/pipe.cpp \
    ../../../events_handler/network_util.cpp \
    ../../../events_handler/event_loop.cpp \
    ../../../OtherTools/rc4/example/rc4.cpp \
    ../../../OtherTools/MD5/MD5.cpp \
    ../../../json_config/CJsonObject.cpp \
    ../../../json_config/cJSON.c \
    ../../../LOG/c_log.cpp \
    main.cpp



INCLUDEPATH += $$PWD/.\
 ../../../OtherTools/rc4/example\
 ../../../events_handler/ \
../../../OtherTools/MD5\
../../../json_config\
../../../LOG\
../

DEPENDPATH += $$PWD/.

CONFIG-=qt
QMAKE_CXXFLAGS +=
LIBS+= -lpthread
HEADERS += \
    ../load_balance_server.h \
    ../../../events_handler/trigger_event.h \
    ../../../events_handler/timer_queue.h \
    ../../../events_handler/thread_pool.h \
    ../../../events_handler/task_scheduler.h \
    ../../../events_handler/pipe.h \
    ../../../events_handler/network_util.h \
    ../../../events_handler/io_channel.h \
    ../../../events_handler/event_loop.h \
    ../../../OtherTools/rc4/example/rc4.h \
    ../../../OtherTools/MD5/MD5.h \
    ../../../json_config/CJsonObject.hpp \
    ../../../json_config/cJSON.h \
    ../../../LOG/c_log.h



