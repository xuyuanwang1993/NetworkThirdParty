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
    ../../OtherTools/MD5/MD5.h \
    ../rtspProxy/proxy_protocol.h \
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
    ../proxyServer/proxybufhandle.h \
    ../proxyClient/rtsp_pusher.h \
    ../proxyServer/proxy_server.h \
    ../proxyServer/proxy_connection.h \
    ../../json_config/CJsonObject.hpp \
    ../../json_config/cJSON.h \
    ../../network_helper/tcp_connection_helper.h \
    ../../RtspServer/example/file_reader.h \
    ../../OtherTools/DelayControl/delay_control.h \
    ../../OtherTools/UPNP/upnpmapper_mode.h \
    ../../OtherTools/UPNP/upnpmapper.h \
    ../../OtherTools/Base64/Base64.h

LIBS+= -lpthread

QMAKE_CXXFLAGS +=

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
    ../../OtherTools/MD5/MD5.cpp \
    ../rtspProxy/proxy_protocol.cpp \
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
    ../proxyServer/proxybufhandle.cpp \
    ../proxyClient/rtsp_pusher.cpp \
    ../proxyServer/proxy_server.cpp \
    ../proxyServer/proxy_connection.cpp \
    ../../json_config/CJsonObject.cpp \
    ../../json_config/cJSON.c \
    ../../network_helper/tcp_connection_helper.cpp \
    ../../RtspServer/example/file_reader.cpp \
    ../../OtherTools/DelayControl/delay_control.cpp \
    ../../OtherTools/UPNP/upnpmapper_mode.cpp \
    ../../OtherTools/UPNP/upnpmapper.cpp \
    ../../OtherTools/Base64/Base64.cpp

INCLUDEPATH += ..\
../../LOG \
../../events_handler \
../../network_helper/ \
../../OtherTools/MD5/ \
../rtspProxy/ \
../../RtspServer/rtsp/ \
../proxyServer/ \
../proxyClient/ \
../../json_config/ \
../../RtspServer/example/ \
../../OtherTools/DelayControl \
../../OtherTools/UPNP/ \
../../OtherTools/Base64/ \

CONFIG-=qt
QMAKE_CXXFLAGS+= '-DDEBUG'
