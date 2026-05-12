#pragma once

#include <mysql/mysql.h>
#include <vector>
#include <mutex>
#include <string>

using namespace std;

class MySQLPool {
public:
    static MySQLPool& instance();

    bool init(const string& host, const string& user, const string& passwd,
              const string& dbname, int port = 3306, int size = 8);

    MYSQL* get();
    void put(MYSQL* conn);

private:
    MySQLPool() = default;
    MYSQL* create_conn();
    void close_all();

    vector<MYSQL*> _pool;
    mutex _mtx;

    string _host;
    string _user;
    string _passwd;
    string _dbname;
    int _port;
    int _size;
};

class MySQLGuard {
public:
    MySQLGuard() {
        _conn = MySQLPool::instance().get();
    }
    ~MySQLGuard() {
        MySQLPool::instance().put(_conn);
    }
    MYSQL* conn() { return _conn; }
private:
    MYSQL* _conn;
};