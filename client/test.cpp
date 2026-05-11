#include<stdio.h>
#include<string>
#include<string.h>
#include<iostream>
#include<jsoncpp/json/json.h>

using namespace std;

int main()
{
    /*---------客户端做的事----------*/
    //1.构建JSON对象，；往里面放入数据（结构化数据）
    Json::Value val;
    val["name"]="小王";
    val["age"]=23;

    //buff接收字符串
    char buff[256]={0};

    //2.对对象进行序列化，复制到缓冲区BUFF
    //toStyledString():把VAL转化成带格式的字符串
    //c_str()：把字符串转化成C语言字符串，方便TCP发送
    strcpy(buff,val.toStyledString().c_str());
    cout<<"buff:"<<buff<<endl;

    //send(buff):发送给服务器

    /*---------服务端做的事----------*/

    //recv()：接收客户端发回来的 JSON 字符串

    //反序列化
    Json::Value res;
    Json::Reader Read;//Json::Reader 负责解析字符串
    Read.parse(buff,res);//parse：把字符串 重新解析成 JSON 对象

    /*
    **Read.parse () 返回 bool
    true = JSON 合法
    false = JSON 非法 / 错误 / 残缺 **
    */

    string name=res["name"].asString();//从 JSON 里把值取出来 → 转成 C++ 字符串 string
    int age=res["age"].asInt();

    cout<<"name:"<<name<<",age:"<<age<<endl;


}