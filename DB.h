#pragma once

#include <sqlite3.h>

class DB{
    private:
        sqlite3* _db;

    public:
        DB();
        DB(const DB&) = delete;
        DB& operator=(const DB&) = delete;
        ~DB();

    public:
        sqlite3* GetSql() const;
};