/*
 * CS 4760
 * Assignment 1
 * Tyler Filla
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <string.h>

#include "shared.h"

client_bundle_t* client_bundle_construct(client_bundle_t* self, size_t num_strings, size_t strings_mass)
{
    self->num_strings = num_strings;
    self->current_num_strings = 0;
    self->string_data_size = num_strings * sizeof(size_t) + strings_mass;
    memset(self->string_data, 0, self->string_data_size);
    return self;
}

client_bundle_t* client_bundle_destruct(client_bundle_t* self)
{
    return self;
}

void client_bundle_append_string(client_bundle_t* self, const char* string)
{
    size_t string_len = strlen(string);

    // The destination for the shared string
    char* string_dest = self->string_data + self->num_strings * sizeof(size_t) + self->current_strings_mass;
    size_t string_offset = string_dest - self->string_data;

    // Pack string into strings area of string data buffer
    memcpy(string_dest, string, string_len);
    string_dest[string_len] = '\0';
    self->current_strings_mass += string_len + 1;

    // Add string offset to lookup table
    memcpy(self->string_data + self->current_num_strings * sizeof(size_t), &string_offset, sizeof(size_t));
}

const char* client_bundle_get_string(const client_bundle_t* self, size_t index)
{
    // Use lookup table to find the desired string's offset and get the string
    size_t string_offset;
    memcpy(&string_offset, self->string_data + index * sizeof(size_t), sizeof(size_t));
    return &self->string_data[string_offset];
}

/*
const char* get_shared_buffer()
{
    // Create matching IPC key for shared memory
    key_t ipc_key = get_ipc_key();
    if (ipc_key == -1)
    {
        perrorf("%s: ftok(3) failed", "idk");
        exit(1);
    }

    // Attach shared buffer as read-only memory
    // This does not create the shared buffer
    int shm_id = shmget(ipc_key, 0, 0);
    if (shm_id == -1)
    {
        perrorf("%s: shmget(2) failed", "idk");
        exit(1);
    }
    const char* shared_buffer = shmat(shm_id, NULL, SHM_RDONLY);
    if (shared_buffer == (void*) -1)
    {
        perrorf("%s: shmat(2) failed", "idk");
        exit(1);
    }

    return shared_buffer;
}
*/
