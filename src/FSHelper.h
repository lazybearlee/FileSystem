//
// Created by 李卓 on 24-6-10.
//
#ifndef HELP_H
#define HELP_H

#include <iostream>

/**
* 帮助文档类
* @brief 显示帮助文档
*/
class Helper {
public:
    /**
    * 显示帮助文档
    */
    static void showHelpInformation(const std::string& command);
private:
    // new命令帮助信息
    static void showNewHelp();
    // sys命令帮助信息
    static void showSysHelp();
    // cd命令帮助信息
    static void showCdHelp();
    // ls命令帮助信息
    static void showLsHelp();
    // ...
};


#endif //HELP_H
