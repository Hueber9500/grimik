#include <iostream>
#include "cpp-httplib/httplib.h"
#include <sqlite3.h>
#include <stdio.h>
#include "UserRepository.h"
#include "json.hpp"
#include <iomanip>
#include <ctime>

using namespace httplib;
using json = nlohmann::json;

#define APPLICATION_JSON "application/json"

std::function<void(const Request& req, std::string& name, std::string& password)> extractUserNameAndPassword;
void GetUserNameAndPasswordFromJSON(const Request& req, std::string& name, std::string& password)
{
    json data = json::parse(req.body);
    name = data["name"];
    password = data["password"];
}

void GetPushTokenAndOSFromJSON(const Request& req, std::string& strPushToken, int& nOS)
{
    json data = json::parse(req.body);
    strPushToken = data["push_token"];
    nOS = data["os"].get<int32_t>();
}

int main()
{
    Server svr;

    svr.Get("/", [](const Request& req, Response& res){
        res.set_content("OK", "text/html");
        return res.status = 200;
    });

    svr.Post("/login", [](const Request& req, Response& res)
    {
        User oUser;
        std::string name, password, pushToken;
        int nOS;

        GetPushTokenAndOSFromJSON(req, pushToken, nOS);
        GetUserNameAndPasswordFromJSON(req, name, password);
        User::GetUser(name.c_str(), password.c_str(), oUser);

        if(oUser.IsNullable())
        {
            return res.status = 404; 
        }

        if(oUser.m_strPushToken != pushToken || oUser.m_nOS != nOS)
        {
            User::RemovePushTokenFromUsers(pushToken.c_str(), nOS);
            oUser.SavePushToken(pushToken.c_str(), nOS);
        }

        json jsonResult;
        jsonResult["id"] = oUser.m_Id;

        res.body = jsonResult.dump();
        res.set_header("Content-Type", APPLICATION_JSON);
        return res.status = 200;
    });

    svr.Post("/register", [](const Request& req, Response& res)
    {
        User oUser;
        std::string name, password;

        GetUserNameAndPasswordFromJSON(req, name, password);
        User::GetUser(name.c_str(), password.c_str(), oUser);

        if(!oUser.IsNullable())
        {
            return res.status = 409;
        }

        User::CreateUser(name.c_str(), password.c_str());
        return res.status = 201;
    });

    svr.Post("/usr_preferences", [](const Request& req, Response& res)
    {
        json jObject = json::parse(req.body);
        int usrId = jObject["id"].get<int>();

        User user;
        user.m_Id = usrId;

        user.LoadPreferences();
        json jResult;
        for(auto& pref : user.m_vPreferences)
        {
            jResult.push_back(pref.ToJson());
        }

        res.body = jResult.dump();
        res.set_header("Content-Type", APPLICATION_JSON);
        return res.status = 200;
    });

    svr.Post("/usr_data", [](const Request& req, Response& res)
    {
        json jObject = json::parse(req.body);

        int usrId = jObject["id"].get<int>();
        std::string bleAddress = jObject["ble"];

        Device oDevice;
        oDevice.m_nUserId = usrId;
        oDevice.m_strBleAddress = bleAddress;
        Device::CreateDevice(oDevice);
        User oUser;
        oUser.m_Id = usrId;

        for(auto& element : jObject["preferences"].items())
        {
            Preference pref;
            pref.m_nChoice = element.value()["choice"].get<int>();
            pref.m_nId = element.value()["id"].get<int>();

            oUser.m_vPreferences.push_back(pref);
        }

        oUser.SavePreferences();
        return res.status = 200;
    });

    svr.set_logger([](const Request& req, const Response& res)
    {
        auto t = std::time(nullptr);
        auto tm = std::localtime(&t);
        
        std::cout<<std::endl<<"REQUEST: "<<std::put_time(tm, "%d.%m.%Y %H:%M:%S") <<std::endl;
        std::cout<<req.path<<std::endl;

        for(auto& header : req.headers)
        {
            std::cout<<header.first<<":"<<header.second<<std::endl;
        }
        std::cout<<std::endl<<req.body<<std::endl;

        std::cout<<"RESPONSE:"<<std::endl;
        std::cout<<res.status<<std::endl;
        std::cout<<res.body<<std::endl;
    });

    svr.listen("192.168.0.247", 12345);

    std::cout<<"Mario"<<std::endl;

    system("read -p 'Press Enter to continue...' var");

    return 0;
}