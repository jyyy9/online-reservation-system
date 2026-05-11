#ifndef OPTYPE_H  //#ifndef ... #define ... #endif
#define OPTYPE_H  //防止头文件重复包含（C++ 必备）

/*
#ifndef OPTYPE_H  // 如果 OPTYPE_H 没有被定义过
#define OPTYPE_H  // 那现在定义它

// 这里放你的枚举、结构体、声明

#endif            // 结束
*/

enum class OP_TYPE
{
    LOGIN = 1,              // 登录
    REGISTER,               // 注册
    VIEW_AVAILABLE_BOOKING, // 查看有哪些可以预定
    BOOK_APPOINTMENT,       // 提交预约
    VIEW_MY_BOOKING,        // 查看我的预约
    CANCEL_MY_BOOKING,      // 取消我的预约
    LOGOUT                  // 退出
};

enum class GENDER{ERR,MAN,FEMALE};

#endif