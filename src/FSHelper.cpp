//
// Created by 李卓 on 24-6-10.
//

#include <string>

#include "FSHelper.h"

/**
 * 帮助信息
 * @param command
 */
void Helper::showHelpInformation(const std::string& command)
{
    // 如果为help命令，那么打印基础帮助信息
    if (command == "help")
    {
        std::cout << "Usage: help [command]" << std::endl;
        // 支持的命令
        std::cout << "Commands:" << std::endl;
        std::cout << "  help [command]  Display help information." << std::endl;
        std::cout << "About file system:" << std::endl;
        std::cout << "  new [name]      Create file system." << std::endl;
        std::cout << "  sfs [name]      Load file system." << std::endl;
        std::cout << "  save            Save file system." << std::endl;
        std::cout << "  exit            Exit file system." << std::endl;
        std::cout << "  clear           Clear screen." << std::endl;
        std::cout << "  super           Print super block." << std::endl;
        std::cout << "About file:" << std::endl;
        std::cout << "  touch file    Create file." << std::endl;
        std::cout << "  cat file [-n size] [-o offset]     Display file." << std::endl;
        std::cout << "  echo file content     Write content to file." << std::endl;
        std::cout << "  open file [-m rwa(could be mixed)]    Open file." << std::endl;
        std::cout << "  close fd    Close file." << std::endl;
        std::cout << "  read fd size     Read file." << std::endl;
        std::cout << "  write file content   Write file." << std::endl;
        std::cout << "  seek fd offset     Seek file." << std::endl;
        std::cout << "  delete file   Delete file." << std::endl;
        std::cout << "About directory:";
        std::cout << "  mkdir [-p] dir     Make directory." << std::endl;
        std::cout << "  rmdir [-p] [--ignore-fail-on-non-empty] dir     Remove directory." << std::endl;
        std::cout << "  ls [-l] dir   List directory with details." << std::endl;
        std::cout << "  cd dir        Change directory." << std::endl;
        std::cout << "  pwd             Print working directory." << std::endl;
        // TODO: Add more commands
        std::cout << "TODO: Add more commands." << std::endl;
        std::cout << "  su [user]       Switch user." << std::endl;
        std::cout << "  useradd [user]  Add user." << std::endl;
        std::cout << "  userdel [user]  Delete user." << std::endl;
        std::cout << "  passwd [user]   Change password." << std::endl;
        std::cout << "  who             List users." << std::endl;
        std::cout << "  group           List groups." << std::endl;
        std::cout << "  chmod [file]    Change mode." << std::endl;
        std::cout << "  chown [file]    Change owner." << std::endl;
        std::cout << "  chgrp [file]    Change group." << std::endl;
    }
    // 如果是其他命令，暂时不管，到时候转到对应的函数
    else
    {
        std::cout << "No help information for command '" << command << "'." << std::endl;
    }
}

void Helper::showNewHelp()
{
}

