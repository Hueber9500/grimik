#include "DB.h"
#include <cassert>
#include <stdio.h>
#include <sqlite3.h>


DB::DB()
{
    int rc = sqlite3_open("test.db", &_db);
    if(rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(DB::_db));
        sqlite3_close(_db);
        assert(0);
    }

    char* szErrorMsg;
    rc = sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, password TEXT);", nullptr, nullptr, &szErrorMsg);
    if(rc)
    {
        fprintf(stderr, "Error %s when executing statement", szErrorMsg);
        sqlite3_free(szErrorMsg);
        sqlite3_close(_db);
        assert(0);
    }

}

sqlite3* DB::GetSql() const{
    return _db;
}

DB::~DB(){
    if(_db!=nullptr){
        sqlite3_close(_db);
    }
}