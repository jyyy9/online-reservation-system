#include "mysql_client.h"
#include "redis_client.h"  
#include <iostream>

bool mysql_client::mysql_con_ser()
{
    MYSQL *mysql = mysql_init(&mysql_con);
    if (mysql == NULL)
    {
        return false;
    }
    mysql = mysql_real_connect(mysql, db_host.c_str(), db_user.c_str(), db_passwd.c_str(), db_name.c_str(), port, NULL, 0);
    if (mysql == NULL)
    {
        cout << "connect db err" << endl;
        return false;
    }
    return true;
}
bool mysql_client::mysql_Register(const string &username, const string &passwd, const string &realname, const string &id_card, const string &usertel, GENDER gen)
{
    string sql = string("insert into `user_info` values(0,'") + username + string("','") + passwd + string("','") + realname + string("','") + id_card + string("','") + usertel + string("',") + to_string(static_cast<int>(gen)) + string(",NOW(),NOW(),1);");
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "注册失败！数据库错误！" << endl;
        cout << "数据库错误原因：" << mysql_error(&mysql_con) << endl;
        cout << "执行的SQL语句：" << sql << endl;
        return false;
    }
    Redis::del("user:info:" + usertel);
    return true;
}

bool mysql_client::mysql_Login(const string &usertel, const string &passwd, string &username)
{
    // 先看手机号码到底存不存在
    // select username,passwd from user_info where phone=17789023478;
    string sql = string("select username,passwd from user_info where phone='") + usertel + string("'");
    // mysql_query执行sql语句，成功返回0
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "登录失败！" << endl;
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con); // 获取结果集
    if (r == NULL)
    {
        cout << "提取结果集失败！" << endl;
        return false;
    }
    int num = mysql_num_rows(r); // 看结果集有多少行，0就是结果为空，用户没有注册
    if (num == 0)
    {
        mysql_free_result(r);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(r); // 取出结果集中的一行记录

    string password = row[1];
    if (password.compare(passwd) != 0)
    {
        return false;
    }
    username = row[0];
    mysql_free_result(r);

    return true;
}
bool mysql_client::mysql_User_Show_Ticket(Json::Value &resval)
{
    //1.先去查redis
    string redis_key = "ticket:all";
    string redis_val;
    if (Redis::get(redis_key, redis_val)) {  // 直接用你的 Redis::get()
        Json::Reader reader;
        reader.parse(redis_val, resval);
        return true;
    }
    // select ticket_id,ticket_name,total_stock,use_date, booked_count from ticket_info;
    string sql = "select ticket_id,ticket_name,price,total_stock,max_per_user,use_date,booked_count from ticket_info;";
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "sql语句执行失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }

    int n = mysql_num_rows(r);
    if (n == 0)
    {
        resval["status"] = "OK";
        resval["num"] = 0;
        return true;
    }
    resval["status"] = "OK";
    resval["num"] = n;

    // 将信息填入到json数组中
    for (int i = 0; i < n; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(r);
        Json::Value tmp;
        // 数字类型必须用 atoi / atof 转成 int/double
        tmp["ticket_id"] = atoi(row[0]);    // ticket_id
        tmp["ticket_name"] = row[1];        // 门票名称
        tmp["price"] = atof(row[2]);        // 价格（转浮点）
        tmp["total_stock"] = atoi(row[3]);  // 总库存
        tmp["max_per_user"] = atoi(row[4]); // 单人最大购买
        tmp["use_date"] = row[5];           // 使用日期
        tmp["booked_count"] = atoi(row[6]); // 已预约数量
        resval["ticket_arr"].append(tmp);
        /*
        tmp = JSON 对象（{}）
        ticket_arr = JSON 数组（[]）
        append = 把对象放进数组里
        */
    }
    mysql_free_result(r);
    //写入redis
    Json::FastWriter writer;
    Redis::set(redis_key, writer.write(resval), 300);
    return true;
}
bool mysql_client::mysql_user_begin()
{
    if (mysql_query(&mysql_con, "begin") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_user_commit()
{
    if (mysql_query(&mysql_con, "commit") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_user_rollback()
{
    if (mysql_query(&mysql_con, "rollback") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_User_Book_Ticket(int ticket_id, string usertel, int num)
{
    mysql_user_begin(); // 启动事务
    string s1 = string("select total_stock,booked_count,use_date,max_per_user from ticket_info where ticket_id=") + to_string(ticket_id);
    if (mysql_query(&mysql_con, s1.c_str()) != 0)
    {
        cout << "查询失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        cout << "提取结果集失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    int Num = mysql_num_rows(r);
    if (Num != 1)
    {
        cout << "记录行不唯一！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(r);
    string str_total_stock = row[0];  // 总票数
    string str_booked_count = row[1]; // 当前定出去的票
    string str_use_date = row[2];     // 使用时间
    string str_max_per_user = row[3]; // 最大购买量
    int tk_total_stock = atoi(str_total_stock.c_str());
    int tk_booked_count = atoi(str_booked_count.c_str());
    int tk_max_per_user = atoi(str_max_per_user.c_str());

    mysql_free_result(r);

    if (tk_total_stock <= tk_booked_count)
    {
        cout << "票已经售罄" << endl;
        mysql_user_rollback();
        return false;
    }
    /*SELECT IFNULL(SUM(book_num), 0) AS has_booked FROM user_order WHERE user_tel = 18902389403 AND ticket_id = 1 AND status = 1;*/
    string s2 = string("SELECT IFNULL(SUM(book_num), 0) AS has_booked FROM user_order WHERE user_tel = '") + usertel + string("' AND ticket_id = ") + to_string(ticket_id) + string(" AND status = 1;");
    if (mysql_query(&mysql_con, s2.c_str()) != 0)
    {
        cout << "查询订票记录失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r2 = mysql_store_result(&mysql_con);
    if (r2 == NULL)
    {
        cout << "提取结果集失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    int Num2 = mysql_num_rows(r2);
    if (Num2 != 1)
    {
        cout << "记录行不唯一！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row2 = mysql_fetch_row(r2);
    string booknum = row2[0];
    int booknum2 = atoi(booknum.c_str());

    mysql_free_result(r2);

    if (num + booknum2 > tk_max_per_user)
    {
        cout << "单人购买量已经超过上限" << endl;
        mysql_user_rollback();
        return false;
    }
    tk_booked_count += num;
    string s3 = string("update ticket_info set booked_count=") + to_string(tk_booked_count) + string(" where ticket_id=") + to_string(ticket_id) + string(";");
    if (mysql_query(&mysql_con, s3.c_str()) != 0)
    {
        cout << "修改预定票数失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    // 执行sql,尝试更新原有订单
    // UPDATE user_order SET book_num = book_num + 1 WHERE user_tel = 18902389403 AND ticket_id = 1 AND status = 1;
    string s4 = string("UPDATE user_order SET book_num = book_num + ") + to_string(num) + string(" WHERE user_tel ='") + usertel + string("' AND ticket_id =") + to_string(ticket_id) + string(" AND status = 1;");
    if (mysql_query(&mysql_con, s4.c_str()) != 0)
    {
        cout << "尝试更新原有订单失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    // 判断有没有在原数据上更新，1更新了，0没有更新（要新增）
    int affected_rows = mysql_affected_rows(&mysql_con);
    if (affected_rows == 0)
    {
        // 拼接插入SQL
        string s5 = "INSERT INTO user_order(user_tel, ticket_id, book_num, use_date) "
                    "VALUES('" +
                    usertel + "', " + to_string(ticket_id) + ", " + to_string(num) + ", '" + str_use_date + "');";

        if (mysql_query(&mysql_con, s5.c_str()) != 0)
        {
            cout << "新增订单失败！" << endl;
            mysql_user_rollback();
            return false;
        }
    }
    mysql_user_commit();
    Redis::del("ticket:stock:" + to_string(ticket_id));
    Redis::del("ticket:all");
    return true;
}
bool mysql_client::mysql_User_Show_My_Ticlet(Json::Value &resval, string usertel)
{
    //先查redis
    string key = "user:order:" + usertel;
    string val;

    if (Redis::get(key, val)) {
        Json::Reader reader;
        reader.parse(val, resval);
        return true;
    }
    // select *from user_order;
    string sql = string("select *from user_order where user_tel='") + usertel + string("';");
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "sql语句执行失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }

    int n = mysql_num_rows(r);
    if (n == 0)
    {
        resval["status"] = "OK";
        resval["num"] = 0;
        return true;
    }
    resval["status"] = "OK";
    resval["num"] = n;

    // 将信息填入到json数组中
    for (int i = 0; i < n; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(r);
        Json::Value tmp;

        tmp["order_id"] = atoi(row[0]);  // order_id (预约ID)
        tmp["user_tel"] = row[1];        // 用户手机号
        tmp["ticket_id"] = atoi(row[2]); // 门票 ID
        tmp["book_num"] = atoi(row[3]);  // 预约数量
        tmp["use_date"] = row[4];        // 使用时间
        tmp["create_time"] = row[5];     // 订单创建时间
        tmp["status"] = atoi(row[6]);    // 状态(1未核销 0已核销)

        resval["ticket_arr"].append(tmp);
    }

    Json::FastWriter writer;
    Redis::set(key, writer.write(resval), 60);
    return true;
}
bool mysql_client::mysql_User_Cancel_Ticket(int ticket_id, string usertel, int num)
{
    mysql_user_begin(); // 启动事务
    string s1 = string("select book_num from user_order where user_tel='") + usertel + string("' AND ticket_id = ")+to_string(ticket_id)+string(" AND status = 1;");
    if (mysql_query(&mysql_con, s1.c_str()) != 0)
    {
        cout << "查询失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r1 = mysql_store_result(&mysql_con);
    if (r1 == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    int n1 = mysql_num_rows(r1);
    if (n1 == 0)
    {
        cout << "查不到此人的预定信息" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row1 = mysql_fetch_row(r1);

    int book_num = atoi(row1[0]);

    mysql_free_result(r1); // 释放
    // 如果取消的数量大于预约的数量，失败
    if (book_num < num)
    {
        cout << "取消票数过多！" << endl;
        mysql_user_rollback();
        return false;
    }
    else if (book_num == num)
    {
        string s2=string("DELETE FROM user_order WHERE user_tel = '")+usertel+string("' AND ticket_id =")+to_string(ticket_id)+string(" AND status = 1;");
        if (mysql_query(&mysql_con, s2.c_str()) != 0)
        {
            cout << "取消预定票数失败！" << endl;
            mysql_user_rollback();
            return false;
        }
    }
    else
    {
        book_num -= num;
        // user_order
        string s3 = string("UPDATE user_order SET book_num = ") + to_string(book_num) + string(" WHERE user_tel = '") + usertel + string("' AND ticket_id = ")+to_string(ticket_id)+string(" AND status = 1;");
        if (mysql_query(&mysql_con, s3.c_str()) != 0)
        {
            cout << "修改预定票数失败！" << endl;
            mysql_user_rollback();
            return false;
        }
    }

    // ticket_info
    string s4 = string("select booked_count from ticket_info where ticket_id=") + to_string(ticket_id) + string(";");
    if (mysql_query(&mysql_con, s4.c_str()) != 0)
    {
        cout << "查询失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r2 = mysql_store_result(&mysql_con);
    if (r2 == NULL)
    {
        cout << "提取结果集失败！" << mysql_error(&mysql_con) << endl;
        return false;
    }
    int n2 = mysql_num_rows(r2);
    if (n2 == 0)
    {
        cout << "查不到此门票的信息" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_ROW row2 = mysql_fetch_row(r2);

    int bookedcount = atoi(row2[0]);
    mysql_free_result(r2); // 释放
    bookedcount -= num;
    // UPDATE ticket_info SET booked_count = 1 WHERE ticket_id = 2;
    string s5 = string("UPDATE ticket_info SET booked_count =") + to_string(bookedcount) + string(" WHERE ticket_id =") + to_string(ticket_id) + string(";");
    if (mysql_query(&mysql_con, s5.c_str()) != 0)
    {
        cout << "修改全部预定票数失败！" << endl;
        mysql_user_rollback();
        return false;
    }
    mysql_user_commit();
    Redis::del("user:order:" + usertel);
    Redis::del("ticket:stock:" + to_string(ticket_id));
    Redis::del("ticket:all");
    return true;
}