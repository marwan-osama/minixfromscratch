#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <unistd.h>

void perform_work(long iterations) {
    volatile double x;
    long i;

    x = 0.0;
    for (i = 0; i < iterations; i++) {
        x += 1.0;
    }
}

int main(int argc, char *argv[]) {
    long iterations;
    struct tms start_tms, end_tms;
    clock_t start_time, end_time;
    long hz;
    double turnaround_time;
    clock_t user_time;
    clock_t sys_time;
    double burst_time;
    double waiting_time;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <iterations>\n", argv[0]);
        return 1;
    }

    iterations = atol(argv[1]);

    start_time = times(&start_tms);
    perform_work(iterations);
    end_time = times(&end_tms);

    hz = sysconf(_SC_CLK_TCK);
    turnaround_time = (double)(end_time - start_time) / hz;
    user_time = end_tms.tms_utime - start_tms.tms_utime;
    sys_time = end_tms.tms_stime - start_tms.tms_stime;
    burst_time = (double)(user_time + sys_time) / hz;
    waiting_time = turnaround_time - burst_time;

    printf("%d,%.4f,%.4f,%.4f\n", getpid(), turnaround_time, waiting_time, burst_time);
    return 0;
}
