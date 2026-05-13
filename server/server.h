#pragma once

#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <jsoncpp/json/json.h>
#include "../common/OpType.h"
#include <mysql/mysql.h>
#include<hiredis/hiredis.h>
#include<vector>//存放连接
#include<mutex>//多线程锁
#include <queue>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>

using namespace std;

#define LIS_MAX 10

// 第一个类：处理监听套接字（服务器端）
class socket_listen
{
public:
    socket_listen()
    {
        sockfd = -1;
        m_port = 6789;
        m_ips = "127.0.0.1";
    }
    socket_listen(string ips, short port) : m_ips(ips), m_port(port)
    {
        sockfd = -1;
    }

    bool socket_init();
    int accept_client();

    void Set_base(struct event_base *base)
    {
        this->base = base;
    }

    int Get_sockfd() const
    {
        return sockfd;
    }

    struct event_base *Get_base() const
    {
        return base;
    }

    bool add_client(int client_fd);

private:
    int sockfd;
    short m_port;
    string m_ips;
    struct event_base *base;
};

// 第二个类：处理连接套接字（客户端）
class socket_con
{
public:
    socket_con(int fd) : c(fd) // 把传进来的fd赋值给变量c
    {
        c_ev = NULL; // 事件指针初始空
    }
    void Set_ev(struct event *ev)
    {
        c_ev = ev;
    }
    ~socket_con()
    {
        event_free(c_ev);
        close(c);
    }
    void Recv_data();
    void Send_err();
    void Send_ok();

    // 业务功能
    void User_Register();    // 注册
    void User_Login();       // 登录
    void User_Show_Ticket(); // 查看预约信息
    void User_Book_Ticket(); // 预定操作
    void User_Show_My_Ticlet();//查看我的预约信息
    void User_Cancel_Ticket();//取消预约

    void AddRef() { ref_++; }
    void ReleaseRef() { if (--ref_ == 0) delete this; }

private:
    int c;              // 文件描述符
    struct event *c_ev; // 是一个libevent事件,是一个指针
    Json::FastWriter writer;
    Json::Value val; // 存储从客户端传入的数据，Json格式

    std::atomic<int> ref_{1};

    string username; // 用户名
    string usertel;  // 电话
    string passwd;   // 密码
    string realname; // 真实姓名
    GENDER gen;      // 性别：0未知 1男 2女
    string id_card;  // 身份证号
};

// 线程池
class ThreadPool {
public:
    using Task = std::function<void()>;

    ThreadPool(int num = 10) {
        for (int i = 0; i < num; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(mtx_);
                        cv_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) return;
                        task = move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) t.join();
    }

    void add_task(Task task) {
        std::lock_guard<std::mutex> lock(mtx_);
        tasks_.emplace(move(task));
        cv_.notify_one();
    }

private:
    vector<std::thread> workers_;
    queue<Task> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_ = false;
};

// 全局线程池（extern）
extern ThreadPool g_pool;