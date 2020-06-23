#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H
#include <vector>
#include <random>
#include <map>
#include <cmath>
#include <deque>
#ifdef DEBUG
#include <iostream>
#endif
namespace micagent {
using namespace std;
template <typename T>
struct shuffle_cards{
    shuffle_cards(vector<T>&input){
        random_device rd;
        auto times=(input.size()+rd()%2)/2;
        for(auto i =0;i<times;i++){
            auto begin_pos=input.size()-1;
            while(begin_pos>0){
                decltype (begin_pos)move_pos=rd()%begin_pos;
                auto tmp=input[begin_pos];
                input[begin_pos]=input[move_pos];
                input[move_pos]=tmp;
                begin_pos--;
            }
        }
    }
};
template <typename A>
class priority_queue{
    static constexpr uint8_t  MIN_PRIORITY_LIMIT=16;
public:
    priority_queue(uint8_t min_priority=16,uint32_t max_items=UINT32_MAX,double pow_base=2)\
        :m_can_jump(false),m_capacity(max_items),m_pos(0),m_queue_size(0),m_min_priority(min_priority)
    {
        if(m_min_priority>MIN_PRIORITY_LIMIT)m_min_priority=MIN_PRIORITY_LIMIT;
        m_data_queue.clear();
        m_data_queue.resize(m_min_priority+1);
        for(decltype (m_min_priority)i=0;i<=m_min_priority;i++){
            int items=ceil(pow(pow_base,m_min_priority-i)+1);
            for(auto j=0;j<items;j++)m_scheduler.emplace_back(i);
        }
        shuffle_cards<uint8_t> tmp(m_scheduler);
#ifdef DEBUG
        for(auto i: m_scheduler){
            cout<<(int)i<<" ";
        }
        cout<<endl;
#endif
    }
    bool push(uint8_t priority,const A &data,bool jump=false){
        if(m_queue_size>=m_capacity||priority>m_min_priority)return false;
        m_queue_size++;
#ifdef DEBUG
        //cout<<"insert priority:"<<(int)priority<<" data:"<<data<<endl;
#endif
        if(m_can_jump&&jump){
            m_data_queue[priority].push_front(data);
        }
        else {
            m_data_queue[priority].push_back(data);
        }
        return true;
    }
    bool pop(A &save){
        if(m_queue_size==0)return false;
        auto search_size=m_scheduler.size();
        while(search_size-->0){
            auto &tmp=m_data_queue[m_scheduler[m_pos]];
            if(!tmp.empty()){
#ifdef DEBUG
                cout<<"get priority:"<<(int)m_scheduler[m_pos]<<" data:"<<tmp.front()<<endl;
#endif
                //find
                save=tmp.front();
                tmp.pop_front();
                m_queue_size--;
                move_pos();
                return true;
            }
            move_pos();
        }
        return false;
    }
    void enable_jump(){m_can_jump=true;}
    void disable_jump(){m_can_jump=false;}
private:
    void move_pos(){
        m_pos++;
        if(m_pos>=m_scheduler.size())m_pos=0;
    }
    vector<deque<A>>m_data_queue;
    vector<uint8_t> m_scheduler;
    bool m_can_jump;
    uint32_t m_capacity;
    uint32_t m_pos;
    uint32_t m_queue_size;
    uint8_t m_min_priority;
};
}
#endif // PRIORITY_QUEUE_H
