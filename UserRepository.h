#pragma once

#include <string>
#include <vector>
#include "json.hpp"

using nlohmann::json;


class Preference{
    public:
        std::string m_strName;
        int m_nId = 0;
        int m_nChoice = 0;
        bool m_bIsSet = false;

    public:
        json ToJson();
};


class User{

    public:
        std::string m_strName;
        int m_Id = 0;
        std::vector<Preference> m_vPreferences;
        std::string m_strPushToken;
        int m_nOS;

    public:
        bool IsNullable();
        void SavePreferences();
        void LoadPreferences();
        void SavePushToken(const char* strPushToken, int nOS);

    public:
        static int GetUser(const char* name, const  char* password, User& oUser);
        static int CreateUser(const char* name, const char* password);
        static void RemovePushTokenFromUsers(const char* token, int os);
};

class Device{
    public:
        std::string m_strBleAddress;
        bool m_bIsActive = false;
        int m_nUserId = 0;

    public:
        static Device&& LoadActiveDeviceByUserId(int nUserId);
        static int CreateDevice(Device& oDevice);
        static int DeactivateAllDevicesForUserIdExceptCurrent(const Device& oDevice);

    private:
        static bool CheckIfDeviceExists(const Device& oDevice);

};

