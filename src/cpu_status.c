#include "cpu_status.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define BUFER_SIZE  256
typedef unsigned long long ULL;

typedef struct {
    ULL user;
    ULL nice;
    ULL system;
    ULL idle;
    ULL iotwait;
    ULL irq;
    ULL softirq;
    ULL reserved[3];
} curr_state_t;

typedef struct {
    ULL total_cpu;
    ULL idle_cpu;
    ULL iotwait_cpu;
} prev_state_t;

typedef struct {
    ULL cpu;
    ULL idle;
    ULL iot_wait;
} delta_t;

static curr_state_t curr_state;
static prev_state_t prev_state;
static delta_t delta;

void read_ram_usage(uint32_t *mem_usage) {
    if (mem_usage == NULL) {
        return;
    }

    char *p_memtotal, *p_memfree;
    int fd, num_read;
    uint8_t read_buff[BUFER_SIZE];
    uint32_t mem_total, mem_free;

    fd = open("/proc/meminfo", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    num_read = read(fd, read_buff, sizeof(read_buff));
    if (num_read < 0) {
        perror("read");
        return;
    }

    p_memtotal = strstr((char *)read_buff, "MemTotal:");
    if (p_memtotal != NULL) {
        sscanf(p_memtotal + 9, "%u", &mem_total);
    }

    p_memfree = strstr((char *)read_buff, "MemFree:");
    if (p_memfree != NULL) {
        sscanf(p_memfree + 8, "%u", &mem_free);
    }

    *mem_usage = mem_total - mem_free;
    close(fd);
}


void read_cpu_usage(float *cpu_usage) {
    if (cpu_usage == NULL) {
        return;
    }

    int fd, num_read;
    char read_buff[BUFER_SIZE];
    char *p_cpu;

    fd = open("/proc/stat", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    num_read = read(fd, read_buff, sizeof(read_buff));
    if (num_read < 0) {
        perror("read");
        return;
    }

    p_cpu = strstr(read_buff, "cpu");
    if (p_cpu != NULL) {
        sscanf(p_cpu + 3, "%llu %llu %llu %llu %llu %llu %llu", &curr_state.user, &curr_state.nice , &curr_state.system, &curr_state.idle,
                                                                &curr_state.iotwait, &curr_state.irq, &curr_state.softirq);
    }

    unsigned long long total_cpu = curr_state.user + curr_state.nice + curr_state.system + curr_state.idle + curr_state.iotwait + curr_state.irq + curr_state.softirq;
    delta.cpu = total_cpu - prev_state.total_cpu;
    delta.idle = curr_state.idle - prev_state.idle_cpu;
    delta.iot_wait = curr_state.iotwait - prev_state.iotwait_cpu;

    prev_state.iotwait_cpu = curr_state.iotwait;
    prev_state.idle_cpu = curr_state.idle;
    prev_state.total_cpu = total_cpu;

    *cpu_usage = ((float)(delta.cpu - delta.idle - delta.iot_wait) / delta.cpu) * 100.0f;

    close(fd);
}