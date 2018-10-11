#ifndef NET_THREAD_H
#define NET_THREAD_H
#include <config.h>

// Thread data structure
typedef struct {
    int socket_id;
    const config_t* conf; // Must not be modified by threads (otherwise its a race condition)
} thread_data_t;

// Starts thread-based web listening, requests get split off in their own separate threads
int thread_listen(config_t* conf);

// Request processing function for POSIX thread
void* thread_handle_request(void* thread_data);

#endif // NET_THREAD_H