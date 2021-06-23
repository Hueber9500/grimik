#include "UserRepository.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>
#include "DB.h"
#include <memory>
#include <string>


template<typename ... Args>
std::string string_format(const std::string& format, Args... args)
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

template<typename Arg>
void format_impl(std::stringstream& ss, const char* format, Arg arg)
{
    while (*format) {
        if (*format == '%' && *++format != '%') {
            auto current_format_qualifier = *format;
            switch(current_format_qualifier) {
                case 'd' : if (!std::is_integral<Arg>()) throw std::invalid_argument("%d introduces integral argument");
                // etc.
            }
            // it's true you'd have to handle many more format qualifiers, but on a safer basis
            ss << arg; // arg type is deduced
        }
        ss << *format++;
        } // the format string is exhausted and we still have args : throw
    throw std::invalid_argument("Too many arguments\n");
}


template<typename Arg, typename... Args>
void format_impl(std::stringstream& ss, const char* format, Arg arg, Args... args)
{
    while (*format) 
    {
        if (*format == '%' && *++format != '%') 
        {
            auto current_format_qualifier = *format;
            switch(current_format_qualifier) 
            {
                case 'd' : if (!std::is_integral<Arg>()) throw std::invalid_argument("%d introduces integral argument");
                // etc.
            }
            // it's true you'd have to handle many more format qualifiers, but on a safer basis
            ss << arg; // arg type is deduced
            return format_impl(ss, ++format, args...); // one arg less
        }
        ss << *format++;
    }
     // the format string is exhausted and we still have args : throw
    throw std::invalid_argument("Too many arguments\n");
}

template<typename... Args>
std::string format(const char* pszFormat, Args... args)
{
    std::stringstream ss;
    format_impl(ss, pszFormat, args...);
    return ss.str();
}

int User::GetUser(const char* name,const char* password, User& oUser)
{
    DB db; 
    auto func = [](void* user, int columnCount, char** data, char** columns) -> int
    {
        User* pUser = (User*)user;
        pUser->m_Id = atoi(data[0]);
        pUser->m_strName = data[1];
        pUser->m_strPushToken = data[2];
        pUser->m_nOS = atoi(data[3]);

        return 0;
    };

    std::string query = string_format("SELECT id, name, IFNULL(push_token, ''), IFNULL(os, 0) FROM users WHERE name = '%s' AND password = '%s' LIMIT 1", name, password);
    int rc = sqlite3_exec(db.GetSql(), query.c_str(), func, &oUser, nullptr);
    
    return rc;
}

void User::SavePushToken(const char* szPushToken, int nOS)
{
    DB db;
    std::string cmd = string_format("UPDATE users SET push_token = '%s', os = '%d' WHERE id = %d", szPushToken, nOS, m_Id);
    int rc = sqlite3_exec(db.GetSql(), cmd.c_str(), nullptr, nullptr, nullptr);
    m_strPushToken = szPushToken;
    m_nOS = nOS;
}

void User::RemovePushTokenFromUsers(const char* szPushToken, int nOS)
{
    DB db;
    std::string cmd = string_format("UPDATE users SET push_token = '', os = 0 WHERE push_token = '%s' AND os = %d", szPushToken, nOS);
    int rc = sqlite3_exec(db.GetSql(), cmd.c_str(), nullptr, nullptr, nullptr);
}

int User::CreateUser(const char* name, const char* password)
{
    
    DB db;
    std::string query = string_format("INSERT INTO users(name, password) VALUES('%s', '%s')", name, password);
    int rc = sqlite3_exec(db.GetSql(), query.c_str(), nullptr, nullptr, nullptr);

    return rc;
}

bool User::IsNullable()
{
    return m_strName.empty() && m_Id == 0;
}

void User::SavePreferences()
{
    DB db;
    if(m_vPreferences.empty()) 
        return;

    for(auto el : m_vPreferences)
    {
        auto query = string_format(
            "DELETE FROM user_preferences WHERE user_id = %d and preference_id = %d;INSERT INTO user_preferences(user_id, preference_id, choice) VALUES(%d, %d, %d);", m_Id, el.m_nId, m_Id, el.m_nId, el.m_nChoice);
        sqlite3_exec(db.GetSql(), query.c_str(), nullptr, nullptr, nullptr);
    }
}

void User::LoadPreferences()
{
    DB db;
    auto func = [](void* usrObject, int columnCount, char** data, char** columns ) -> int
    {
        User* pUsr = (User*)usrObject;
        Preference preference;
        preference.m_strName  = data[0];
        preference.m_nId = atoi(data[1]);
        if(data[2])
        {
            preference.m_nChoice = atoi(data[2]);
            preference.m_bIsSet = true;
        }

        pUsr->m_vPreferences.push_back(std::move(preference));

        return 0;
    };
    auto query = string_format("SELECT p.name, p.id, up.choice FROM preferences p LEFT JOIN user_preferences up ON p.id = up.preference_id AND up.user_id = %d", m_Id);
    int rc = sqlite3_exec(db.GetSql(), query.c_str(), func, this, nullptr);
}

int Device::DeactivateAllDevicesForUserIdExceptCurrent(const Device& oDevice)
{
    auto query = string_format("UPDATE devices SET is_active = 0 WHERE (user_id = %d and ble_address <> '%s') OR (user_id <> %d AND ble_address = '%s')", oDevice.m_nUserId, oDevice.m_strBleAddress.c_str(), oDevice.m_nUserId, oDevice.m_strBleAddress.c_str());
    return sqlite3_exec(DB().GetSql(), query.c_str(), nullptr, nullptr, nullptr);
}

Device&& Device::LoadActiveDeviceByUserId(int nUserId)
{
    DB db;
    Device oDevice;
    oDevice.m_nUserId = nUserId;

    auto func = [](void* device, int columnCount, char** data, char** columns ) -> int
    {
        Device* pDevice = (Device*)device;
        pDevice->m_strBleAddress = data[0];
        pDevice->m_bIsActive = atoi(data[1]) == 1;
        return 0;
    };

    auto query = string_format("SELECT ble_address, is_active FROM devices WHERE user_id = %d and is_active = 1 ORDER BY id DESC LIMIT 1", oDevice.m_nUserId);
    int rc = sqlite3_exec(db.GetSql(), query.c_str(), func, &oDevice, nullptr );

    return std::move(oDevice);
}

bool Device::CheckIfDeviceExists(const Device& oDevice)
{
    DB db;

    bool bHasRows = false;
    auto query = string_format("SELECT 1 FROM devices WHERE user_id = %d and ble_address = '%s'", oDevice.m_nUserId, oDevice.m_strBleAddress.c_str());
    auto func = [](void* funcData, int columnCount, char** data, char** columns ) -> int
    {
        bool* pbHasRows = (bool*)funcData;
        *pbHasRows = true;
        return 0;
    };

    sqlite3_exec(db.GetSql(), query.c_str(), func, &bHasRows, nullptr);

    return bHasRows;
}

int Device::CreateDevice( Device& oDevice)
{
    DB db;
    std::string query;
    int rc;
    if(CheckIfDeviceExists(oDevice))
    {
        query = string_format("UPDATE devices SET is_active = 1 WHERE user_id = %d and ble_address = '%s'", oDevice.m_nUserId, oDevice.m_strBleAddress.c_str());
        rc = sqlite3_exec(db.GetSql(), query.c_str(), nullptr, nullptr, nullptr);
    
    }
    else
    {
        query = string_format("INSERT INTO devices(user_id, ble_address, is_active) VALUES(%d, '%s', 1)", oDevice.m_nUserId, oDevice.m_strBleAddress.c_str());
        rc = sqlite3_exec(db.GetSql(), query.c_str(), nullptr, nullptr, nullptr);
    }

    DeactivateAllDevicesForUserIdExceptCurrent(oDevice);

    oDevice.m_bIsActive = true;
    return rc;
}

json Preference::ToJson()
{
    return json{
        {"name", m_strName},
        {"id", m_nId},
        {"choice", m_nChoice},
        {"is_set", m_bIsSet}
    };
}