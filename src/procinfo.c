#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static void usage(const char *a){fprintf(stderr,"Usage: %s <pid>\n",a); exit(1);}
static int isnum(const char*s){for(;*s;s++) if(!isdigit(*s)) return 0; return 1;}
int main(int c,char**v){
    if(c!=2||!isnum(v[1])) usage(v[0]);

    char path[256];
    FILE *f;

    snprintf(path, sizeof(path), "/proc/%s/stat", v[1]);
    f = fopen(path, "r");
    if (!f)
        DIE("stat");

    int pid, ppid;
    char comm[256], state;
    unsigned long utime, stime;

    if (fscanf(f, "%d %255s %c %d", &pid, comm, &state, &ppid) != 4)
        DIE_MSG("Failed to parse /proc/<pid>/stat");

    for (int i = 0; i < 10; i++) {
        if (fscanf(f, "%*s") != 0)
            DIE_MSG("Failed to parse /proc/<pid>/stat");
    }

    if (fscanf(f, "%lu %lu", &utime, &stime) != 2)
        DIE_MSG("Failed to read CPU times");

    fclose(f);

    long ticks = sysconf(_SC_CLK_TCK);
    double user_time = utime / (double)ticks;
    double sys_time = stime / (double)ticks;

    snprintf(path, sizeof(path), "/proc/%s/status", v[1]);
    f = fopen(path, "r");
    if (!f)
        DIE("status");

    char line[256];
    long vmrss = -1;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmRSS: %ld kB", &vmrss) == 1)
            break;
    }
    fclose(f);

    snprintf(path, sizeof(path), "/proc/%s/cmdline", v[1]);
    f = fopen(path, "r");
    if (!f)
        DIE("cmdline");

    char cmdline[4096];
    size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, f);
    fclose(f);

    if (n == 0) {
        strcpy(cmdline, "-");
    } else {
        cmdline[n] = '\0';
        for (size_t i = 0; i < n - 1; i++)
            if (cmdline[i] == '\0')
                cmdline[i] = ' ';
    }

    printf("PID:%d\n", pid);
    printf("State:%c\n", state);
    printf("PPID:%d\n", ppid);
    printf("Cmd:%s\n", cmdline);
    printf("CPU:%.0f %.3f\n", user_time, sys_time);
    printf("VmRSS:%ld\n", vmrss);

    return 0;
}