HEADERS += \
    core_server.h \
    ../../out/include/Idns_server/dns_server.h \
    ../../out/include/lb_server/load_balance_server.h \
    ../../out/include/netcore/trigger_event.h \
    ../../out/include/netcore/timer_queue.h \
    ../../out/include/netcore/timeout_session_task.h \
    ../../out/include/netcore/thread_pool.h \
    ../../out/include/netcore/task_scheduler.h \
    ../../out/include/netcore/pipe.h \
    ../../out/include/netcore/network_util.h \
    ../../out/include/netcore/io_channel.h \
    ../../out/include/netcore/event_loop.h \
    ../../out/include/c_log/c_log.h \
    ../../out/include/json_config/ConfigUtils.h \
    ../../out/include/json_config/CJsonObject.hpp \
    ../../out/include/json_config/cJSON.h \
    ../../out/include/Iother_tools/rc4_interface.h \
    ../../out/include/Iother_tools/priority_queue.h \
    ../../out/include/Iother_tools/MD5.h \
    ../../out/include/Iother_tools/dns.h \
    ../../out/include/Iother_tools/delay_control.h \
    ../../out/include/Iother_tools/Base64.h \
    ../../out/include/Idaemon/daemon_instance.h

SOURCES += \
    core_server.cpp \
    example/main.cpp


INCLUDEPATH+= \
../../out/include/json_config/ \
../../out/include/c_log/ \
../../out/include/netcore/ \
../../out/include/lb_server/ \
../../out/include/Idns_server/ \
../../out/include/Iother_tools/ \
../../out/include/Idaemon/ \

CONFIG-=qt

