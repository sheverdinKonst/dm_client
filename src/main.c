#include "run_CLIcmd.h"
#include "state_handler.h"
#include "settings_module.h"

#define VERSION ("2.1.35")

int main(int argc, char **argv)
{
    openlog("dm_info", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "====>>> T_FORTIS DEVICE MANAGER <<<<==== Version %s \n", VERSION);
    closelog();

    if (argc >=2)
    {
        for(int optid=1; optid < argc; optid++)
        {
            if (argv[optid][0] == '-')
            {
                switch (argv[optid][1])
                {
                    case 'h':
                    {
                        printf("help\n");
                        printf("v - Print Version\n");
                        printf("d - Print Debug Info\n");
                    }
                    break;

                    case 'v':
                    {
                        printf("Version %s\n", VERSION);
                    }
                    break;

                    case 'd':
                    {
                        printf("debug on\n");

                        setMainTestFlag(1);
                    }
                    break;

                    case 'l':
                    {
                        int param = 0;
                        printf("location on\n");
                        param = atoi(argv[optid+1]);
                        setLocationTestFlag(param);
                    }
                    break;
                }
            }
        }
    }

    client_app();
    return 0;
}

