/*
 *  based on:
 *  ssss version 0.5  -  Copyright 2005,2006 B. Poettering
 *
 *  refactored by Christopher Hall 2017-01-18
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

#if !defined(_SSSS_H_)
#define _SSSS_H_ 1

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define VERSION "0.5"

#define RANDOM_SOURCE "/dev/random"

#define MAXDEGREE 1024
#define MAXTOKENLEN 128
#define MAXLINELEN (MAXTOKENLEN + 1 + 10 + 1 + MAXDEGREE / 4 + 10)

// errors
typedef enum {
    ERROR_OK = 0,
    ERROR_INVALID_SECURITY_LEVEL,  // possibly secret is too long
    ERROR_INPUT_IS_NULL,
    ERROR_INPUT_STRING_TOO_SHORT,
    ERROR_INPUT_STRING_TOO_LONG,
    ERROR_BUFFER_TOO_SMALL,
    ERROR_BINARY_DATA,             // non ASCII bytes detected
    ERROR_INVALID_SYNTAX,          // i.e. not prefix-number-share
    ERROR_INVALID_SHARE,           // could be duplicate
    ERROR_CANNOT_OPEN_RANDOM,      // problem with RANDOM_SOURCE
    ERROR_CANNOT_CLOSE_RANDOM,     // ..
    ERROR_CANNOT_READ_RANDOM,      // ..
    ERROR_SECURITY_LEVEL_TOO_SMALL_FOR_DIFFUSION,
    ERROR_SHARE_HAS_ILLEGAL_LENGTH,
    ERROR_SHARES_HAVE_DIFFERENT_SECURITY_LEVELS,  // ie different bit counts
    ERROR_SHARES_INCONSISTENT,     // possibly a single share was used twice
    ERROR_MALLOC_FAILED,
    
    // no errors after here
    ERROR_maximum
} error_t;

// API
// ===

// external random number

typedef void *random_open_t(void *data);                                 // data is cprng_t.argument
typedef int random_close_t(void *data);                                  // data is return value from open
typedef ssize_t random_read_t(void *data, void *buffer, size_t nbytes);  // data is return value from open

// for split cprng parameter
typedef struct {
    random_open_t *open;
    random_close_t *close;
    random_read_t *read;
    void *argument; // passed to open call
} cprng_t;


// callback for split
typedef error_t process_share_t(void* data,          // for passing file handle etc
const char *buffer,  // the share as a string
size_t length,       // characters in buffer
int number,          // share number 1..N
int total);          // total shares

// split a secret from buffer into shares
// each share is passed to callback (with extra data item e.g. file handle)
error_t split(const char *secret,                // hex or ASCII secret to split
              process_share_t *process_share,    // called for each share to be saved
              void *data,                        // just passed to callback
              int security,                      // bits or zero for auto, e.g. 512 => 64 bytes
              int threshold,                     // shares to reconstruct secret
              int number,                        // total shares
              bool diffusion,                    // ? extra eccoding
              const char *prefix,                // for output like: prefix-N-share
              bool hexmode,                      // false => ASCII
              const cprng_t *cprng               // NULL => internal RANDOM_SOURCE
);


// callback for combine
typedef const char *read_share_t(void* data,     // for passing file handle etc
int number,     // share number 1..N
int threshold,  // total shares
size_t size);   // maximum bytes

// fetch shares and combine back to get secret
error_t combine(char *secret,                    // the reconstituted secret
                size_t secret_size,              // size of secret, must include space for '\0'
                read_share_t *get_share,         // fetch a share string
                void *data,                      // just passed to callback
                int threshold,                   // shares to reconstruct secret
                bool diffusion,                  // ?
                bool hexmode);                   // false => ASCII


// wrapper API
// ===========

error_t wrapped_split(char **shares,             // returns pointer to allocated data (caller must free after use)
                      const char *secret,        // hex or ASCII secret to split
                      int security,              // bits or zero for auto, e.g. 512 => 64 bytes
                      int threshold,             // shares to reconstruct secret
                      int number,                // total shares
                      bool diffusion,            // ? extra eccoding
                      const char *prefix,        // for output like: prefix-N-share
                      bool hexmode,              // false => ASCII
                      const char *random_bytes,  // NULL => internal RANDOM_SOURCE, otherwise array of random data
                      size_t byte_count);        // ... must be: threshold * MAX_DEGREE/8 bytes long

char **wrapped_allocate_shares(int number);      // allocate a sutable array for wrapped_split

error_t wrapped_free_shares(char **shares,       // to clear and release share allocation
                            int number);         // must be original allocation size

error_t wrapped_combine(char *secret,            // the reconstituted secret
                        size_t secret_size,      // size of secret, must include space for '\0'
                        const char **shares,     // must contain threshold * '\0' terminated entries
                        int threshold,           // shares to reconstruct secret
                        bool diffusion,          // ?
                        bool hexmode);           // false => ASCII


// for use by main routine (not really for export)
// ===============================================

int field_size_valid(int deg);


#endif
