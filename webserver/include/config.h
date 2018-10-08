#ifndef CONFIG_H
#define CONFIG_H
#include <stdio.h>

typedef enum {
    // Invalid state (0)
    MODE_UNKNOWN,

    // Default mode
    // MODE_THREAD (1): Main thread listens for incoming connections, accepted connections are split off in separate threads for processing
    MODE_THREAD,

    // NOT YET IMPLEMENTED
    // MODE_PREFORK (2): Prematurely create X amount of subprocesses/forks for handling request load
    MODE_PREFORK,
} request_mode_t;

typedef struct {
    // Listening port for accepting client connections
    int listen_port;

    // File binding to log-file (used by Logger)
    FILE* logfile;

    // What request-handling mode to use
    request_mode_t request_mode;

    // If should be run as daemon
    bool as_daemon;

    // 0 if read_conf_file was successful
    // > 0 if read_conf_file failed
    int parse_err;
} config_t;

// Parse configuration file ".lab3-config" and return pointer to config_t object
// Return NULL and set perror if parsed incorrect config file
// Run this BEFORE override_conf(...)
config_t* read_conf_file(FILE *confFile);

// Parse program arguments and override configuration object with them
// Example: parsing "-d" in argv will set config.as_daemon to 'true'
// Run this AFTER read_conf_file(...)
void override_conf(config_t* config, char const *argv[]);

#endif // CONFIG_H