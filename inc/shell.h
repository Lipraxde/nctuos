#ifndef SHELL_H
#define SHELL_H

#define SHELL_HIST_MAX 10
#define BUF_LEN 1024

struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char** argv);
};

int mon_chgcolor(int argc, char **argv);
int mon_help(int argc, char **argv);
int mon_kerninfo(int argc, char **argv);
int print_tick(int argc, char **argv);

#endif
