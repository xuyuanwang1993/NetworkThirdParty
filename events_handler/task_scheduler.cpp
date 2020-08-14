#include "task_scheduler.h"
#include<exception>
namespace micagent {
TaskScheduler::TaskScheduler(int _id):m_last_handle_none(false),m_id(_id),m_wake_up_pipe(make_shared<Pipe>()),m_is_running(false),m_last_time_out(MIN_TIME_OUT)
{
    if(m_wake_up_pipe->open()){
        m_wakeup_channel.reset(new Channel((*m_wake_up_pipe.get())()));
        m_wakeup_channel->setReadCallback([this](Channel *chn){
            (void)*chn;
            uint8_t buf[10]={0};
            while(m_wake_up_pipe->read(buf,10)>0);
            return true;
        });
		m_wakeup_channel->enableReading();
    }
	Network_Util::Instance().set_ignore_sigpipe(m_wakeup_channel->fd());
}
void TaskScheduler::start()
{
    if(!get_running_state()){
        {
            DEBUG_LOCK
            m_is_running.exchange(true);
        }
        while(get_running_state()){
            if(!handleEvent()){
                throw runtime_error("TaskScheduler handleEvent!");
            }
        }
        MICAGENT_LOG(LOG_WARNNING,"TaskScheduler %d exit!",m_id);
    }
}
void TaskScheduler::stop()
{
    {
        DEBUG_LOCK
        m_is_running.exchange(false);
    }
    wake_up();
}
void TaskScheduler::updateChannel(ChannelPtr channel)
{
    (void)channel;
}
void TaskScheduler::updateChannel(Channel * channel)
{
    (void)channel;
}
void TaskScheduler::removeChannel(ChannelPtr &channel)
{
    (void)channel;
}
void TaskScheduler::removeChannel(SOCKET fd)
{
    (void)fd;
}
bool TaskScheduler::handleEvent()
{
    return  false;
}
bool TaskScheduler::handle_channel_events(SOCKET fd,int events)
{
    if(events==0)return true;
    unique_lock<mutex> locker(m_mutex);
    auto channel=m_channel_map.find(fd);
    if(channel!=std::end(m_channel_map)){
        auto channel_ptr=channel->second.lock();
        if(locker.owns_lock())locker.unlock();
        if(channel_ptr)channel_ptr->handleEvent(events);
        else {
            return false;
        }
    }
    return  true;
}
void TaskScheduler::wake_up()
{
    static uint8_t message[1]={1};
    m_wake_up_pipe->write(message,sizeof (message));
}
int64_t TaskScheduler::get_time_out()
{
    if(!m_last_handle_none){
        m_last_time_out=MIN_TIME_OUT;
    }
    else {
        m_last_time_out*=2;
        m_last_time_out=m_last_time_out>MAX_TIME_OUT?MAX_TIME_OUT:m_last_time_out;
    }
    return m_last_time_out;
}
SelectTaskScheduler::SelectTaskScheduler(int _id):TaskScheduler (_id),m_max_sock_num(0)
{
    FD_ZERO(&m_read_sets);
    FD_ZERO(&m_write_sets);
    FD_ZERO(&m_exception_sets);
    this->updateChannel(m_wakeup_channel);
}
SelectTaskScheduler::~SelectTaskScheduler()
{
    /*do nothing*/
}
void SelectTaskScheduler::updateChannel(ChannelPtr channel)
{
    auto fd=channel->fd();
    if(fd!= INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter==std::end(m_channel_map)){
            if(!channel->isNoneEvent()){
                m_channel_map.emplace(fd,channel);
                m_max_sock_num=m_max_sock_num>fd?m_max_sock_num:fd;
                FD_SET(fd,&m_exception_sets);
                if(channel->isReading())FD_SET(fd,&m_read_sets);
                if(channel->isWriting())FD_SET(fd,&m_write_sets);
                wake_up();
            }
        }
        else {
            FD_CLR(fd,&m_read_sets);
            FD_CLR(fd,&m_write_sets);
            FD_CLR(fd,&m_exception_sets);
            if(channel->isNoneEvent()){
                m_channel_map.erase(iter);
                m_max_sock_num=((fd==m_max_sock_num)?(m_max_sock_num-1):m_max_sock_num);
            }
            else {
                FD_SET(fd,&m_exception_sets);
                if(channel->isReading())FD_SET(fd,&m_read_sets);
                if(channel->isWriting())FD_SET(fd,&m_write_sets);
                wake_up();
            }
        }
    }
}
void SelectTaskScheduler::updateChannel(Channel * channel)
{
    auto fd=channel->fd();
    if(fd != INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter!=std::end(m_channel_map)) {
            FD_CLR(fd,&m_read_sets);
            FD_CLR(fd,&m_write_sets);
            FD_CLR(fd,&m_exception_sets);
            if(channel->isNoneEvent()){
                m_channel_map.erase(iter);
                m_max_sock_num=((fd==m_max_sock_num)?(m_max_sock_num-1):m_max_sock_num);
            }
            else {
                FD_SET(fd,&m_exception_sets);
                if(channel->isReading())FD_SET(fd,&m_read_sets);
                if(channel->isWriting())FD_SET(fd,&m_write_sets);
                wake_up();
            }
        }
    }
}
void SelectTaskScheduler::removeChannel(ChannelPtr &channel)
{
    auto fd=channel->fd();
    if(fd != INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter!=end(m_channel_map)){
            FD_CLR(fd,&m_read_sets);
            FD_CLR(fd,&m_write_sets);
            FD_CLR(fd,&m_exception_sets);
            m_channel_map.erase(iter);
            m_max_sock_num=((fd==m_max_sock_num)?(m_max_sock_num-1):m_max_sock_num);
        }
    }
}
void SelectTaskScheduler::removeChannel(SOCKET fd)
{
    if(fd != INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter!=end(m_channel_map)){
            FD_CLR(fd,&m_read_sets);
            FD_CLR(fd,&m_write_sets);
            FD_CLR(fd,&m_exception_sets);
            m_channel_map.erase(iter);
            m_max_sock_num=((fd==m_max_sock_num)?(m_max_sock_num-1):m_max_sock_num);
        }
    }
}
bool SelectTaskScheduler::handleEvent()
{
    fd_set read_sets;
    fd_set write_sets;
    fd_set exception_sets;
    FD_ZERO(&read_sets);
    FD_ZERO(&write_sets);
    FD_ZERO(&exception_sets);
    SOCKET max_fd;
    {
        lock_guard<mutex> locker(m_mutex);
        read_sets=m_read_sets;
        write_sets=m_write_sets;
        exception_sets=m_exception_sets;
        max_fd=m_max_sock_num;
    }
    /*安全检查*/
    if(max_fd==INVALID_SOCKET)max_fd=0;
    auto timeout=get_time_out();
    struct timeval tv = { static_cast<__time_t>(timeout / 1000), static_cast<__suseconds_t>(timeout % 1000 * 1000 )};
    int ret = select(max_fd+1, &read_sets, &write_sets, &exception_sets, &tv);
    if(ret<0){
#if defined(__linux) || defined(__linux__)
        if(errno!=EINTR&&errno!=EAGAIN)return false;
#elif defined(WIN32) || defined(_WIN32)
        int err = WSAGetLastError();
        if (err == WSAEINVAL && read_sets.fd_count == 0) {err=EINTR;}
        if(err!=EINTR)return false;
#endif
    }
    if(ret>0){
        m_last_handle_none=false;
        for(SOCKET fd=1;fd<=max_fd;fd++){
            int events=0;
            if(FD_ISSET(fd,&read_sets))events|=EVENT_IN;
            if(FD_ISSET(fd,&write_sets))events|=EVENT_OUT;
            if(FD_ISSET(fd,&exception_sets))events|=EVENT_HUP;
            if(!handle_channel_events(fd,events))
            {
                removeChannel(fd);
            }
        }
    }
    else {
        m_last_handle_none=true;
    }
    return true;
}
EpollTaskScheduler::EpollTaskScheduler(int _id):TaskScheduler (_id)
{
#if defined(__linux) || defined(__linux__)
    m_epoll_fd = epoll_create1(0);
#endif
    this->updateChannel(m_wakeup_channel);
}
EpollTaskScheduler::~EpollTaskScheduler()
{
    /*just do nothing*/
}
void EpollTaskScheduler::updateChannel(ChannelPtr channel)
{
#if defined(__linux) || defined(__linux__)
    auto fd=channel->fd();
    if(fd!= INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter==std::end(m_channel_map)){
            if(!channel->isNoneEvent()){
                m_channel_map.emplace(fd,channel);
                update(EPOLL_CTL_ADD, channel);
                wake_up();
            }
        }
        else {
            if(channel->isNoneEvent()){
                update(EPOLL_CTL_DEL, channel);
                m_channel_map.erase(iter);
            }
            else {
                update(EPOLL_CTL_MOD, channel);
                wake_up();
            }
        }
    }
#endif
}
void EpollTaskScheduler::updateChannel(Channel *channel)
{
#if defined(__linux) || defined(__linux__)
    auto fd=channel->fd();
    if(fd!= INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter!=std::end(m_channel_map)) {
            auto ptr=iter->second.lock();
            if(!ptr){
                m_channel_map.erase(iter);
                return;
            }
            if(channel->isNoneEvent()){
                update(EPOLL_CTL_DEL, ptr);
                m_channel_map.erase(iter);
            }
            else {
                update(EPOLL_CTL_MOD, ptr);
                wake_up();
            }
        }
    }
#endif
}
void EpollTaskScheduler::removeChannel(ChannelPtr &channel)
{
#if defined(__linux) || defined(__linux__)
    auto fd=channel->fd();
    if(fd!= INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter!=end(m_channel_map)){
            update(EPOLL_CTL_DEL, channel);
            m_channel_map.erase(iter);
        }
    }
#endif
}
void EpollTaskScheduler::removeChannel(SOCKET fd)
{
#if defined(__linux) || defined(__linux__)
    if(fd!= INVALID_SOCKET){
        lock_guard<mutex> locker(m_mutex);
        auto iter=m_channel_map.find(fd);
        if(iter!=end(m_channel_map)){
            auto ptr=iter->second.lock();
            if(!ptr){
                m_channel_map.erase(iter);
                return;
            }
            update(EPOLL_CTL_DEL, ptr);
            m_channel_map.erase(iter);
        }
    }
#endif
}
bool EpollTaskScheduler::handleEvent()
{
#if defined(__linux) || defined(__linux__)
    struct epoll_event events[512];
    bzero(&events,sizeof (events));
    int numEvents = -1;
    auto timeout=get_time_out();
    numEvents = epoll_wait(m_epoll_fd, events, 512, static_cast<int>(timeout));
    if(numEvents < 0)  //
    {
        if(errno != EINTR)
        {
            return false;
        }
    }
    if(numEvents>0)m_last_handle_none=false;
    else m_last_handle_none=true;
    for(int n=0; n<numEvents; n++)
    {
        if(!this->handle_channel_events(events[n].data.fd,static_cast<int>(events[n].events)))
        {
            removeChannel(events[n].data.fd);
        }
    }
    return true;
#else
    return false;
#endif
}
void EpollTaskScheduler::update(int operation, ChannelPtr& channel)
{
#if defined(__linux) || defined(__linux__)
    struct epoll_event event ;
    bzero(&event,sizeof (event));
    if(operation != EPOLL_CTL_DEL)
    {
        event.data.fd = channel->fd();
        event.events = static_cast<uint32_t>(channel->events());
    }

    if(::epoll_ctl(m_epoll_fd, operation, channel->fd(), &event) < 0)
    {

    }
#endif
}
}
