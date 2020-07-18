CONFIG-=qt

LIBS+= -lpthread
QMAKE_CXXFLAGS+= '-O2' '-DDEBUG'
INCLUDEPATH += \
../ \
../../../json_config \
../../../LOG \

HEADERS += \
    ../../../LOG/c_log.h \
    ../../../json_config/CJsonObject.hpp \
    ../../../json_config/cJSON.h \
    ../daemon_instance.h

SOURCES += \
    main.cpp \
    ../../../LOG/c_log.cpp \
    ../../../json_config/CJsonObject.cpp \
    ../../../json_config/cJSON.c \
    ../daemon_instance.cpp

DISTFILES += \
    ../../../shellScript/check_pro_port_status.sh \
    ../../../shellScript/check_port_owner.sh
