README

This program is called smallsh - it is a small shell written in C that implements a subset of features of well-known shells. The features of this program include:
- a prompt for running commands
- handling of blank lines and commented lines
- input and output redirection
- foreground and background processes
- three built-in commands: exit, cd, and status
- execution of other commands by creating new processes via the execvp function
- custom handlers for SIGINT and SIGTSTP
- variable expansion for $$

To compile, run the following command (requires gcc compiler):
gcc -g -Wall --std=gnu99 -o smallsh main.c
