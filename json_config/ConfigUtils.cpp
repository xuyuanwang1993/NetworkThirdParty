#include "ConfigUtils.h"
#define CONFIG_AUTO_SAVE 0
using namespace micagent;
void Config_Manager::Load_Path(const string &path)
{
    lock_guard<mutex>locker(m_mutex);
    m_json_object.reset(CJsonObject::CreateInstance(path));
}
bool Config_Manager::Update_Mode_Config(const string &mode_name,const CJsonObject &config_object)
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_json_object)return false;
    CJsonObject tmp;
    bool ret=false;
    if(m_json_object.get()->Get(mode_name,tmp)){
        ret=m_json_object->Replace(mode_name,config_object);
    }
    else ret=m_json_object->Add(mode_name,config_object);
    m_json_object->SaveToFile();
    return ret;
}
void Config_Manager::Delete_Mode_Config(const string &mode_name)
{
    lock_guard<mutex>locker(m_mutex);
    do{
        if(!m_json_object)break ;
        CJsonObject tmp;
        if(!m_json_object.get()->Get(mode_name,tmp))break ;
        m_json_object->Delete(mode_name);
    }while(0);

}
bool Config_Manager::Get_Mode_Config(const string &mode_name,CJsonObject &result)const
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_json_object)return false;
    return  m_json_object.get()->Get(mode_name,result);
}
void Config_Manager::Save_Config()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_json_object)m_json_object->SaveToFile();
}
string Config_Manager::Dump_All_Config(bool use_fmt)const
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_json_object)return  string();
    if(use_fmt)return m_json_object->ToFormattedString();
    else {
       return  m_json_object->ToString();
    }
}
Config_Manager::~Config_Manager()
{
#if CONFIG_AUTO_SAVE
    if(m_json_object)m_json_object->SaveToFile();
#endif
}
