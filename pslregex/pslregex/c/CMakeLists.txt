cmake_minimum_required(VERSION 3.0.0)
project(csv_dn_parser VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

add_executable(execc utils.c parser.c trie.c analyzer/flashstart.c analyzer/training.c generator/1707.c generator/training.c main.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
