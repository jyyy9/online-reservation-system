#include<stdio.h>
#include<string>
#include<string.h>
#include<iostream>
#include<jsoncpp/json/json.h>

using namespace std;

int main()
{
    Json::Value val;
    val["tel"] = "1370000000";
    val["passwd"] = "123456";
    val["age"] = 23;

    cout<<val.toStyledString()<<endl;

    //---------------------------------
    Json::Value root;
    Json:: Reader Read;
    Read.parse(val.toStyledString().c_str(),root);
    cout<<root["age"].asInt()<<endl;
    cout<<root["tel"].asString()<<endl;
    string s = root["tel"].asString();
    cout<<s<<endl;

}