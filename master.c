/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

//
// master.c
// This is the main file for the master executable.
//

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "perrorf.h"
#include "shared.h"

#define DEFAULT_STRINGS_FILENAME "strings.in"

/**
 * The file path to the executable image of this process.
 */
static char* image_path = NULL;

/**
 * The client bundle instance.
 */
static client_bundle_t* bundle = NULL;

/**
 * The shared memory identifier.
 */
static int shmid = -1;

/**
 * Handle all manual cleanup operations.
 */
static void do_cleanup();

static void handle_sigalrm(int sig)
{
    fprintf(stderr, "%s: timeout encountered\n", image_path);

    // For simplicity, we treat the timeout feature as an interrupt mechanism
    // Send SIGINT to all in this process group
    // This propagates to all children
    killpg(getpgrp(), SIGINT);

    exit(2);
}

static void handle_sigint(int sig)
{
    fprintf(stderr, "%s: execution interrupted\n", image_path);
    exit(2);
}

static void print_help(FILE* dest)
{
    fprintf(dest, "Usage: %s [option...]\n\n", image_path);
    fprintf(dest, "Supported options:\n");
    fprintf(dest, "    -f <file>   Read strings from <file> (defaults to strings.in)\n");
    fprintf(dest, "    -h          Display this information\n");
    fprintf(dest, "    -t <time>   Set timeout to <time> milliseconds (default 60000)\n");
}

static void print_usage(FILE* dest)
{
    fprintf(dest, "Usage: %s [option...]\n", image_path);
    fprintf(dest, "Try `%s -h' for more information.\n", image_path);
}

int main(int argc, char* argv[])
{
    image_path = argv[0];

    ///////////////////////////////////
    // Handle command-line arguments //
    ///////////////////////////////////

    int print_quit_help = 0;
    char* strings_filename = NULL;
    unsigned long time_limit = 60000;

    int opt;
    while ((opt = getopt(argc, argv, "hf:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            print_quit_help = 1;
            break;
        case 'f':
            strings_filename = strdup(optarg);
            break;
        case 't':
            time_limit = strtoul(optarg, NULL, 10);
            break;
        default:
            print_usage(stderr);
            return 1;
        }
    }

    if (print_quit_help)
    {
        print_help(stdout);
        return 1;
    }

    // Use default filename if none provided
    if (strings_filename == NULL)
    {
        strings_filename = strdup(DEFAULT_STRINGS_FILENAME);
    }

    ///////////////////////////
    // Set up event handlers //
    ///////////////////////////

    // Set a cleanup handler for graceful modes of termination
    atexit(&do_cleanup);

    // Handle SIGALRM signal (triggered by timeout)
    struct sigaction sigaction_sigalrm = {};
    sigaction_sigalrm.sa_handler = &handle_sigalrm;
    if (sigaction(SIGALRM, &sigaction_sigalrm, NULL))
    {
        perrorf("%s: cannot handle SIGALRM: sigaction(2) failed", image_path);
        return 1;
    }

    // Handle SIGINT signal (most likely triggered by terminal ^C key combo)
    struct sigaction sigaction_sigint = {};
    sigaction_sigint.sa_handler = &handle_sigint;
    if (sigaction(SIGINT, &sigaction_sigint, NULL))
    {
        perrorf("%s: cannot handle SIGINT: sigaction(2) failed", image_path);
        return 1;
    }

    // Split time limit into seconds and milliseconds
    unsigned long time_limit_seconds = time_limit / 1000;
    unsigned long time_limit_milliseconds = time_limit % 1000;

    // Set timeout after time_limit milliseconds
    struct itimerval timer = {};
    timer.it_value.tv_sec = time_limit_seconds;
    timer.it_value.tv_usec = time_limit_milliseconds * 1000;
    if (setitimer(ITIMER_REAL, &timer, NULL))
    {
        perrorf("%s: cannot set timeout: setitimer(2) failed", image_path);
        return 1;
    }

    /////////////////////////////
    // Read strings input file //
    /////////////////////////////

    // Local (non-shared) string buffer
    // This is the working memory for organizing the input strings before we share them
    // We can't getdelim(3) into shared memory, so we need to use normal memory first
    // Additionally, this lets use allocate exactly the amount of shared memory necessary
    struct
    {
        /** String data buffer */
        char** buf;

        /** Buffer capacity. */
        size_t cap;

        /** Current number of strings. */
        size_t num;

        /** Current mass of strings (size of all strings packed tightly, separated by null characters). */
        size_t mass;
    } strings = { NULL, 0, 0, 0 };

    // Allocate initial capacity of ten
    strings.buf = calloc(10, sizeof(char*));
    strings.cap = 10;

    if (strings.buf == NULL)
    {
        perrorf("%s: unable to allocate initial buffer capacity", image_path);
        return 1;
    }

    // Open input strings file for read
    FILE* strings_file = fopen(strings_filename, "r");

    if (strings_file == NULL)
    {
        perrorf("%s: unable to read strings file: %s", image_path, strings_filename);
        free(strings_filename);
        return 1;
    }

    free(strings_filename);

    // Read strings from file into buffer
    // For short strings, getdelim(3) can be memory inefficient, but this shouldn't matter
    size_t line_buffer_size = 0;
    while (getdelim(&strings.buf[strings.num], &line_buffer_size, '\n', strings_file) != -1)
    {
        strings.num++;
        char* line = strings.buf[strings.num - 1];
        size_t line_len = strlen(line);

        // Trim newline characters from string
        while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
        {
            line[line_len - 1] = '\0';
            line_len--;
        }

        // Add packed line length (including null terminator) to the total strings mass
        strings.mass += line_len + 1;

        // If buffer has reached capacity
        if (strings.num == strings.cap)
        {
            // Attempt to double the buffer size
            size_t new_cap = strings.cap * 2;
            char** new_buf = realloc(strings.buf, new_cap * sizeof(char*));

            if (new_buf == NULL)
            {
                perrorf("%s: unable to increase buffer size", image_path);
                return 1;
            }

            // Zero out any new elements in buffer and use it
            size_t cap_diff = new_cap - strings.cap;
            memset(new_buf + cap_diff, 0, cap_diff * sizeof(char*));
            strings.buf = new_buf;
            strings.cap = new_cap;
        }

        // Reset line buffer size to zero
        // getdelim(3) changes it after each call, but wants it to be zero!
        line_buffer_size = 0;
    }

    // If getdelim(3) exited with an error
    if (errno)
    {
        perrorf("%s: getdelim(3) failed", image_path);
        return 1;
    }

    // Close input strings file
    fclose(strings_file);

    ////////////////////////////////////////////
    // Set up shared memory and client bundle //
    ////////////////////////////////////////////

    // Get common IPC key
    key_t ipc_key = get_ipc_key();
    if (ipc_key == -1)
    {
        perrorf("%s: ftok(3) failed", image_path);
        return 1;
    }

    // Allocate and construct client bundle in shared memory
    shmid = shmget(ipc_key, sizeof_client_bundle_t(strings.num, strings.mass), IPC_CREAT | IPC_EXCL | 0600);
    if (shmid == -1)
    {
        perrorf("%s: shmget(2) failed", image_path);
        return 1;
    }
    bundle = shmat(shmid, NULL, 0);
    if (bundle == (void*) -1)
    {
        perrorf("%s: shmat(2) failed", image_path);
        return 1;
    }
    client_bundle_construct(bundle, strings.num, strings.mass);

    // Copy strings into client bundle
    for (size_t i = 0; i < strings.num; ++i)
    {
        client_bundle_append_string(bundle, strings.buf[i]);
    }

    // Free local string buffer
    // All strings are stored in shared memory now
    free(strings.buf);

    /////////////////////
    // Distribute work //
    /////////////////////

    // Loop through all strings
    for (size_t str = 0; str < strings.num; ++str)
    {
        // If number of workers is at max, wait for one child to die
        // In the meantime, at least one worker will transition to finished
        if (bundle->num_workers == MAX_WORKERS)
        {
            wait(NULL);
            bundle->num_workers--;
        }

        // Transition finished workers to ready state
        for (int i = 0; i < MAX_WORKERS; ++i)
        {
            if (bundle->worker_states[i] == WORKER_STATE_FINISHED)
            {
                bundle->worker_states[i] = WORKER_STATE_READY;
            }
        }

        // Find lowest available seq number (these get reused as workers finish)
        // The worker will hold onto this value for its entire life
        int seq = 0;
        for (; seq < MAX_WORKERS; ++seq)
        {
            if (bundle->worker_states[seq] == WORKER_STATE_READY)
                break;
        }

        // If no seq number is available, then either the workers are zombies or there's a fatal inconsistency
        if (seq == MAX_WORKERS)
            return 1;

        // Fork off a new worker process
        pid_t worker_pid = fork();

        if (worker_pid == -1)
        {
            // Currently in master after failed fork
            perrorf("%s: fork(2) failed", image_path);
            return 1;
        }
        else if (worker_pid == 0)
        {
            // Currently in child after successful fork

            // Convert seq and str to decimal text for command-line argument passing
            char arg_seq[32];
            snprintf(arg_seq, sizeof(arg_seq), "%d", seq);
            char arg_str[32];
            snprintf(arg_str, sizeof(arg_str), "%ld", str);

            // Execute palin program in this process
            char image_palin[] = "./palin";
            if (execv(image_palin, (char* []) { image_palin, arg_seq, arg_str, NULL }))
            {
                perrorf("child %d: %d: execv(3) failed", bundle->num_workers, getpid());
                _exit(EXIT_FAILURE);
            }
        }
        else
        {
            // Currently in master after successful fork
            // Mark worker as running and continue
            bundle->worker_states[seq] = WORKER_STATE_RUNNING;
            bundle->num_workers++;
        }
    }

    return 0;
}

static void do_cleanup()
{
    ////////////////////////
    // Clean up resources //
    ////////////////////////

    fprintf(stderr, "cleanup\n");

    // Wait for all children to die
    while (wait(NULL) > 0)
    {
    }

    // Destruct client bundle
    client_bundle_destruct(bundle);

    // Detach and deallocate shared memory as necessary
    if (bundle != NULL && shmdt(bundle) == -1)
    {
        perrorf("%s: shmdt(2) failed", image_path);
        return;
    }
    if (shmid != -1 && shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perrorf("%s: shmctl(2) failed", image_path);
        return;
    }
}
