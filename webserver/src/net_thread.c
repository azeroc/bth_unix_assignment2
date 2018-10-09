#include <net_thread.h>

// Starts thread-based web listening, requests get split off in their own separate threads
int thread_listen(config_t* conf)
{

    return 0;
}

// Request processing function for POSIX thread
void* thread_handle_request(void* thread_data)
{
    // Cast void* back to proper type
    thread_data_t* td = (thread_data_t*) thread_data;

    printf("Socket id: %d\n", td->socket_id);

    // Cleanup and exit
    free(td);
    pthread_exit(NULL);
}