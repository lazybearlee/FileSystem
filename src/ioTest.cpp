//
// Created by 李卓 on 24-6-11.
//

#include <fstream>
#include <iostream>
#include "FileSystem.h"
/**
 * 声明一个用于测试IO的结构体
 */
struct Test
{
    int a;
    char b;
    double c;
    char d[10];
};

/**
 * 测试IO，将结构体写入文件并读取
 */
void testIO()
{
    // 创建一个Test结构体
    Test t = {1, 'a', 3.14, "Hello"};
    // 打开文件
    std::ofstream ofs("test.dat", std::ios::binary | std::ios::out);
    // 写入结构体
    ofs.write(reinterpret_cast<char*>(&t), sizeof(Test));
    // 关闭文件
    ofs.close();
    // 打开文件读取
    std::ifstream ifs("test.dat", std::ios::binary | std::ios::in);
    // 读取结构体
    Test t2{};
    ifs.read(reinterpret_cast<char*>(&t2), sizeof(Test));
    // 输出结构体
    std::cout << t2.a << " " << t2.b << " " << t2.c << " " << t2.d << std::endl;
    // 关闭文件
    ifs.close();
}

/**
 * 测试超级块的IO
 */
// void testSuperBlockIO()
// {
//     // 创建一个超级块
//     SuperBlock sb = {

