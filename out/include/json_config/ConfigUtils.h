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
    /**
     * @brief Get_Instance single instance
     * @return
     */
    static Config_Manager &Get_Instance(){static Config_Manager instance;return instance;}
    /**
     * @brief Load_Path load config from path
     * @param path
     */
    void Load_Path(const string &path);
    /**
     * @brief Update_Mode_Config update config
     * @param mode_name the mode that needs update
     * @param config_object json_config
     * @return
     */
    bool Update_Mode_Config(const string &mode_name,const CJsonObject &config_object);
    /**
     * @brief Delete_Mode_Config delete config  according to mode_name
     * @param mode_name
     */
    void Delete_Mode_Config(const string &mode_name);
    /**
     * @brief Get_Mode_Config read config by the mode_name
     * @param mode_name
     * @param result
     * @return return false when the mode_name's item is not existed
     */
    bool Get_Mode_Config(const string &mode_name,CJsonObject &result)const;
    /**
     * @brief Save_Config save to file
     */
    void Save_Config();
    /**
     * @brief Dump_All_Config return all config with a string
     * @param use_fmt if usefmt,the return string will add  proper '\t' '\r\n' or '\n'
     * @return
     */
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
