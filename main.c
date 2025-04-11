#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>
#include <stdbool.h>

#include "mib.h"

static int keepRunning = 0;

static void stopSubagent(int a)
{
    keepRunning = 0;
}

typedef struct CmdArgs {
    bool daemon;
    const char *dataFile;
    bool syslog;
} CmdArgs;

static int parseCmdArgs(int argc, char *argv[], CmdArgs *cmdArgs)
{
    int n, numArgs;

    for (n = 1, numArgs = argc -1; n <= numArgs; n++) {
        const char *arg;
        const char *val;

        arg = argv[n];

        if (strcmp(arg, "--daemon") == 0) {
            cmdArgs->daemon = true;
        } else if (strcmp(arg, "--data-file") == 0) {
            val = argv[++n];
            cmdArgs->dataFile = strdup(val);
        } else if (strcmp(arg, "--syslog") == 0) {
            cmdArgs->syslog = true;
        } else {
            fprintf(stderr, "ERROR: invalid argument \"%s\n\n", arg);
            return -1;
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    const char *snmpSubagent = "snmpSubagent";
    CmdArgs cmdArgs = { 0 };

    if (parseCmdArgs(argc, argv, &cmdArgs) != 0){
        return -1;
    }

    if (cmdArgs.syslog)
        snmp_enable_calllog();
    else
        snmp_enable_stderrlog();

    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);

    if (cmdArgs.daemon) {
        // Run in the background
        if (netsnmp_daemonize(true, !cmdArgs.syslog) != 0) {
            snmp_log(LOG_ERR, "Can't become daemon!\n");
            return -1;
        }
    }

    if (init_agent(snmpSubagent) != 0) {
        snmp_log(LOG_ERR, "Subagent initialization failed!\n");
        return -1;
    }

    mibInit(cmdArgs.dataFile);

    init_snmp(snmpSubagent);

    // Catch TERMINATE and INTERRUPR signals, used
    // to gracefully exit the snmpSubagent...
    signal(SIGTERM, stopSubagent);
    signal(SIGINT, stopSubagent);

    snmp_log(LOG_INFO, "%s running...\n", snmpSubagent);

    // Main work loop...
    keepRunning = 1;
    while (keepRunning) {
        agent_check_and_process(1);
    }

    snmp_shutdown(snmpSubagent);

    snmp_log(LOG_INFO, "%s terminated!\n", snmpSubagent);

    return 0;
}


