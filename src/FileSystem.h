//
// Created by 李卓 on 24-6-9.
//

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include "VirtualDisk.h"



class FileSystem {
public:
    void create(const std::string &systemName);
    void load(const std::string &filename);
    void save();
    void execute(const std::string &command);
private:
    VirtualDisk disk;
    std::string currentFilename;
};

#endif // FILESYSTEM_H

