#ifndef CONFIG_H
#define CONFIG_H
#include <common.h>

#define CONF_FILENAME ".lab3-config"
#define CONFIG_LINE_MAX (PATH_MAX+100)

typedef enum {
    // Invalid state (0)
    MODE_UNKNOWN,

    // Default mode
    // MODE_THREAD (1): Main thread listens for incoming connections, accepted connections are split off in separate threads for processing
    MODE_THREAD,

    // NOT YET IMPLEMENTED
    // MODE_FORK (2): Main process creates/forks child processes for incoming connection handling
    MODE_FORK,
} request_mode_t;

typedef struct {
    // Listening port for accepting client connections
    int port;

    // The path to "www" directory of webserver
    char doc_root_dir[PATH_MAX];

    // File binding to log-file (used by Logger)
    // Default: NULL
    FILE* logfile;

    // What request-handling mode to use
    request_mode_t request_mode;

    // 0: run webserver normally
    // 1: run webserver as daemon
    int as_daemon;

    // 0 if read_conf_file was successful
    // > 0 if read_conf_file failed
    int parse_err;
} config_t;

// Parse configuration file ".lab3-config" and fill passed config_t object
// Run this BEFORE override_conf(...)
void read_conf_file(config_t* config);

// Parse program arguments and override configuration object with them
// Example: parsing "-d" in argv will set config.as_daemon to 'true'
// Run this AFTER read_conf_file(...)
void override_conf(config_t* config, char const *argv[]);

#endif // CONFIG_H