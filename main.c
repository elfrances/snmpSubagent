#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>
#include <stdbool.h>

#include "mib.h"

static bool keepRunning = true;

static void sigUsr1Handler(int a)
{
    // Need to update snmpd.conf
    snmpdConfigChange = true;
}

static void stopSubagent(int a)
{
    // Terminate the main work loop...
    keepRunning = false;
}

typedef struct CmdArgs {
    bool daemon;
    const char *dataFile;
    bool syslog;
} CmdArgs;

static const char *help =
        "SYNTAX:\n"
        "    snmpSubagent [OPTIONS]\n"
        "\n"
        "OPTIONS:\n"
        "    --daemon\n"
        "        Run in the background.\n"
        "    --data-file <path>\n"
        "        Path to the CSV file used to update the value of the\n"
        "        SUBAGENT-EXAMPLE-MIB objects.\n"
        "    --help\n"
        "        Show this help and exit.\n"
        "    --syslog\n"
        "        Use syslog for logging.\n"
        "\n";


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
        } else if (strcmp(arg, "--help") == 0) {
            printf("%s\n", help);
            exit(0);
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

    if (netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1) != SNMPERR_SUCCESS) {
        snmp_log(LOG_ERR, "Can't set NETSNMP_DS_AGENT_ROLE!\n");
        return -1;
    }

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

    // Change the default 15 sec AgentX reconnect period to
    // a faster 5 sec period ...
    if (netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_AGENTX_PING_INTERVAL, 5) != SNMPERR_SUCCESS) {
        snmp_log(LOG_WARNING, "Can't set AGENTX_PING_INTERVAL!\n");
    }

    // Catch USR1 signal, used to indicate a change
    // in the snmpd.conf file...
    signal(SIGUSR1, sigUsr1Handler);

    // Catch TERMINATE and INTERRUPR signals, used
    // to gracefully exit the snmpSubagent...
    signal(SIGTERM, stopSubagent);
    signal(SIGINT, stopSubagent);

    snmp_log(LOG_INFO, "%s running...\n", snmpSubagent);

    // Main work loop...
    while (keepRunning) {
        agent_check_and_process(1);
    }

    snmp_shutdown(snmpSubagent);

    snmp_log(LOG_INFO, "%s terminated!\n", snmpSubagent);

    return 0;
}


