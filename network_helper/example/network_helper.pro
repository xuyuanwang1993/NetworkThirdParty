LIBS+= -lpthread -rdynamic
QMAKE_CXXFLAGS += -g
CONFIG-=qt
QMAKE_CXXFLAGS+= '-DDEBUG' -rdynamic '-DUTIL_TEST=1'

HEADERS += \
    ../../LOG/c_log.h \
    ../../events_handler/trigger_event.h \
    ../../events_handler/timer_queue.h \
    ../../events_handler/thread_pool.h \
    ../../events_handler/task_scheduler.h \
    ../../events_handler/pipe.h \
    ../../events_handler/network_util.h \
    ../../events_handler/io_channel.h \
    ../../events_handler/event_loop.h \
    ../buffer_handle.h \
    ../tcp_connection.h \
    ../tcp_server.h \
    ../http_request.h \
    ../http_response.h \
    ../tcp_connection_helper.h \
    ../unix_socket_helper.h \
    ../tcp_client.h \
    ../tcp_client_example.h \
    ../http_client.h \
    ../http_helper.h \
    ../http_send_buf.h \
    ../websocket_common.h \
    ../../OtherTools/Base64/Base64.h \
    ../websocket_client.h \
    ../websocket_buffer_cache.h \
    ../../OtherTools/MD5/MD5.h

SOURCES += \
    ../../LOG/c_log.cpp \
    ../../events_handler/trigger_event.cpp \
    ../../events_handler/timer_queue.cpp \
    ../../events_handler/thread_pool.cpp \
    ../../events_handler/task_scheduler.cpp \
    ../../events_handler/pipe.cpp \
    ../../events_handler/network_util.cpp \
    ../../events_handler/event_loop.cpp \
    main.cpp \
    ../buffer_handle.cpp \
    ../tcp_connection.cpp \
    ../tcp_server.cpp \
    ../http_request.cpp \
    ../http_response.cpp \
    ../tcp_connection_helper.cpp \
    ../unix_socket_helper.cpp \
    ../tcp_client.cpp \
    ../tcp_client_example.cpp \
    ../http_client.cpp \
    ../http_helper.cpp \
    ../http_send_buf.cpp \
    ../websocket_common.cpp \
    ../../OtherTools/Base64/Base64.cpp \
    ../websocket_client.cpp \
    ../websocket_buffer_cache.cpp \
    ../../OtherTools/MD5/MD5.cpp
INCLUDEPATH += \
../../LOG \
../../events_handler \
../ \
../../OtherTools/Base64 \
../../OtherTools/MD5/ \
