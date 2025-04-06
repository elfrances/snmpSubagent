#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>

#include "mib.h"

static int keepRunning = 0;

static void stopSubagent(int a)
{
    keepRunning = 0;
}


int main(int argc, char *argv[])
{
    const char *snmpSubagent = "snmpSubagent";

    snmp_enable_stderrlog();

    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);

    init_agent(snmpSubagent);

    mibInit();

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


