#ifndef CONFIG_H
#define CONFIG_H
#include <common.h>

#define DEFAULT_CONF_FILE ".lab3-config"
#define CONFIG_LINE_MAX (PATH_MAX+100)

// Socket / Request buffer sizes
#define CONF_SOCK_BUFSIZE 8192
#define CONF_REQ_BUFSIZE 8192

typedef struct {
    // Listening port for accepting client connections
    uint16_t port;

    // The path to "www" directory of webserver
    char doc_root_dir[PATH_MAX];

    // 0: run webserver normally
    // 1: run webserver as daemon
    int as_daemon;
} config_t;

// Parse configuration file ".lab3-config" and fill passed config_t object
// Run this BEFORE override_conf(...)
int read_conf_file(config_t* config, const char* filename);

// Parse program arguments and override configuration object with them
// Overrides:
// -d : config_t->as_daemon = true
// -p <port> : config_t->port = atoi(<port>)
// Run this AFTER read_conf_file(...)
// Ignore irrelevant argv entries
// Return 0 if argument were OK, 1 if not
int override_conf(config_t* config, int argc, char const *argv[]);

// Check configuration values and if they are correct:
// 1. Check if config object itself had parse errors from config file (config->parse_err > 0)
// 1. Check if root doc dir exists and is writeable
// 2. Check if logfile exists and is writeable IF it was assigned (if its NULL, then it is OK, it means we aren't using logging)
// 3. Check if port can be listened to (optional)
// Return 0 if there are no validation errors
// Return 1 if there are errors and make main function return this as exit code
// Run this AFTER override_conf(...)
int validate_conf(config_t* config);

// Imprison webserver in its document root directory (requires root access)
// 0 if successful, 1 if not
int chroot_doc_root(config_t* config);

// Print given config object
void print_conf(config_t* config);

#endif // CONFIG_H