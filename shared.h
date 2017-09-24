/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

#ifndef SHARED_H
#define SHARED_H

#include <stddef.h>

/**
 * The maximum number of concurrent worker processes.
 */
#define MAX_WORKERS 19

#define WORKER_STATE_READY 0
#define WORKER_STATE_RUNNING 1
#define WORKER_STATE_FINISHED 2

/**
 * Get common IPC key.
 */
#define get_ipc_key() ftok("/bin/echo", 'Q')

/**
 * A bundle of details shared among the master process to its palin children (client) processes. Designed to be
 * allocated in a shared memory segment (it has no pointers to heap-allocated memory) to provide a conveyor for string
 * data and synchronization information.
 */
typedef struct
{
    //
    // Workers
    //

    /** The current number of workers. */
    int num_workers;

    /** The current states of all workers. 0 = ready, 1 = running, 2 = finished. */
    int worker_states[MAX_WORKERS];

    //
    // Synchronization
    //

    /** Synchronization flags for running workers. */
    int worker_flags[MAX_WORKERS];

    /** Synchronization turn for running workers. */
    int worker_turn;

    //
    // Strings
    //

    /** The total number of strings when full. */
    size_t num_strings;

    /** The current number of strings appended. */
    size_t current_num_strings;

    /** The current strings mass. */
    size_t current_strings_mass;

    /** The size of the string data member below. */
    size_t string_data_size;

    /** The string data. Prefixed with a lookup table as specified in the README. */
    char string_data[];
} client_bundle_t;

/**
 * Calculate the size of a client bundle with the given parameters.
 *
 * @param num_strings The total number of strings
 * @param strings_mass The total tightly-packed size of the strings
 * @return [size_t] The size of the client bundle object
 */
#define sizeof_client_bundle_t(num_strings, strings_mass) (sizeof(client_bundle_t) + (num_strings) * sizeof(size_t) \
        + (strings_mass))

/**
 * Construct a client bundle instance.
 *
 * @param bundle The client bundle instance
 * @param num_strings The total number of strings
 * @param strings_mass The total tightly-packed size of the strings
 * @return The client bundle instance
 */
client_bundle_t* client_bundle_construct(client_bundle_t* bundle, size_t num_strings, size_t strings_mass);

/**
 * Destruct a client bundle instance. NOPs with NULL input.
 *
 * @param bundle The client bundle instance
 * @return The client bundle instance
 */
client_bundle_t* client_bundle_destruct(client_bundle_t* bundle);

/**
 * Append a string to a client bundle instance.
 *
 * @param bundle The bundle
 * @param string The string
 */
void client_bundle_append_string(client_bundle_t* bundle, const char* string);

/**
 * Get the string at the given index from a client bundle instance.
 *
 * @param bundle The bundle
 * @param index The index
 * @return The string
 */
const char* client_bundle_get_string(const client_bundle_t* bundle, size_t index);

#endif // #ifndef SHARED_H
