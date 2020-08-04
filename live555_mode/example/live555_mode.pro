HEADERS += \
    live555_client.h \
    live555_common.h \
    byte_cache_buf.h

MOC_DIR +=.moc
RCC_DIR +=.rcc
OBJECTS_DIR +=.obj

LIBS+= /usr/local/lib/libliveMedia.a \
/usr/local/lib/libgroupsock.a \
/usr/local/lib/libBasicUsageEnvironment.a \
/usr/local/lib/libUsageEnvironment.a \
./libRtspserver.so \
-lpthread \

QMAKE_CXXFLAGS += -g -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
QMAKE_LFLAGS +=
SOURCES += \
    main.cpp \
    live555_client.cpp \
    byte_cache_buf.cpp

INCLUDEPATH += . \
.. \
/usr/local/include/liveMedia \
/usr/local/include/BasicUsageEnvironment \
/usr/local/include/UsageEnvironment \
/usr/local/include/groupsock \
/usr/local/include/openssl \


CONFIG-=qt

QMAKE_CXXFLAGS+= '-DDEBUG'
