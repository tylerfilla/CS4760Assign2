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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "perrorf.h"
#include "shared.h"

/**
 * The file path to the executable image of this process.
 */
static char* image_path;

/**
 * Determine if an ASCII string is a palindrome.
 *
 * @param first A pointer to the first character
 * @param last A pointer to the last character (must be >= first)
 * @return Nonzero if such is the case, otherwise zero
 */
static int is_palindrome(const char* first, const char* last)
{
    // Recursive base case
    if (first >= last)
        return 1;

    // Skip all non-alphanumeric characters
    while (first <= last && !isalnum(*first))
    {
        first++;
    }
    while (first <= last && !isalnum(*last))
    {
        last--;
    }

    // Quick bounds check
    // If the above step (skipping non-alphanumerics) resulted in first >= last, then default to yes
    if (first >= last)
        return 1;

    // Ignore letter case
    int c_first = *first;
    int c_last = *last;
    if (isalpha(c_first))
    {
        c_first = tolower(c_first);
    }
    if (isalpha(c_last))
    {
        c_last = tolower(c_last);
    }

    if (c_first == c_last)
        return is_palindrome(first + 1, last - 1);

    return 0;
}

int main(int argc, char* argv[])
{
    image_path = argv[0];

    // Quick and dirty argument check
    // This program isn't intended to be used directly by humans
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <seq> <str>\n", image_path);
        return 1;
    }

    // Parse seq argument
    // This is a number unique to this palin instance throughout its lifetime
    // It is used as identification during synchronization
    char* fail_seq = { '\0' };
    int seq = (int) strtol(argv[1], &fail_seq, 10);
    if (*fail_seq != '\0')
    {
        fprintf(stderr, "%s: invalid seq: %s\n", image_path, argv[1]);
        return 1;
    }

    // Parse str argument
    // This is the index of the string assigned to this palin instance
    char* fail_str = { '\0' };
    size_t str = strtoul(argv[2], &fail_str, 10);
    if (*fail_str != '\0')
    {
        fprintf(stderr, "%s: invalid str: %s\n", image_path, argv[2]);
        return 1;
    }

    // Get common IPC key
    key_t ipc_key = get_ipc_key();
    if (ipc_key == -1)
    {
        perrorf("%s: ftok(3) failed", image_path);
        return 1;
    }

    // Attach shared memory and obtain client bundle
    int shm_id = shmget(ipc_key, 0, 0);
    if (shm_id == -1)
    {
        perrorf("%s: shmget(2) failed", image_path);
        return 1;
    }
    client_bundle_t* bundle = shmat(shm_id, NULL, 0);
    if (bundle == (void*) -1)
    {
        perrorf("%s: shmat(2) failed", image_path);
        return 1;
    }

    // Determine if assigned string is a palindrome
    const char* string = client_bundle_get_string(bundle, str);
    int palin = is_palindrome(string, string + strlen(string) - 1);

    // FIXME: This output is just for development
    fprintf(stderr, "PALINDROME \"%s\": %s\n", string, palin ? "yes" : "no");

    // Mark this worker as ready
    bundle->worker_states[seq] = 2;

    return 0;
}
