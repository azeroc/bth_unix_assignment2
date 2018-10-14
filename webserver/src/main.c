#include <common.h>
#include <config.h>
#include <net_thread.h>

// Detach as daemon
// Returns child PID if you are parent/exiting process, returns 0 if you are child/daemon process, Returns -1 if forking failed
int daemon_detach() {
    pid_t pid;

    // Fork off the parent process
    pid = fork();

    if (pid < 0) {
        return -1;
    } else if (pid > 0) {
        return pid;
    }

    // Create new session and make this current process the leader of it
    if (setsid() < 0) {
        return -1;
    }

    // Close standard inputs
    close(STDIN_FILENO);  // 0
    close(STDOUT_FILENO); // 1
    close(STDOUT_FILENO); // 2

    // Reopen them to /dev/null
    // Fun situation: without this sockets will take 0, 1 and 2 and that means that printf would actually send stuff to clients
    // Normally best option is to use syslog, but we simply didn't have enough time to go for Grade B with the proper logging setup
    open("/dev/null", O_RDONLY); // 0
    open("/dev/null", O_WRONLY); // 1
    open("/dev/null", O_WRONLY); // 2

    return 0;
}

void usage() {
    printf("Usage: webserver [options]\n");
    printf("Options:\n");
    printf("\t-h              : Print this usage text\n");
    printf("\t-c <config path>: Read server settings from <config path> file\n");
    printf("\t-p <port number>: Use <port number> for listening\n");
    printf("\t-d              : Run webserver as daemon (detached process)\n");
}

int main(int argc, char const *argv[])
{
    // Variable definitions
    config_t config;
    const char* conf_filename = DEFAULT_CONF_FILE;
    int daemon_ret;

    // Required/special argument parsing
    for (int i = 0; i < argc; i++) {
        if (strlen(argv[i]) == 2 && argv[i][0] == '-') {
            // Option: -h
            if (argv[i][1] == 'h') {
                usage();
                return 0;
            }

            // Option: -c
            if (argv[i][1] == 'c') {
                if ((i+1) >= argc) {
                    usage();
                    return 0;
                } else {
                    conf_filename = argv[i+1];
                }
            }
        }
    }

    // Config parse, argv override and validation
    if (read_conf_file(&config, conf_filename) != 0) { return 1; }
    if (override_conf(&config, argc, argv) != 0) { usage(); return 1; }
    if (validate_conf(&config) != 0) { return 1;}

    // Print loaded config 
    printf("[INFO] [main] Printing loaded config object ...\n");
    print_conf(&config);

    // Daemon handling
    if (config.as_daemon == 1) {
        printf("[INFO] [main] Creating daemon process for the server...\n");

        daemon_ret = daemon_detach();
        if (daemon_ret > 0) {
            printf("[INFO] [main] Successfully created daemon instance with PID: %d\n", daemon_ret);
            return 0;
        } else if (daemon_ret == -1) {
            printf("[ERROR] [main] Failed to detach/fork as daemon process, error: %s\n", strerror(errno));
            return 1;
        }
    }

    // chroot document root directory
    if (chroot_doc_root(&config) != 0) {
        printf("[ERROR] [main] Failed to chroot doc root \"%s\", error: %s (Reminder: chroot requires root privilege, e.g. sudo)\n", config.doc_root_dir, strerror(errno));
        return 1;
    }

    // Start HTTP 1.0 web-server listening service
    return thread_listen(&config);
}
