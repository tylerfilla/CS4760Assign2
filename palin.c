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

#include "shared.h"

/**
 * Determine if an ASCII string is a palindrome.
 *
 * @param first A pointer to the first character
 * @param last A pointer to the last character (must be >= first)
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
    image_path = argv[0];

    // Quick and dirty argument check
    // This program isn't intended to be used directly by humans
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <seq> <str>\n", image_path);
        return 1;
    }

    // Parse seq argument
    char* fail_seq = { '\0' };
    size_t seq = strtoul(argv[1], &fail_seq, 10);
    if (*fail_seq != '\0')
    {
        fprintf(stderr, "invalid seq: %s\n", argv[1]);
        return 1;
    }

    // Parse str argument
    char* fail_str = { '\0' };
    size_t str = strtoul(argv[2], &fail_str, 10);
    if (*fail_str != '\0')
    {
        fprintf(stderr, "invalid str: %s\n", argv[2]);
        return 1;
    }

    // Connect to shared buffer
    const char* shared_buffer = get_shared_buffer();

    // Get total number of strings
    size_t num_strings = get_shared_num_strings(shared_buffer);

    // Validate assigned string index
    if (str >= num_strings)
    {
        fprintf(stderr, "str out of range: %ld\n", str);
        return 1;
    }

    // Get desired string
    const char* string = get_shared_string(shared_buffer, str);
    size_t string_length = strlen(string);

    printf("%s is palindrome? %d\n", string, is_palindrome(string, string + string_length - 1));

    return 0;
}
