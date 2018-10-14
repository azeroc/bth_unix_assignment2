#include <config.h>

// Helper "switch" like function to correctly map values based on keys to config_t object
// Skip unknown key-value pairs
// Returns 0 on successful translation, 1 on type errors
int confparse_key_value(config_t* config, const char* key, const char* val)
{
    if (strcmp(key, "port") == 0) {
        config->port = (uint16_t) atoi(val);

        if (config->port == 0) {
            printf("[ERROR] [confparse_key_value] Config-parsing couldn't parse \"port\" key to valid port uint16_t\n");
            return 1;
        }
    } else if (strcmp(key, "as_daemon") == 0) {
        if (strcmp(val, "true") == 0 || strcmp(val, "1") == 0) {
            config->as_daemon = 1;
        } else if (strcmp(val, "false") == 0 || strcmp(val, "0") == 0) {
            config->as_daemon = 0;
        } else {
            printf("[ERROR] [confparse_key_value] Config-parsing couldn't parse \"as_daemon\" key to valid flag (allowed values: true, false, 1, 0)\n");
            return 1;
        }
    } else if (strcmp(key, "doc_root_dir") == 0) {
        strncpy(config->doc_root_dir, val, PATH_MAX);
    }

    return 0;
}

// Parse configuration file ".lab3-config" and return pointer to config_t object
// Return NULL and set perror if parsed incorrect config file
// Run this BEFORE override_conf(...)
// Returns 0 on successful read, 1 on read failure
int read_conf_file(config_t* config, const char* filename)
{
    // Declaration
    FILE* filePtr;
    char config_line [CONFIG_LINE_MAX];
    char key[100];
    char value[PATH_MAX];
    char scan_fmt[100];
    char* c_ptr;

    // Setup scan_fmt for config line parsing
    // Usually should be "%99[^= ] = %4095s"
    // Notes: simple format such as "%s = %s" can't work, because key-values could be squished together as "port=80" (no spaces),
    //        so there is need to separate key and value by '=' in scan_fmt, hence "%[^= ]" format
    sprintf(scan_fmt, "%%%d[^= ] = %%%ds", 99, PATH_MAX);

    // Config default values
    config->port = 80;
    strcpy(config->doc_root_dir, "../../www");
    config->as_daemon = 0;

    // Begin parsing from config file
    filePtr = fopen(filename, "r");
    if (filePtr != NULL)
    {
        while(fgets(config_line, CONFIG_LINE_MAX, filePtr) != NULL)
        {
            // Skip past blank lines as well as "comments"
            c_ptr = config_line; // Reset to start of line
            while (*c_ptr == ' ' || *c_ptr == '\r' || *c_ptr == '\n' || *c_ptr == '\t' || *c_ptr == '\f' || *c_ptr == '\v') { c_ptr++; }
            if (*c_ptr == '\0' || *c_ptr == '#') { continue; }

            // Implementing config line parsing logic here ...
            if (sscanf(config_line, scan_fmt, key, value) < 2) {
                printf("[ERROR] [read_conf_file] Couldn't scan-parse following line: \"%s\"\n", config_line);
                return 1;
            }

            if (confparse_key_value(config, key, value) != 0) { return 1; }
        }

        fclose (filePtr);
    } else {
        printf("[ERROR] Couldn't open config file \"%s\", error: %s\n", filename, strerror(errno));
        return 1;
    }

    return 0;
}

// Parse program arguments and override configuration object with them
// Example: parsing "-d" in argv will set config.as_daemon to 'true'
// Run this AFTER read_conf_file(...)
// Returns 0 if there were no invalid arguments, 1 if there are (in which case usage should be returned)
int override_conf(config_t* config, int argc, char const *argv[])
{
    const char* arg;
    const char* argn;
    for (int i = 0; i < argc; i++) {
        arg = argv[i];
        if (i+1 < argc) { argn = argv[i+1]; } else { argn = NULL; }

        if (arg[0] == '-') {
            if (arg[1] == 'p' && arg[2] == '\0') { // Port override
                    if (argn) {
                        if ( (config->port = (uint16_t)atoi(argn)) == 0 ) {
                            printf("[ERROR] [override_conf] Couldn't parse port parameter into usable port\n");
                            return 1;
                        }
                    } else {
                        printf("[ERROR] [override_conf] Couldn't find port parameter after '-p' option\n");
                        return 1;
                    }
            } else if (arg[1] == 'd' && arg[2] == '\0') {
                config->as_daemon = 1;
            }
        }
    }
    
    return 0;
}

// Print given config object
void print_conf(config_t* config) {
    printf("Loaded config:\n");
    printf("\tport: %d\n", config->port);
    printf("\tdoc_root_dir: %s\n", config->doc_root_dir);
    printf("\tas_daemon: %d\n", config->as_daemon);
}

// Check configuration values and if they are correct
int validate_conf(config_t* config)
{
    char index_file_path[PATH_MAX];
    int access_result;
    char resolved_path[PATH_MAX];
    const char* doc_root = config->doc_root_dir;

    // Transform doc_root_dir to realpath
    if (realpath(doc_root, resolved_path) == NULL) {
        printf("[ERROR] [validate_conf] Failed to get realpath of \"%s\", error: %s\n", doc_root, strerror(errno));
        return 1;
    }
    strncpy(config->doc_root_dir, resolved_path, PATH_MAX); // Replace doc_root with the resolved path

    // Check for existence and read permission in the directory
    access_result = access(doc_root, F_OK | R_OK);
    if (access_result == 0) {
        printf("[INFO] [validate_conf] Checks for document root directory \"%s\" - OK (Exists and is readable)\n", doc_root);
    } else {
        printf("[ERROR] [validate_conf] Checks failed for document root directory \"%s\", error: %s\n", doc_root, strerror(errno));
        return 1;
    }

    // Check for index.html file
    sprintf(index_file_path, "%s/index.html", doc_root); // <doc root path>/index.html
    access_result = access(index_file_path, F_OK | R_OK);
    if (access_result == 0) {
        printf("[INFO] [validate_conf] Checks for index.html file at path \"%s\" - OK (Exists and is readable)\n", index_file_path);
    } else {
        printf("[ERROR] [validate_conf] Checks failed for index.html file at path \"%s\", error: %s\n", index_file_path, strerror(errno));
        return 1;
    }    

    return 0;
}

// Imprison webserver in its document root directory (requires root access)
// 0 if successful, 1 if not (check errno)
int chroot_doc_root(config_t* config)
{
    // Attempt to CHROOT it
    if (chroot(config->doc_root_dir) != 0) {
        return 1;
    }

    printf("[INFO] [chroot_doc_root] Document root \"%s\" successfully chroot-ed\n", config->doc_root_dir);
    return 0;
}