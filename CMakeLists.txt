cmake_minimum_required(VERSION 3.26)
project(dm_client C)

set(CMAKE_C_STANDARD 11)

add_executable(dm_client src/main.c
        src/net_utils.c
        src/net_utils.h
        src/dm_mainHeader.h
        src/run_CLIcmd.c
        src/run_CLIcmd.h
        src/parsing.c
        src/parsing.h
        src/state_handler.c
        src/state_handler.h
        src/settings_module.c
        src/settings_module.h
        )

