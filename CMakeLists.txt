project (chinese2pinyin)
cmake_minimum_required (VERSION 2.8)

#version number
set (chinese2pinyin 1)
set (chinese2pinyin 0)
find_package (Threads)

add_executable (chinese2pinyin main.cpp chinese2pinyin.cpp)

TARGET_LINK_LIBRARIES (chinese2pinyin ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(chinese2pinyin sqlite3)

SET(CMAKE_INSTALL_PREFIX /usr/)
INSTALL (TARGETS chinese2pinyin DESTINATION bin)
INSTALL (DIRECTORY data/ DESTINATION share/chinese2pinyin/data)
