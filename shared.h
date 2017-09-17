/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

#ifndef SHARED_H
#define SHARED_H

//
// Utility Functions
//

/**
 * The file path to the executable image of this process. This should be set once.
 */
extern char* image_path;

/**
 * Print a formatted string to stderr explaining the meaning of errno.
 *
 * @param format Format string
 * @param ... Format parameters
 */
void perrorf(const char* format, ...);

//
// Shared Buffer Functions
//

/**
 * Utility function to create the shared buffer.
 *
 * @param size The shared buffer size
 * @param [out] shm_id The shmid for the memory
 * @return The shared buffer
 */
char* create_shared_buffer(size_t size, int* shm_id);

/**
 * Utility function to destroy the shared buffer.
 *
 * @param shared_buffer The shared buffer
 * @param shm_id The shmid for the memory
 */
void destroy_shared_buffer(char* shared_buffer, int shm_id);

/**
 * Utility function to get the shared buffer for read.
 *
 * @return The shared buffer
 */
const char* get_shared_buffer();

/**
 * Get the number of strings in the shared buffer.
 *
 * @param buffer The shared buffer
 * @return The number of strings
 */
size_t get_shared_num_strings(const char* buffer);

/**
 * Set the number of strings in the shared buffer.
 *
 * @param buffer The shared buffer
 * @param num_strings The number of strings
 */
void set_shared_num_strings(char* buffer, size_t num_strings);

/**
 * Get a particular string in the shared buffer.
 *
 * @param buffer The shared buffer
 * @param index The string index
 * @return The string
 */
const char* get_shared_string(const char* buffer, size_t index);

/**
 * Set a particular string in the shared buffer.
 *
 * @param buffer The shared buffer
 * @param index The string index
 * @param count The current total number of strings
 * @param mass A pointer to the current total size of the strings area
 * @param string The string
 */
void add_shared_string(char* buffer, size_t index, size_t count, size_t* mass, const char* string);

#endif // #ifndef SHARED_H
