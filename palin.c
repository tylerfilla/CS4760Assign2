/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

//
// palin.c
// This is the main file for the palin executable.
//

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>

/**
 * Determine if an ASCII string is a palindrome.
 *
 * @param first A pointer to the first character
 * @param last A pointer to the last character
 * @return Nonzero if such is the case, otherwise zero
 */
int is_palindrome(const char* first, const char* last)
{
    // Recursive base case
    if (first >= last)
        return 1;

    if (*first == *last)
        return is_palindrome(first + 1, last - 1);

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <seq> <str>\n", argv[0]);
        return 1;
    }

    // Parse seq argument
    char* fail_seq = NULL;
    size_t arg_seq = strtoul(argv[1], &fail_seq, 10);

    if (fail_seq == NULL || *fail_seq != '\0')
    {
        fprintf(stderr, "%s: invalid seq: %s\n", argv[0], argv[1]);
        return 1;
    }

    // Parse str argument
    char* fail_str = NULL;
    size_t arg_str = strtoul(argv[2], &fail_str, 10);

    if (fail_str == NULL || *fail_str != '\0')
    {
        fprintf(stderr, "%s: invalid str: %s\n", argv[0], argv[2]);
        return 1;
    }

    // Create identical IPC key for shared memory
    key_t ipc_key = ftok("/bin/echo", 'Q');
    if (ipc_key == -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("ftok(3) failed");
        return 1;
    }

    // Allocate and attach memory for shared buffer
    int shm_id = shmget(ipc_key, 0, 0);
    if (shm_id == -1)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("shmget(2) failed");
        return 1;
    }
    char* shared_buffer = shmat(shm_id, NULL, 0);
    if (shared_buffer == NULL)
    {
        fprintf(stderr, "%s: ", argv[0]);
        perror("shmat(2) failed");
        return 1;
    }

    size_t k;
    memcpy(&k, shared_buffer, sizeof(size_t));
    printf("%ld\n", k);

    size_t p;
    memcpy(&p, shared_buffer + sizeof(size_t) + 5 * sizeof(size_t), sizeof(size_t));
    printf("%ld\n", p);

    char* str = &shared_buffer[p];
    printf("%s\n", str);

    char test[] = "a56g65a";
    printf("%d\n", is_palindrome(&test[0], &test[6]));

    return 0;
}
