#include <common.h>
#include <sys/utsname.h>
#include <test.h>
#include <config.h>

int main(int argc, char const *argv[])
{
    // Variable definitions
    config_t config;
    struct utsname os_info;
    uname(&os_info); // Load OS details into os_info variable

    // Begin printing out OS info
    printf("Hello, world! OS Info:\n");
    printf("SysName:  %s\n", os_info.sysname);
    printf("NodeName: %s\n", os_info.nodename);
    printf("Release:  %s\n", os_info.release);
    printf("Version:  %s\n", os_info.version);
    printf("Machine:  %s\n", os_info.machine);

    // Test header file working
    printf("sum_for(5, 20) = %d\n", sum_for(5, 20));

    // Config parse and override
    read_conf_file(&config);
    override_conf(&config, argv);

    return 0;
}
