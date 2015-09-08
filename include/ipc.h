//
// //EXTERMINATOR\\ an ncurses-based front-end for GDB written in C99
// First v. Dr Anton Gerdelan 3 Sep 2015
// https://github.com/capnramses/exterminator
//

#pragma once

#include <stdbool.h>

bool start_ipc (int argc, char** argv);

void child_ipc (int pipes[][2], int argc, char** argv);

void parent_ipc (int pipes[][2], int argc, char** argv);

void write_child (int pipe, const char* input);

long int read_child (int pipe, char* output);
