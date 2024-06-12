set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

# Define the source files
set(SOURCES
        main.cpp
        FileSystem.cpp
        FSHelper.cpp
        ioTest.cpp
        INodeCache.h
        VDisk.h
        INodeCache.cpp
        FileManager.h
        FileManager.cpp
)

# Create the executable
add_executable(LiFileSystem ${SOURCES})

