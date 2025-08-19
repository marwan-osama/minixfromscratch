#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/resource.h>

#define MAX_JOBS 20

/* Defines a single job in our test scenario */
struct job {
    long iterations;
    int nice_val;
    int delay_sec;
};

/* --- Define Your Test Scenario Here --- */
/* An array of jobs to be executed. */
struct job scenario[] = {
    /* iterations, nice value, delay before start */
    { 20000000,  0,  0 }, /* P1: Long job, arrives at t=0 */
    { 10000000,  0,  2 }, /* P2: Short job, arrives at t=2 */
    { 20000000,  0,  2 }, /* P3: Long job, arrives at t=4 (2+2) */
    { 10000000, -10, 2 }, /* P4: Short, HIGH PRIORITY job, arrives at t=6 (4+2) */
    { 10000000,  10, 2 }  /* P5: Short, LOW PRIORITY job, arrives at t=8 (6+2) */
};
#define NUM_JOBS (sizeof(scenario)/sizeof(struct job))

int main(int argc, char *argv[]) {
    int i;
    int pids[MAX_JOBS];
    int pipes[MAX_JOBS][2];
    char worker_path[] = "/usr/src/commands/runtest/worker";

    double total_turnaround;
    double total_waiting;

    total_turnaround = 0.0;
    total_waiting = 0.0;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <algorithm_name>\n", argv[0]);
        exit(1);
    }
    printf("Starting test for %s...\n", argv[1]);

    /* --- Launch all jobs according to the scenario --- */
    for (i = 0; i < NUM_JOBS; i++) {
        /* Delay for the specified arrival time */
        if (scenario[i].delay_sec > 0) {
            sleep(scenario[i].delay_sec);
        }

        /* Create a pipe to capture the child's output */
        if (pipe(pipes[i]) < 0) {
            perror("pipe failed");
            exit(1);
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed");
            exit(1);
        }

        if (pids[i] == 0) { /* --- Child Process --- */
            char iterations_str[20];
            char *worker_args[3];

            /* Set priority if specified */
            if (scenario[i].nice_val != 0) {
                if (setpriority(PRIO_PROCESS, 0, scenario[i].nice_val) != 0) {
                    perror("setpriority failed");
                }
            }

            /* Redirect stdout to the pipe */
            close(pipes[i][0]); /* Close read end */
            dup2(pipes[i][1], STDOUT_FILENO); /* Duplicate write end to stdout */
            close(pipes[i][1]);

            /* Prepare arguments for exec */
            sprintf(iterations_str, "%ld", scenario[i].iterations);
            worker_args[0] = worker_path;
            worker_args[1] = iterations_str;
            worker_args[2] = NULL;

            /* Execute the worker program */
            execv(worker_path, worker_args);

            /* execv only returns on error */
            perror("execv failed");
            exit(127);
        }
        
        /* --- Parent Process --- */
        close(pipes[i][1]); /* Close write end */
    }

    printf("All processes launched. Waiting for completion and collecting results...\n");

    /* --- Collect results and wait for children to finish --- */
    for (i = 0; i < NUM_JOBS; i++) {
        char buffer[1024];
        int count;
        int pid;
        double turnaround, waiting, burst;

        count = read(pipes[i][0], buffer, sizeof(buffer) - 1);
        if (count > 0) {
            buffer[count] = '\0';
            
            /* Parse the CSV output from the worker */
            sscanf(buffer, "%d,%lf,%lf,%lf", &pid, &turnaround, &waiting, &burst);

            printf("  PID %d finished. Turnaround: %.4fs, Waiting: %.4fs\n",
                   pid, turnaround, waiting);

            total_turnaround += turnaround;
            total_waiting += waiting;
        }
        close(pipes[i][0]); /* Close the read end of the pipe */
    }
    
    /* Wait for all child processes to ensure they are cleaned up by the kernel */
    for(i = 0; i < NUM_JOBS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("\nTest for %s complete.\n", argv[1]);
    printf("------------------------------------------\n");
    printf("Average Turnaround Time: %.4f\n", total_turnaround / NUM_JOBS);
    printf("Average Waiting Time:    %.4f\n", total_waiting / NUM_JOBS);
    printf("------------------------------------------\n");

    return 0;
}
