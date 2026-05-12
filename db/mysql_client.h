#pragma once
#include <mysql/mysql.h>
#include <string>
#include <jsoncpp/json/json.h>
#include "../common/OpType.h"
#include "mysql_pool.h"

using namespace std;

class mysql_client
{
public:
    mysql_client() = default;
    ~mysql_client() = default;
    
    bool mysql_Register(const string &username, const string &passwd, const string &realname, const string &id_card, const string &usertel, GENDER gen);
    bool mysql_Login(const string &usertel, const string &passwd, string &username);
    bool mysql_User_Show_Ticket(Json::Value &resval);
    bool mysql_User_Book_Ticket(int ticket_id, string usertel, int num);
    bool mysql_User_Show_My_Ticlet(Json::Value &resval,string usertel);
    bool mysql_User_Cancel_Ticket(int ticket_id, string usertel, int num);

private:

    bool mysql_user_begin(MYSQL* conn);
    bool mysql_user_commit(MYSQL* conn);
    bool mysql_user_rollback(MYSQL* conn);
};
