#include "live555_client.h"
#include<string>
#include<exception>
#include<cstdlib>
#include<stdexcept>
using namespace std;
// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0
//you can rewrite it！
string get_stream_prefix()
{
    return "test_stream_";
}
shared_ptr<Api_rtsp_server::Rtsp_Handle> g_handle;
// Forward function definitions:

// RTSP 'response handlers':

static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
static void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
static void subsessionByeHandler(void* clientData, char const* reason);
// called when a RTCP "BYE" is received for a subsession
static void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// Used to iterate through each stream's 'subsessions', setting up each one:
static void setupNextSubsession(RTSPClient* rtspClient);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
    env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
    env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

static char eventLoopWatchVariable = 0;
void delay_task(void *ptr)
{
    UsageEnvironment* env=(UsageEnvironment*)ptr;
    timeval time_now;
    gettimeofday(&time_now,nullptr);
    printf("%ld   %ld \r\n",time_now.tv_sec,time_now.tv_usec);
    env->taskScheduler().scheduleDelayedTask(40000,delay_task,ptr);
}
int test_client(int argc, char** argv) {
    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    // We need at least one "rtsp://" URL argument:
    if (argc < 2) {
        usage(*env, argv[0]);
        return 1;
    }

    // There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
    for (int i = 1; i <= argc-1; ++i) {
        ourRTSPClient::openURL(*env, argv[0], argv[i],i);
    }

    // All subsequent activity takes place within the event loop:
    //env->taskScheduler().scheduleDelayedTask(40000,delay_task,env);
    env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

    return 0;

    // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
    // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
    // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
    /*
    env->reclaim(); env = NULL;
    delete scheduler; scheduler = NULL;
  */
}

void ourRTSPClient::openURL(UsageEnvironment& env, char const* progName, char const* rtspURL,uint32_t stream_index) {
    // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
    // to receive (even if more than stream uses the same "rtsp://" URL).
    RTSPClient* rtspClient = ourRTSPClient::createNew(env, rtspURL,  stream_index,RTSP_CLIENT_VERBOSITY_LEVEL, progName);
    if (rtspClient == NULL) {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return;
    }


    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
    // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            delete[] resultString;
            break;
        }

        char* const sdpDescription = resultString;
        env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore
        if (scs.session == NULL) {
            env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);

    // An unrecoverable error occurred with this stream.
    ourRTSPClient::shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP True    //True   False

void setupNextSubsession(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) {
        if (!scs.subsession->initiate()) {
            env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } else {
            env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
            if (scs.subsession->rtcpIsMuxed()) {
                env << "client port " << scs.subsession->clientPortNum();
            } else {
                env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
            }
            env << ")\n";

            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }
    //创建rtsp转发流
    if(scs.m_session_id!=0xffffffff)Api_rtsp_server::Api_Rtsp_Server_Remove_Stream(g_handle,scs.m_session_id);
    auto stream_name=get_stream_prefix()+to_string(scs.m_stream_index);
    scs.m_session_id=Api_rtsp_server::Api_Rtsp_Server_Add_Stream(g_handle,stream_name,scs.m_media_info);
    if(scs.m_session_id!=0xffffffff)scs.m_is_setup=true;
    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            break;
        }

        env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
        if (scs.subsession->rtcpIsMuxed()) {
            env << "client port " << scs.subsession->clientPortNum();
        } else {
            env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
        }
        env << ")\n";

        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)
        if(0 == strncmp(scs.subsession->mediumName(), "audio", 5))
        {//audio
            if((scs.subsession->rtpPayloadFormat() == 0)      /*PCMU*/
                    || (scs.subsession->rtpPayloadFormat() == 8 )) /*PCMA*/
            {
                scs.m_media_info.audio_type=Api_rtsp_server::PCMA;
                scs.subsession->sink = G711aDummySink::createNew(env, *scs.subsession,scs, 1,rtspClient->url());
            }
            else if(strcmp(scs.subsession->codecName(),"MPEG4-GENERIC")==0||scs.subsession->rtpPayloadFormat()==104||scs.subsession->rtpPayloadFormat()==97) {
                scs.m_media_info.audio_type=Api_rtsp_server::AAC;
                scs.m_media_info.hasADTS=false;
                scs.m_media_info.sampleRate=scs.subsession->rtpTimestampFrequency();
                scs.m_media_info.channels=scs.subsession->numChannels();
                auto profile_id= scs.subsession->attrVal_int("profile-id");
                if(profile_id == 0)
                    profile_id = 1;
                scs.subsession->sink = AACDummySink::createNew(env, *scs.subsession,scs, 1,scs.m_media_info.sampleRate,scs.m_media_info.channels,profile_id,rtspClient->url());
            }
            else {
                env<<"Unsupported format!\n";
            }
            env<<"audio type:"<<scs.subsession->codecName()<<" rtpPayloadFormat:"<<scs.subsession->rtpPayloadFormat()<<"\n";
        }
        else if(0 == strncmp(scs.subsession->mediumName(), "video", 5))
        {//video
            if(0 == strcmp(scs.subsession->codecName(), "H264"))
            {
                scs.m_media_info.viedo_type=Api_rtsp_server::H264;
                scs.m_media_info.frameRate=25;
            }
            else if(0 == strcmp(scs.subsession->codecName(), "H265"))
            {
                scs.m_media_info.viedo_type=Api_rtsp_server::H265;
                scs.m_media_info.frameRate=25;
            }
            else
            {
                env<<"Unsupported format!"<<scs.subsession->codecName()<<"\n";
            }
            scs.subsession->sink = H264or5DummySink::createNew(env, *scs.subsession,scs, 0,rtspClient->url());
        }

        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL) {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }

        env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
    Boolean success = False;

    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }

        // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
        // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
        // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
        // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0) {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
        }

        env << *rtspClient << "Started playing session";
        if (scs.duration > 0) {
            env << " (for up to " << scs.duration << " seconds)";
        }
        env << "...\n";

        success = True;
    } while (0);
    delete[] resultString;

    if (!success) {
        // An unrecoverable error occurred with this stream.
        ourRTSPClient::shutdownStream(rtspClient);
    }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL) return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    ourRTSPClient::shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData, char const* reason) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    env << *rtspClient << "Received RTCP \"BYE\"";
    if (reason != NULL) {
        env << " (reason:\"" << reason << "\")";
        delete[] (char*)reason;
    }
    env << " on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
    ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
    StreamClientState& scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;

    // Shut down the stream:
    ourRTSPClient::shutdownStream(rtspClient);
}

void ourRTSPClient::shutdownStream(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }

    env << *rtspClient << "Closing the stream.\n";
    Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,uint32_t stream_index,
                                        int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
    return new ourRTSPClient(env, rtspURL, stream_index,verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,uint32_t stream_index,
                             int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1){
    scs.m_stream_index=stream_index;
}

ourRTSPClient::~ourRTSPClient() {
    if(scs.m_is_setup){
        scs.m_is_setup=false;
        Api_rtsp_server::Api_Rtsp_Server_Remove_Stream(g_handle,scs.m_session_id);
    }
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
    : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != NULL) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment& env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}


// Implementation of "H264or5DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 500000

H264or5DummySink* H264or5DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id,char const* streamId) {
    return new H264or5DummySink(env, subsession, scs,channel_id,streamId);
}


H264or5DummySink::H264or5DummySink(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id, char const* streamId)
    : MediaSink(env),
      fSubsession(subsession) ,m_scs(scs),m_channel_id(channel_id){
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
    m_stream_name=get_stream_prefix()+to_string(m_scs.m_stream_index);
#if LIVE_SAVE_STREAM_TO_FILE
    m_save_fp=fopen(m_stream_name.c_str(),"w+");
    if(!m_save_fp)
    {
        throw invalid_argument(string("failed to open ")+m_stream_name);
    }
#endif
}

H264or5DummySink::~H264or5DummySink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;
#if LIVE_SAVE_STREAM_TO_FILE
    if(m_save_fp)fclose(m_save_fp);
#endif
}

void H264or5DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                         struct timeval presentationTime, unsigned durationInMicroseconds) {
    H264or5DummySink* sink = (H264or5DummySink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}



void H264or5DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                                         struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
#if LIVE_SAVE_STREAM_TO_FILE
    static uint8_t start_code[4]={0x00,0x00,0x00,0x01};
    if(m_save_fp&&frameSize>0)
    {
        fwrite(start_code,1,4,m_save_fp);
        fwrite(fReceiveBuffer,1,frameSize,m_save_fp);
    }
#endif
    if(m_scs.m_is_setup&&frameSize>0)
    {

        Api_rtsp_server::Api_Rtsp_Push_Frame(g_handle,m_scs.m_session_id,fReceiveBuffer,frameSize,m_channel_id,presentationTime.tv_sec*1000*1000+presentationTime.tv_usec);
    }
    // We've just received a frame of data.  (Optionally) print out information about it:
    //sprintf("%02x \r\n ",fReceiveBuffer[0]);
#if !DEBUG_PRINT_EACH_RECEIVED_FRAME
    if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
#ifdef DEBUG_PRINT_NPT
    envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
    envir() << "\n";
#endif

    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean H264or5DummySink::continuePlaying() {
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClosure, this);
    return True;
}



#define G711ADUMMY_SINK_RECEIVE_BUFFER_SIZE 2048

G711aDummySink* G711aDummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id,char const* streamId) {
    return new G711aDummySink(env, subsession, scs,channel_id,streamId);
}


G711aDummySink::G711aDummySink(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id, char const* streamId)
    : MediaSink(env),
      fSubsession(subsession) ,m_scs(scs),m_channel_id(channel_id){
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[G711ADUMMY_SINK_RECEIVE_BUFFER_SIZE];
    m_stream_name=get_stream_prefix()+to_string(m_scs.m_stream_index)+".g711a";
#if LIVE_SAVE_STREAM_TO_FILE
    m_save_fp=fopen(m_stream_name.c_str(),"w+");
    if(!m_save_fp)
    {
        throw invalid_argument(string("failed to open ")+m_stream_name);
    }
#endif
}

G711aDummySink::~G711aDummySink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;
#if LIVE_SAVE_STREAM_TO_FILE
    if(m_save_fp)fclose(m_save_fp);
#endif
}

void G711aDummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                       struct timeval presentationTime, unsigned durationInMicroseconds) {
    G711aDummySink* sink = (G711aDummySink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void G711aDummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                                       struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
#if LIVE_SAVE_STREAM_TO_FILE
    uint8_t start_code[4]={0x00,0x01,0x00,0x01};
    if(m_save_fp&&frameSize>0)
    {
        start_code[2]=frameSize>>1;
        fwrite(start_code,1,4,m_save_fp);
        fwrite(fReceiveBuffer,1,frameSize,m_save_fp);
    }
#endif
    if(m_scs.m_is_setup&&frameSize>0)
    {
        Api_rtsp_server::Api_Rtsp_Push_Frame(g_handle,m_scs.m_session_id,fReceiveBuffer,frameSize,m_channel_id,presentationTime.tv_sec*1000*1000+presentationTime.tv_usec);
    }
    // We've just received a frame of data.  (Optionally) print out information about it:
#if DEBUG_PRINT_EACH_RECEIVED_FRAME
    if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
#ifdef DEBUG_PRINT_NPT
    envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
    envir() << "\n";
#endif

    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean G711aDummySink::continuePlaying() {
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, G711ADUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClosure, this);
    return True;
}

#define AACDUMMY_SINK_RECEIVE_BUFFER_SIZE 5*1024
static uint32_t AACSampleRate[16] =
{
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0 /*reserved */
};
AACDummySink* AACDummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id,\
                                      uint32_t sampleRate,uint8_t channel_num,uint8_t profile_id,char const* streamId) {
    return new AACDummySink(env, subsession, scs,channel_id,sampleRate,channel_num,profile_id,streamId);
}


AACDummySink::AACDummySink(UsageEnvironment& env, MediaSubsession& subsession,StreamClientState &scs , uint32_t channel_id, \
                           uint32_t sampleRate,uint8_t channel_num,uint8_t profile_id,char const* streamId)
    : MediaSink(env),
      fSubsession(subsession) ,m_scs(scs),m_channel_id(channel_id),m_sampleRate(sampleRate),m_channel_num(channel_num),m_profile_id(profile_id){
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[AACDUMMY_SINK_RECEIVE_BUFFER_SIZE];
    m_stream_name=get_stream_prefix()+to_string(m_scs.m_stream_index)+".aac";
    m_index=0;
    for(uint8_t i=0;i<16;i++)
    {
        if(AACSampleRate[i]==m_sampleRate)
        {
            m_index=i;
            break;
        }
    }
#if LIVE_SAVE_STREAM_TO_FILE
    m_aac_header[0] = 0xFF;
    m_aac_header[1] = 0XF9;//F1
    m_aac_header[2] = (((m_profile_id - 1) << 6) + (m_index << 2) + (m_channel_num >> 2));
    m_aac_header[3] = 0X00;
    m_aac_header[4] = 0X00;
    m_aac_header[5] = 0X00;
    m_aac_header[6] = 0XFC;
    m_save_fp=fopen(m_stream_name.c_str(),"w+");
    if(!m_save_fp)
    {
        throw invalid_argument(string("failed to open ")+m_stream_name);
    }
#endif
}

AACDummySink::~AACDummySink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;
#if LIVE_SAVE_STREAM_TO_FILE
    if(m_save_fp)fclose(m_save_fp);
#endif
}

void AACDummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                     struct timeval presentationTime, unsigned durationInMicroseconds) {
    AACDummySink* sink = (AACDummySink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}


void AACDummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                                     struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
#if LIVE_SAVE_STREAM_TO_FILE
    if(m_save_fp&&frameSize>0)
    {
        uint32_t packet_len=frameSize+7;
         m_aac_header[3] = (((m_channel_num & 3) << 6) + (packet_len >> 11));
          m_aac_header[4] = ((packet_len & 0x7FF) >> 3);
          m_aac_header[5] = (((packet_len & 7) << 5) + 0x1F);
        fwrite(m_aac_header,1,7,m_save_fp);
        fwrite(fReceiveBuffer,1,frameSize,m_save_fp);
    }
#endif
    if(m_scs.m_is_setup&&frameSize>0)
    {

        Api_rtsp_server::Api_Rtsp_Push_Frame(g_handle,m_scs.m_session_id,fReceiveBuffer,frameSize,m_channel_id,presentationTime.tv_sec*1000*1000+presentationTime.tv_usec);
    }
    // We've just received a frame of data.  (Optionally) print out information about it:
    //printf("%02x \r\n ",fReceiveBuffer[0]);
#if DEBUG_PRINT_EACH_RECEIVED_FRAME
    if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
#ifdef DEBUG_PRINT_NPT
    envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
    envir() << "\n";
#endif

    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean AACDummySink::continuePlaying() {
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, AACDUMMY_SINK_RECEIVE_BUFFER_SIZE,
                          afterGettingFrame, this,
                          onSourceClosure, this);
    return True;
}
