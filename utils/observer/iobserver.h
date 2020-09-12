#ifndef IOBSERVER_H
#define IOBSERVER_H
#include<map>
#include<string>
#include<functional>
#include<mutex>
namespace micagent {
using namespace std;
template <typename DATA>
class iobserver{
public:
iobserver(){}
virtual ~iobserver(){}
/**
 * @brief subscribe add a subscribe event
 * @param subscriber_name specify a subscriber
 * @param event_name the event that you care for
 * @param data_handle the function who will be called when the event is triggered
 * @return when the subscriber_name is existing,it will return false
 */
virtual bool subscribe(const string&subscriber_name,const string &event_name,const function<void(const DATA&)>&data_handle){
    lock_guard<mutex>locker(m_iobserver_mutex);
    auto iter=m_iobserver_session_map.find(event_name);
    if(iter==end(m_iobserver_session_map)){
        auto ret=m_iobserver_session_map.emplace(event_name,map<string,function<void(const DATA &)>>());
        if(!ret.second)throw runtime_error("iobserver!");
        iter=ret.first;
    }
    auto iter2=iter->second.find(subscriber_name);
    if(iter2!=end(iter->second))return false;
    auto ret=iter->second.emplace(subscriber_name,data_handle);
    return ret.second;
}
/**
 * @brief unsubscribe cancel a subscription
 * @param subscriber_name the subscriber's name
 * @param event_name the name of subscription
 */
virtual void unsubscribe(const string&subscriber_name,const string &event_name){
    lock_guard<mutex>locker(m_iobserver_mutex);
    auto iter=m_iobserver_session_map.find(event_name);
    if(iter!=end(m_iobserver_session_map))
    {
        auto iter2=iter->second.find(subscriber_name);
        if(iter2!=end(iter->second))
        {
            iter->second.erase(iter2);
        }
    }
}
/**
 * @brief notify active a subscription event
 * @param event_name the name of the subscription
 * @param data what will be send to the recipient
 */
virtual void notify(const string &event_name,const DATA&data)
{
    lock_guard<mutex>locker(m_iobserver_mutex);
    auto iter=m_iobserver_session_map.find(event_name);
    if(iter==end(m_iobserver_session_map))return;
    for(auto i :iter->second)
    {
        i.second(data);
    }
}
protected:
mutex m_iobserver_mutex;
map<string,map<string,function<void(const DATA &)>>>m_iobserver_session_map;
};
}

#endif // IOBSERVER_H
