/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

#ifndef PERRORF_H
#define PERRORF_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * Print a formatted string to stderr explaining the meaning of errno.
 *
 * @param format Format string
 * @param ... Format parameters
 */
static inline void perrorf(const char* format, ...)
{
    // Make sure perror(3) will actually print something
    if (!errno)
        return;

    // Format message into a buffer
    va_list args;
    va_start(args, format);
    char msg[256];
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    perror(msg);
}

#endif // #ifndef PERRORF_H
