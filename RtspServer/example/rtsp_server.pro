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
    ../../network_helper/tcp_server.h \
    ../../network_helper/tcp_connection.h \
    ../../network_helper/buffer_handle.h \
    ../rtsp/media_source.h \
    ../rtsp/rtp.h \
    ../rtsp/media.h \
    ../rtsp/aac_source.h \
    ../rtsp/g711a_source.h \
    ../rtsp/h264_source.h \
    ../rtsp/h265_source.h \
    ../rtsp/rtsp_message.h \
    ../rtsp/rtsp_helper.h \
    ../../OtherTools/MD5/MD5.h \
    ../rtsp/rtsp_server.h \
    ../rtsp/rtsp_connection.h \
    ../rtsp/rtp_connection.h \
    ../rtsp/media_session.h \
    file_reader.h \
    ../../network_helper/tcp_connection_helper.h \
    ../API/API_RtspServer.h

LIBS+= -lpthread

QMAKE_CXXFLAGS += -g -std=c++11

SOURCES += \
    main.cpp \
    ../../LOG/c_log.cpp \
    ../../events_handler/trigger_event.cpp \
    ../../events_handler/timer_queue.cpp \
    ../../events_handler/thread_pool.cpp \
    ../../events_handler/task_scheduler.cpp \
    ../../events_handler/pipe.cpp \
    ../../events_handler/network_util.cpp \
    ../../events_handler/event_loop.cpp \
    ../../network_helper/tcp_server.cpp \
    ../../network_helper/tcp_connection.cpp \
    ../../network_helper/buffer_handle.cpp \
    ../rtsp/media_source.cpp \
    ../rtsp/aac_source.cpp \
    ../rtsp/g711a_source.cpp \
    ../rtsp/h264_source.cpp \
    ../rtsp/h265_source.cpp \
    ../rtsp/rtsp_message.cpp \
    ../rtsp/rtsp_helper.cpp \
    ../../OtherTools/MD5/MD5.cpp \
    ../rtsp/rtsp_server.cpp \
    ../rtsp/rtsp_connection.cpp \
    ../rtsp/rtp_connection.cpp \
    ../rtsp/media_session.cpp \
    file_reader.cpp \
    ../../network_helper/tcp_connection_helper.cpp \
    ../API/API_RtspServer.cpp

INCLUDEPATH += ..\
../../LOG \
../../events_handler \
../../network_helper/\
../rtsp\
../../OtherTools/MD5/\
../API/ \

CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
