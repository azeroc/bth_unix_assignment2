#ifndef CONFIG_H
#define CONFIG_H
#include <common.h>

#define CONFIG_LINE_MAX (PATH_MAX+100)

// DEFAULTS
#define CONF_DEFAULT_SOCK_BUFSIZE 8192
#define CONF_DEFAULT_REQ_BUFSIZE 8192

// Config errors
typedef enum {
    // Parsing was OK (default)
    CONFERR_OK,

    // Parsing failed because there was file error (not found, invalid location, permission problems)
    CONFERR_PARSE_FILE_ERROR,

    // Bad concurrent processing mode for requests (values other than "1" or "2" were assigned)
    CONFERR_BAD_REQUEST_MODE,

    // Bad listening port (already taken or not allowed to listen to)
    CONFERR_BAD_PORT,

    // Failed to bind a file to config_t->logfile
    CONFERR_LOG_FILE_ERROR,

    // Parsing failed because config file had bad format (incorrect values or bad formatting)
    CONFERR_PARSE_BAD_FORMAT,
} conf_error_t;

// Concurrent processing mode
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
    uint16_t port;

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

    // Request TCP read/write socket buffer size
    // Default: 8192 B / 8kB
    int sock_bufsize;

    // Request's message buffer size
    // Default: 8192 B / 8kB (Common limit among web servers)
    // If this is exceeded, server returns 400 error (Bad Request)
    int req_bufsize;

    // CONF_PARSE_OK(0) if read_conf_file was successful
    // conf_error_t > 0 if read_conf_file failed
    conf_error_t err_code;
} config_t;

// Parse configuration file ".lab3-config" and fill passed config_t object
// Run this BEFORE override_conf(...)
void read_conf_file(config_t* config, const char* filename);

// Parse program arguments and override configuration object with them
// Overrides:
// -d : config_t->as_daemon = true
// -l <logfilename> : config_t->logfile = fopen(<logfilename>, "w") [check if fopen succeded, if not - set config error]
// -
// Run this AFTER read_conf_file(...)
// Ignore irrelevant argv entries
void override_conf(config_t* config, int argc, char const *argv[]);

// Check configuration values and if they are correct:
// 1. Check if config object itself had parse errors from config file (config->parse_err > 0)
// 1. Check if root doc dir exists and is writeable
// 2. Check if logfile exists and is writeable IF it was assigned (if its NULL, then it is OK, it means we aren't using logging)
// 3. Check if port can be listened to (optional)
// Return 0 if there are no validation errors
// Return 1 if there are errors and make main function return this as exit code
// Run this AFTER override_conf(...)
int validate_conf(config_t* config);

#endif // CONFIG_H