#include "system_log.h"

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

void write_sys_log(const char *tag, const char *msg) {
    if (tag == NULL || msg == NULL) {
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t == NULL) {
        return;
    }

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/mnt/data/log_%04d%02d%02d.txt", t->tm_year+1900, t->tm_mon+1, t->tm_mday);

    char log[256];
    int ret = snprintf(log, sizeof(log), "%02d:%02d:%02d    [%s]  %s\n", t->tm_hour, t->tm_min, t->tm_sec, tag, msg);

    if (ret < 0) {
        return;
    }

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd < 0) {
        perror("open");
        close(fd);
        return;
    }

    ssize_t num_write = write(fd, log, strlen(log));
    if (num_write < 0) {
        perror("write");
        close(fd);
        return;
    }

    printf("%s", log);

    close(fd);
}

void manage_sys_log(void) {
    DIR *dir = opendir("/mnt/data");
    if (dir == NULL) {
        perror("dir");
        return;
    }

    time_t now = time(NULL);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "log_", 4) == 0) {
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s", entry->d_name);

            struct stat file_stat;
            if (stat(file_path, &file_stat) == 0) {
                double diff = difftime(now, file_stat.st_mtime);
                if (diff > (3*24*3600)) {
                    unlink(file_path);
                }
            }
        }
    }

    closedir(dir);
}