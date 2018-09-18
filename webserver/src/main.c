#include <stdio.h>
#include <sys/utsname.h>

int main(int argc, char const *argv[])
{
    // Define utsname variable
    struct utsname os_info;
    uname(&os_info); // Load OS details into os_info variable

    // Begin printing out OS info
    printf("Hello, world! OS Info:\n");
    printf("SysName:  %s\n", os_info.sysname);
    printf("NodeName: %s\n", os_info.nodename);
    printf("Release:  %s\n", os_info.release);
    printf("Version:  %s\n", os_info.version);
    printf("Machine:  %s\n", os_info.machine);

    return 0;
}
