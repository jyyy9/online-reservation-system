#pragma once
#include <mysql/mysql.h>
#include <string>
#include <jsoncpp/json/json.h>
#include "../common/OpType.h"

using namespace std;

class mysql_client
{
public:
    mysql_client()
    {
        db_host = "127.0.0.1";
        db_user = "root";
        db_passwd = "123456";
        db_name = "Online_Res_DB";
        port = 3306;
    }
    mysql_client(string host, string user, string passwd, string dbname, int p = 3306)
        : db_host(host),
          db_user(user),
          db_passwd(passwd),
          db_name(dbname),
          port(p)
    {
    }

    ~mysql_client()
    {
        mysql_close(&mysql_con); // 关闭连接
    }
    bool mysql_con_ser();
    bool mysql_Register(const string &username, const string &passwd, const string &realname, const string &id_card, const string &usertel, GENDER gen);
    bool mysql_Login(const string &usertel, const string &passwd, string &username);
    bool mysql_User_Show_Ticket(Json::Value &resval);
    bool mysql_User_Book_Ticket(int ticket_id, string usertel, int num);
    bool mysql_User_Show_My_Ticlet(Json::Value &resval,string usertel);
    bool mysql_User_Cancel_Ticket(int ticket_id, string usertel, int num);

private:
    MYSQL mysql_con;  // 初始数据库对象
    string db_host;   // 数据库地址
    string db_user;   // 数据库用户名
    string db_passwd; // 数据库密码
    string db_name;   // 数据库名
    int port;

    bool mysql_user_begin();
    bool mysql_user_commit();
    bool mysql_user_rollback();
};
