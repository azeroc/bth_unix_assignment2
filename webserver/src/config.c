#include <config.h>

// Parse configuration file ".lab3-config" and return pointer to config_t object
// Return NULL and set perror if parsed incorrect config file
// Run this BEFORE override_conf(...)
void read_conf_file(config_t* config, const char* filename)
{
    // Declaration
    FILE* filePtr;
    char config_line [CONFIG_LINE_MAX];

    // Config default values
    config->port = 80;
    strcpy(config->doc_root_dir, "../../www");
    config->logfile = NULL;
    config->request_mode = MODE_THREAD;
    config->as_daemon = 0;
    config->sock_bufsize = CONF_DEFAULT_SOCK_BUFSIZE;
    config->req_bufsize = CONF_DEFAULT_REQ_BUFSIZE;
    config->err_code = CONFERR_OK;

    printf("debug => read_conf_file::filePtr = fopen(\"%s\", \"r\")\n", filename);
    filePtr = fopen(filename, "r");
    printf("debug => config contents:\n");

    if (filePtr != NULL)
    {
        while(fgets(config_line, CONFIG_LINE_MAX, filePtr) != NULL)
        {
            // Implementing config line parsing logic here ...

            printf("\t%s", config_line); // Debug
        }

        printf("\n"); // Debug
        fclose (filePtr);
    } else {
        config->err_code = CONFERR_PARSE_FILE_ERROR;
    }
}

// Parse program arguments and override configuration object with them
// Example: parsing "-d" in argv will set config.as_daemon to 'true'
// Run this AFTER read_conf_file(...)
void override_conf(config_t* config, int argc, char const *argv[])
{
    // Implement override logic here ...

    // Debug: print final config_t object
    printf("debug => config object:\n");
    printf("\tport: %d\n", config->port);
    printf("\tdoc_root_dir: %s\n", config->doc_root_dir);
    printf("\tlogfile: %p\n", config->logfile);
    printf("\trequest_mode: %d\n", config->request_mode);
    printf("\tas_daemon: %d\n", config->as_daemon);
    printf("\tparse_err: %d\n", config->err_code);
}

// Check configuration values and if they are correct
int validate_conf(config_t* config)
{
    // Parsing error check
    if (config->err_code != CONFERR_OK) {
        // TODO: Logger
        printf("ERROR: Config validation failed, config->err_code: %d\n", config->err_code);
        return 1;
    }

    // ... rest of validation here ...
    return 0;
}