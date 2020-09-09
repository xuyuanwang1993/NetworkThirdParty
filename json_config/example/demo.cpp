#include <string>
#include <iostream>
#include "CJsonObject.hpp"
#include "ConfigUtils.h"
//int main()
//{
//    //int iValue;
//    //double fTimeout;
//    //std::string strValue;
//    //neb::CJsonObject oJson("{\"refresh_interval\":60,"
//    //                    "\"test_float\":[18.0, 10.0, 5.0],"
//    //                    "\"timeout\":12.5,"
//    //                    "\"dynamic_loading\":["
//    //                        "{"
//    //                            "\"so_path\":\"plugins/User.so\", \"load\":false, \"version\":1,"
//    //                            "\"cmd\":["
//    //                                 "{\"cmd\":2001, \"class\":\"neb::CmdUserLogin\"},"
//    //                                 "{\"cmd\":2003, \"class\":\"neb::CmdUserLogout\"}"
//    //                            "],"
//    //                            "\"module\":["
//    //                                 "{\"path\":\"im/user/login\", \"class\":\"neb::ModuleLogin\"},"
//    //                                 "{\"path\":\"im/user/logout\", \"class\":\"neb::ModuleLogout\"}"
//    //                            "]"
//    //                         "},"
//    //                         "{"
//    //                         "\"so_path\":\"plugins/ChatMsg.so\", \"load\":false, \"version\":1,"
//    //                             "\"cmd\":["
//    //                                  "{\"cmd\":2001, \"class\":\"neb::CmdChat\"}"
//    //                             "],"
//    //                         "\"module\":[]"
//    //                         "}"
//    //                    "]"
//    //                "}");
//    // std::cout << oJson.ToString() << std::endl;
//    // std::cout << "-------------------------------------------------------------------" << std::endl;
//    // std::cout << oJson["dynamic_loading"][0]["cmd"][1]("class") << std::endl;
//    // oJson["dynamic_loading"][0]["cmd"][0].Get("cmd", iValue);
//    // std::cout << "iValue = " << iValue << std::endl;
//    // oJson["dynamic_loading"][0]["cmd"][0].Replace("cmd", -2001);
//    // oJson["dynamic_loading"][0]["cmd"][0].Get("cmd", iValue);
//    // std::cout << "iValue = " << iValue << std::endl;
//    // oJson.Get("timeout", fTimeout);
//    // std::cout << "fTimeout = " << fTimeout << std::endl;
//    // oJson["dynamic_loading"][0]["module"][0].Get("path", strValue);
//    // std::cout << "strValue = " << strValue << std::endl;
//    // std::cout << "-------------------------------------------------------------------" << std::endl;
//    // oJson.AddEmptySubObject("depend");
//    // oJson["depend"].Add("nebula", "https://github.com/Bwar/Nebula");
//    // oJson["depend"].AddEmptySubArray("bootstrap");
//    // oJson["depend"]["bootstrap"].Add("BEACON");
//    // oJson["depend"]["bootstrap"].Add("LOGIC");
//    // oJson["depend"]["bootstrap"].Add("LOGGER");
//    // oJson["depend"]["bootstrap"].Add("INTERFACE");
//    // oJson["depend"]["bootstrap"].Add("ACCESS");
//    // std::cout << oJson.ToString() << std::endl;
//    // std::cout << "-------------------------------------------------------------------" << std::endl;
//    // std::cout << oJson.ToFormattedString() << std::endl;

//    // std::cout << "-------------------------------------------------------------------" << std::endl;
//    // neb::CJsonObject oCopyJson = oJson;
//    // if (oCopyJson == oJson)
//    // {
//    //     std::cout << "json equal" << std::endl;
//    // }
//    // oCopyJson["depend"]["bootstrap"].Delete(1);
//    // oCopyJson["depend"].Replace("nebula", "https://github.com/Bwar/CJsonObject");
//    // std::cout << oCopyJson.ToString() << std::endl;
//    // std::cout << "-------------------------key traverse------------------------------" << std::endl;
//    // std::string strTraversing;
//    // while(oJson["dynamic_loading"][0].GetKey(strTraversing))
//    // {
//    //     std::cout << "traversing:  " << strTraversing << std::endl;
//    // }
//    // std::cout << "---------------add a new key, then key traverse---------------------" << std::endl;
//    // oJson["dynamic_loading"][0].Add("new_key", "new_value");
//    // while(oJson["dynamic_loading"][0].GetKey(strTraversing))
//    // {
//    //     std::cout << "traversing:  " << strTraversing << std::endl;
//    // }

//    // std::cout << oJson["test_float"].GetArraySize() << std::endl;
//    // float fTestValue = 0.0;
//    // for (int i = 0; i < oJson["test_float"].GetArraySize(); ++i)
//    // {
//    //     oJson["test_float"].Get(i, fTestValue);
//    //     std::cout << fTestValue << std::endl;
//    // }
//    // oJson.AddNull("null_value");
//    // std::cout << oJson.IsNull("test_float") << "\t" << oJson.IsNull("null_value") << std::endl;
//    // oJson["test_float"].AddNull();
//    // std::cout << oJson.ToString() << std::endl;
//     //using neb::CJsonObject;
//	 //system("chcp 65001");
//     //auto object = CJsonObject::CreateInstance("test2.txt");
//	 //object->Add("test", 1);
//	 //object->Add("test2", 1.12346);
//	 //object->Add("test3", "ddadsa");
//	 //object->Add("test4", true);
//	 //object->AddNull("qwer");
//	 //object->AddEmptySubObject("object2");
//	 //(*object)["object2"].Add("1234", 1);
//	 //object->AddEmptySubArray("array");
//	 //(*object)["array"].Add(1);
//	 //(*object)["array"].Add(2);
//	 //(*object)["array"].AddAsFirst(1);
////	 std::cout << "------------------------------------------------------------" << std::endl;
////	 std::cout << object->ToString() << std::endl;
////	 std::cout << "------------------------------------------------------------" << std::endl;
////	 std::cout << object->ToFormattedString() << std::endl;
////	 std::cout << "------------------------------------------------------------" << std::endl;
////	 object->SaveToFile();
//neb::CJsonObject object;
//object.Add("1",1);
//object.Add("2",1);
//object.Add("3",1);
//object.Add("4",1);
//object.Add("5",1);
//auto &object_test=object["test"];
//object_test.Add("test","test");
//object.Add("test",object_test);
//std::string key;
//std::cout<<object.GetKey(key);
//std::cout<<object.GetKey(key);
//std::cout<<object.GetKey(key);
//std::cout<<object.GetKey(key);
//std::cout<<object.GetKey(key);
//std::cout<<object.GetKey(key);
//std::cout<<object.ToFormattedString()<<std::endl;
//std::cout<<object_test.ToFormattedString()<<std::endl;
//	 return 0;
//}
int main(){
    auto &instance=micagent::Config_Manager::Get_Instance();
    instance.Load_Path("test.json");
    std::cout<<instance.Dump_All_Config()<<"\r\n";
    std::cout<<"-----------------------------------------------"<<"\r\n";
    {
        neb::CJsonObject object1;
        object1.Add("port",8554);
        instance.Update_Mode_Config("rtsp",object1);
        std::cout<<instance.Dump_All_Config()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    {
        neb::CJsonObject object1;
        object1.Add("port",8554);
        std::cout<<instance.Update_Mode_Config("rtsp",object1)<<"\r\n";
        std::cout<<instance.Dump_All_Config()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    {
        neb::CJsonObject object1;
        object1.Add("port",8554);
        instance.Update_Mode_Config("rtmp",object1);
        std::cout<<instance.Dump_All_Config()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    {
        instance.Delete_Mode_Config("rtmp");
        std::cout<<instance.Dump_All_Config()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    {
        neb::CJsonObject object1;
        object1.Add("port",554);
        instance.Update_Mode_Config("rtsp",object1);
        std::cout<<instance.Dump_All_Config()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    {
        neb::CJsonObject object1;
        object1.Add("port",8554);
        instance.Update_Mode_Config("rtmp",object1);
        std::cout<<instance.Dump_All_Config()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    {
        neb::CJsonObject object1;
        std::cout<<instance.Get_Mode_Config("rtsp",object1)<<"\r\n";
        std::cout<<object1.ToFormattedString()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    {
        neb::CJsonObject object1;
        std::cout<<instance.Get_Mode_Config("rtmp",object1)<<"\r\n";
        std::cout<<object1.ToFormattedString()<<"\r\n";
        std::cout<<"-----------------------------------------------"<<"\r\n";
    }
    instance.Save_Config();
    return 0;
}
