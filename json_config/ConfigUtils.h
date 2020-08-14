#ifndef CONFIGUTILS_H
#define CONFIGUTILS_H
#include "CJsonObject.hpp"
#include <mutex>
#include<memory>
namespace micagent {
using neb::CJsonObject;
using namespace std;
class Config_Manager
{
public:
    static Config_Manager &Get_Instance(){static Config_Manager instance;return instance;}
    void Load_Path(const string &path);
    bool Update_Mode_Config(const string &mode_name,const CJsonObject &config_object);
    void Delete_Mode_Config(const string &mode_name);
    bool Get_Mode_Config(const string &mode_name,CJsonObject &result)const;
    void Save_Config();
    string Dump_All_Config(bool use_fmt=true)const;
protected:
    Config_Manager() {}
    virtual ~Config_Manager();
protected:
    mutable mutex m_mutex;
    shared_ptr<CJsonObject> m_json_object;
};
}
#endif // CONFIGUTILS_H
