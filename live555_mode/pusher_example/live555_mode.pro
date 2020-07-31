HEADERS += \
    live555_client.h \
    live555_common.h \
    ../../network_helper/tcp_server.h \
    ../../network_helper/tcp_connection_helper.h \
    ../../network_helper/tcp_connection.h \
    ../../network_helper/http_response.h \
    ../../network_helper/http_request.h \
    ../../network_helper/buffer_handle.h \
    ../../events_handler/trigger_event.h \
    ../../events_handler/timer_queue.h \
    ../../events_handler/thread_pool.h \
    ../../events_handler/task_scheduler.h \
    ../../events_handler/pipe.h \
    ../../events_handler/network_util.h \
    ../../events_handler/io_channel.h \
    ../../events_handler/event_loop.h \
    ../../OtherTools/MD5/MD5.h \
    ../../RtspProxy/rtspProxy/proxy_protocol.h \
    ../../RtspProxy/proxyClient/rtsp_pusher.h \
    ../../RtspServer/rtsp/rtsp_server.h \
    ../../RtspServer/rtsp/rtsp_message.h \
    ../../RtspServer/rtsp/rtsp_helper.h \
    ../../RtspServer/rtsp/rtsp_connection.h \
    ../../RtspServer/rtsp/rtp_connection.h \
    ../../RtspServer/rtsp/rtp.h \
    ../../RtspServer/rtsp/media_source.h \
    ../../RtspServer/rtsp/media_session.h \
    ../../RtspServer/rtsp/media.h \
    ../../RtspServer/rtsp/h265_source.h \
    ../../RtspServer/rtsp/h264_source.h \
    ../../RtspServer/rtsp/g711a_source.h \
    ../../RtspServer/rtsp/aac_source.h \
    ../../json_config/CJsonObject.hpp \
    ../../json_config/cJSON.h \
    ../../LOG/c_log.h \
    ../../RtspProxy/proxyServer/proxy_server.h \
    ../../RtspProxy/proxyServer/proxy_connection.h \
    ../../RtspProxy/proxyServer/proxybufhandle.h

MOC_DIR +=.moc
RCC_DIR +=.rcc
OBJECTS_DIR +=.obj

LIBS+= /usr/local/lib/libliveMedia.a \
/usr/local/lib/libgroupsock.a \
/usr/local/lib/libBasicUsageEnvironment.a \
/usr/local/lib/libUsageEnvironment.a \
-lpthread \

QMAKE_CXXFLAGS += -g -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
QMAKE_LFLAGS +=
SOURCES += \
    main.cpp \
    live555_client.cpp \
    ../../network_helper/tcp_server.cpp \
    ../../network_helper/tcp_connection_helper.cpp \
    ../../network_helper/tcp_connection.cpp \
    ../../network_helper/http_response.cpp \
    ../../network_helper/http_request.cpp \
    ../../network_helper/buffer_handle.cpp \
    ../../events_handler/trigger_event.cpp \
    ../../events_handler/timer_queue.cpp \
    ../../events_handler/thread_pool.cpp \
    ../../events_handler/task_scheduler.cpp \
    ../../events_handler/pipe.cpp \
    ../../events_handler/network_util.cpp \
    ../../events_handler/event_loop.cpp \
    ../../OtherTools/MD5/MD5.cpp \
    ../../RtspProxy/rtspProxy/proxy_protocol.cpp \
    ../../RtspProxy/proxyClient/rtsp_pusher.cpp \
    ../../RtspServer/rtsp/rtsp_server.cpp \
    ../../RtspServer/rtsp/rtsp_message.cpp \
    ../../RtspServer/rtsp/rtsp_helper.cpp \
    ../../RtspServer/rtsp/rtsp_connection.cpp \
    ../../RtspServer/rtsp/rtp_connection.cpp \
    ../../RtspServer/rtsp/media_source.cpp \
    ../../RtspServer/rtsp/media_session.cpp \
    ../../RtspServer/rtsp/h265_source.cpp \
    ../../RtspServer/rtsp/h264_source.cpp \
    ../../RtspServer/rtsp/g711a_source.cpp \
    ../../RtspServer/rtsp/aac_source.cpp \
    ../../json_config/CJsonObject.cpp \
    ../../json_config/cJSON.c \
    ../../LOG/c_log.cpp \
    ../../RtspProxy/proxyServer/proxy_server.cpp \
    ../../RtspProxy/proxyServer/proxy_connection.cpp \
    ../../RtspProxy/proxyServer/proxybufhandle.cpp

INCLUDEPATH += . \
.. \
/usr/local/include/liveMedia \
/usr/local/include/BasicUsageEnvironment \
/usr/local/include/UsageEnvironment \
/usr/local/include/groupsock \
/usr/local/include/openssl \
../../network_helper \
../../events_handler \
../../OtherTools/MD5 \
../../RtspProxy/rtspProxy \
../../RtspProxy/proxyClient \
../../RtspServer/rtsp \
../../json_config \
../../LOG \
../../RtspProxy/proxyServer \

CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
