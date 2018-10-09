#include <common.h>
#include <config.h>

void usage() {
    printf("Usage: webserver [options] -c <config path>\n");
    printf("Options:\n");
    printf("\t-h              : Print this usage text\n");
    printf("\t-c <config path>: Read server settings from <config path> file (Required)\n");
    printf("\t-p <port number>: Use <port number> for listening\n");
    printf("\t-d              : Run webserver as daemon (detached process)\n");
    printf("\t-l <log path>   : Write STDOUT/STDERR to <log path> file\n");
    printf("\t-s <thread|fork>: Select request handling method (default: thread)\n");
}

int main(int argc, char const *argv[])
{
    // Variable definitions
    config_t config;
    const char* conf_filename = NULL;
    int debug_mode = 0;
    int validation_result = 0;

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
    read_conf_file(&config, conf_filename);
    override_conf(&config, argc, argv);
    validation_result = validate_conf(&config);

    if (validation_result > 0) {
        usage();
        return validation_result;
    }

    return 0;
}
