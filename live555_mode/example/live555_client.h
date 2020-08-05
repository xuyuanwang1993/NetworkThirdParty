#ifndef LIVE555_CLIENT_H
#define LIVE555_CLIENT_H
#include "live555_common.h"
#include "API_RtspServer.h"
#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"
#define DEBUG_PRINT_NPT 1

#define LIVE_SAVE_STREAM_TO_FILE 0
// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:
// If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
// and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
// then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
/*
    env->reclaim(); env = NULL;
    delete scheduler; scheduler = NULL;
  */

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
    uint32_t m_stream_index;
    bool m_is_setup=false;
    Api_rtsp_server::Media_Info m_media_info;
    uint32_t m_session_id;
};
// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:
class ourRTSPClient: public RTSPClient {
public:
    static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,uint32_t stream_index,
                                    int verbosityLevel = 0,
                                    char const* applicationName = NULL,
                                    portNumBits tunnelOverHTTPPortNum = 0);
    // The main streaming routine (for each "rtsp://" URL):
    static void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL,uint32_t stream_index);
    // Used to shut down and close a stream (including its "RTSPClient" object):
    static void shutdownStream(RTSPClient* rtspClient);
protected:
    ourRTSPClient(UsageEnvironment& env, char const* rtspURL,uint32_t stream_index,
                  int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
    virtual ~ourRTSPClient();

public:
    StreamClientState scs;
};
// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class H264or5DummySink: public MediaSink {
public:
    static H264or5DummySink* createNew(UsageEnvironment& env,
                                       MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id,// identifies the kind of data that's being received
                                       char const* streamId = NULL); // identifies the stream itself (optional)

private:
    H264or5DummySink(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs, uint32_t channel_id,char const* streamId);
    // called only by "createNew()"
    virtual ~H264or5DummySink();

    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                           struct timeval presentationTime, unsigned durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;
    StreamClientState &m_scs;
    uint32_t m_channel_id;
    std::string m_stream_name;
#if LIVE_SAVE_STREAM_TO_FILE
    FILE *m_save_fp;
#endif
};

class G711aDummySink: public MediaSink {
public:
    static G711aDummySink* createNew(UsageEnvironment& env,
                                     MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id,// identifies the kind of data that's being received
                                     char const* streamId = NULL); // identifies the stream itself (optional)

private:
    G711aDummySink(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs, uint32_t channel_id,char const* streamId);
    // called only by "createNew()"
    virtual ~G711aDummySink();

    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                           struct timeval presentationTime, unsigned durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;
    StreamClientState &m_scs;
    uint32_t m_channel_id;
    std::string m_stream_name;
#if LIVE_SAVE_STREAM_TO_FILE
    FILE *m_save_fp;
#endif
};
class AACDummySink: public MediaSink {
public:
    static AACDummySink* createNew(UsageEnvironment& env,
                                   MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id,\
                                   uint32_t sampleRate,uint8_t channel_num,uint8_t profile_id,/*identifies the kind of data that's being received*/\
                                   char const* streamId = nullptr); // identifies the stream itself (optional)

private:
    AACDummySink(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs, uint32_t channel_id,\
                 uint32_t sampleRate,uint8_t channel_num,uint8_t profile_id,char const* streamId);
    // called only by "createNew()"
    virtual ~AACDummySink();

    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                           struct timeval presentationTime, unsigned durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;
    StreamClientState &m_scs;
    uint32_t m_channel_id;
    std::string m_stream_name;
    uint32_t m_sampleRate;
    uint8_t m_channel_num;
    uint8_t m_profile_id;
    uint8_t m_index;
#if LIVE_SAVE_STREAM_TO_FILE
    uint8_t m_aac_header[7];
    FILE *m_save_fp;
#endif
};
#endif // LIVE555_CLIENT_H
