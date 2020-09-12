TARGET = core_test
HEADERS += \
    ../../out/include/rtsp_server/rtsp_server.h \
    ../../out/include/rtsp_server/rtsp_message.h \
    ../../out/include/rtsp_server/rtsp_helper.h \
    ../../out/include/rtsp_server/rtsp_connection.h \
    ../../out/include/rtsp_server/rtp_connection.h \
    ../../out/include/rtsp_server/rtp.h \
    ../../out/include/rtsp_server/media_source.h \
    ../../out/include/rtsp_server/media_session.h \
    ../../out/include/rtsp_server/media.h \
    ../../out/include/rtsp_server/h265_source.h \
    ../../out/include/rtsp_server/h264_source.h \
    ../../out/include/rtsp_server/g711a_source.h \
    ../../out/include/rtsp_server/file_reader.h \
    ../../out/include/rtsp_server/API_RtspServer.h \
    ../../out/include/rtsp_server/aac_source.h \
    ../../out/include/rtsp_proxy/rtsp_pusher.h \
    ../../out/include/rtsp_proxy/proxy_server.h \
    ../../out/include/rtsp_proxy/proxy_protocol.h \
    ../../out/include/rtsp_proxy/proxy_connection.h \
    ../../out/include/rtsp_proxy/proxybufhandle.h \
    ../../out/include/lb_client/load_balance_client.h \
    ../../out/include/c_log/c_log.h \
    ../../out/include/Idaemon/daemon_instance.h \
    ../../out/include/IUPNP/upnpmapper_mode.h \
    ../../out/include/IUPNP/upnpmapper.h \
    ../../out/include/network_helper/tcp_server.h \
    ../../out/include/network_helper/tcp_connection_helper.h \
    ../../out/include/network_helper/tcp_connection.h \
    ../../out/include/network_helper/http_response.h \
    ../../out/include/network_helper/http_request.h \
    ../../out/include/network_helper/buffer_handle.h \
    ../../out/include/netcore/trigger_event.h \
    ../../out/include/netcore/timer_queue.h \
    ../../out/include/netcore/timeout_session_task.h \
    ../../out/include/netcore/thread_pool.h \
    ../../out/include/netcore/task_scheduler.h \
    ../../out/include/netcore/pipe.h \
    ../../out/include/netcore/network_util.h \
    ../../out/include/netcore/io_channel.h \
    ../../out/include/netcore/event_loop.h \
    ../../out/include/json_config/ConfigUtils.h \
    ../../out/include/json_config/CJsonObject.hpp \
    ../../out/include/json_config/cJSON.h \
    ../../out/include/Iother_tools/rc4_interface.h \
    ../../out/include/Iother_tools/priority_queue.h \
    ../../out/include/Iother_tools/MD5.h \
    ../../out/include/Iother_tools/dns.h \
    ../../out/include/Iother_tools/delay_control.h \
    ../../out/include/Iother_tools/Base64.h \
    proxy_server_mode.h \
    ../../out/include/Idns_client/dns_client.h

SOURCES += \
    proxy_server_mode.cpp \
    example/main.cpp


INCLUDEPATH+= \
../../out/include/rtsp_server \
../../out/include/rtsp_proxy \
../../out/include/lb_client \
../../out/include/c_log/ \
../../out/include/Idaemon \
../../out/include/IUPNP \
../../out/include/network_helper \
../../out/include/netcore \
../../out/include/json_config \
../../out/include/Iother_tools \
../../out/include/Idns_client \

CONFIG-=qt

