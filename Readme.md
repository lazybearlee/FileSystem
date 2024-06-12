# 类Linux实现——简单文件系统

## 1. 简介

本项目是一个简单的文件系统，实现了以下功能：

1. 文件系统虚拟磁盘
    1. `new`创建虚拟磁盘，包括超级块、位示图、inode表、数据区 ✔️
    2. `format`格式化虚拟磁盘，设置超级块、位示图、inode表、数据区的初始值 ✔️
    3. `sys`挂载虚拟磁盘，加载超级块、位示图、inode表、数据区 ✔️
    4. `exit`卸载/保存虚拟磁盘，保存超级块、位示图、inode表、数据区 ✔️
    5. 空闲块管理，通过位图分配和回收inode和数据块，数据块额外采用成组链接法链接 ✔️
    6. 磁盘索引，目前在数据结构上支持单/双/三级索引，但是暂时没有实现多级索引的具体操作 ✔️
2. 文件操作
    1. `create/touch`创建文件—— ✔️
        1. 暂时不支持递归创建目录
        2. 不支持文件名中包含空格
    2. `delete/rm`删除文件 ✔️
        1. 暂时不支持`-r`递归删除目录
    3. `read/cat`查看文件——支持按文件名、大小、偏移量查看文件内容 ️✔️
    4. `write/append`修改文件——支持按文件名、大小、偏移量修改文件内容 ️✔️
3. 目录操作
    1. `mkdir`创建目录——支持`-p`参数递归创建目录 ✔️
    2. `rmdir`删除目录 ✔️
        1. 暂时不支持`-p`递归删除目录
        2. 暂时不支持`--ignore-fail-on-non-empty`参数
    3. `ls`查看目录内容——支持-l参数查看文件详细信息 ✔️
    4. `cd`切换目录——支持相对路径和绝对路径 ✔️
4. 权限与用户
    1. 完成了底层对于用户的权限控制，但是暂时没有实现用户的概念，所以所有文件都是root用户的 ✔️
    2. 暂时不支持权限管理与用户管理相关的命令

## 2. 项目结构

```
FILESYSTEM
│  .gitignore
│  CMakeLists.txt
│  Readme.md
│  实验报告.md  
├─src
│      CMakeLists.txt
│      FileSystem.cpp
│      FileSystem.h
│      FSHelper.cpp
│      FSHelper.h
│      ioTest.cpp
│      main.cpp
│      
└─tests
        CMakeLists.txt
```

## 2. 使用方法

### 可执行文件

#### Windows

双击可执行程序执行：

![img.png](img/img.png)

#### Linux

终端输入以下命令执行：

```shell
./LiFileSystem
```

### 源码编译

本项目采用CMake+Ninja构建，编译源码需要安装CMake和Ninja。

在项目根目录下执行以下命令：

```shell
mkdir build
cd build
cmake -G Ninja ..
ninja
```

编译完成后，可执行文件在`build`目录下。

## 3. 使用流程

### 创建虚拟磁盘

使用以下命令创建虚拟磁盘：

```shell
new example
```

这会在当前目录下创建一个名为`example.vdisk`的虚拟磁盘。
并且系统会自动格式化和挂载这个虚拟磁盘。

### 载入虚拟磁盘

使用以下命令载入虚拟磁盘：

```shell
sys example.vdisk
```

这会载入当前目录下的`example.vdisk`虚拟磁盘。

### 退出程序

使用以下命令退出程序：

```shell
quit
```

这会保存当前文件系统的状态，并退出程序。

### clear清屏

使用以下命令清屏：

```shell
clear
```

**！！！在进入文件系统后，可以执行以下命令：**

### 格式化虚拟磁盘

使用以下命令格式化虚拟磁盘：

```shell
format
```

这会格式化当前虚拟磁盘，清空所有数据。

### 创建文件

使用以下命令创建文件：

```shell
create file1
```

这个命令支持相对路径与绝对路径。

### 读取文件

使用以下命令读取文件：

```shell
read file1 -n 10 -o 0
```

这个命令会从文件`file1`的偏移量`0`处读取`10`个字节的内容。

### 写入文件

使用以下命令写入文件：

```shell
write file1 "Hello World" -o 0
```

这个命令会从文件`file1`的偏移量`0`处写入`Hello World`。

### 创建目录

使用以下命令创建目录：

```shell
mkdir dir1
```

这个命令支持相对路径与绝对路径，同时也支持递归创建目录：

```shell
mkdir -p dir1/dir2/dir3
```

### 查看目录

使用以下命令查看目录：

```shell
ls dir1
```

这个命令会列出`dir1`目录下的所有文件和目录。
当然，这个命令也支持`-l`参数，可以查看详细信息。

详细信息包括：文件类型、权限、所有者、组、大小、修改时间和文件名。

### 删除文件

使用以下命令删除文件：

```shell
delete file1
```

这个命令会删除`file1`文件，支持相对路径与绝对路径。

### 删除目录

使用以下命令删除目录：

```shell
rmdir dir1
```

这个命令会删除`dir1`目录，支持相对路径与绝对路径。目前这个命令不支持递归删除目录。

### 切换目录

使用以下命令切换目录：

```shell
cd dir1
```

这个命令会切换到`dir1`目录，支持相对路径与绝对路径，如果目录不存在会切换到当前用户的主目录。

