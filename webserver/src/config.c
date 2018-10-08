#include <config.h>

// Parse configuration file ".lab3-config" and return pointer to config_t object
// Return NULL and set perror if parsed incorrect config file
// Run this BEFORE override_conf(...)
void read_conf_file(config_t* config)
{
    // Declaration
    FILE* fptr;
    char config_line [CONFIG_LINE_MAX];

    // Config default values
    config->port = 80;
    strcpy(config->doc_root_dir, "../../www");
    config->logfile = NULL;
    config->request_mode = MODE_THREAD;
    config->as_daemon = 0;
    config->parse_err = 0;

    fptr = fopen(CONF_FILENAME, "r");
    if (fptr != NULL)
    {
        while(fgets(config_line, CONFIG_LINE_MAX, fptr) != NULL)
        {
            // Implementing config line parsing logic here ...

            printf("%s", config_line); // Debug
        }

        printf("\n"); // Debug
        fclose (fptr);
    }
}

// Parse program arguments and override configuration object with them
// Example: parsing "-d" in argv will set config.as_daemon to 'true'
// Run this AFTER read_conf_file(...)
void override_conf(config_t* config, char const *argv[])
{
    // Implement override logic here ...

    // Debug: print final config_t object
    printf("config object:\n");
    printf("\tport: %d\n", config->port);
    printf("\tdoc_root_dir: %s\n", config->doc_root_dir);
    printf("\tlogfile: %p\n", config->logfile);
    printf("\trequest_mode: %d\n", config->request_mode);
    printf("\tas_daemon: %d\n", config->as_daemon);
    printf("\tparse_err: %d\n", config->parse_err);
}