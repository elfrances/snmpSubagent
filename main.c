#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>

// SUBAGENT-EXAMPLE-MIB Object Handlers

// myReadOnlyInteger OBJECT-TYPE
//     SYNTAX      Integer32 (0..2147483647)
//     MAX-ACCESS  read-only
//     STATUS      current
//     DESCRIPTION "A read-only integer value"
//     ::= { myScalars 1 }
static oid myReadOnlyIntegerOid[] = { 1, 3, 6, 1, 3, 9999, 1, 1, 0 };
static int myReadOnlyInteger = 3662;
static int myReadOnlyIntegerCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    snmp_log(LOG_INFO, "myReadOnlyIntegerCb: \n");
    return 0;
}

// myReadWriteInteger OBJECT-TYPE
//     SYNTAX      Integer32 (0..2147483647)
//     MAX-ACCESS  read-write
//     STATUS      current
//     DESCRIPTION "A read-write integer value"
//     DEFVAL       { 0 }
//     ::= { myScalars 2 }
static oid myReadWriteIntegerOid[] = { 1, 3, 6, 1, 3, 9999, 1, 2, 0 };
static long myReadWriteInteger = 0;
static int myReadWriteIntegerCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    snmp_log(LOG_INFO, "myReadWriteIntegerCb: \n");
    return 0;
}

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

    // Register myReadOnlyInteger handler
    if (netsnmp_register_read_only_int_instance("myReadOnlyInteger",
                                                myReadOnlyIntegerOid, OID_LENGTH(myReadOnlyIntegerOid),
                                                &myReadOnlyInteger, myReadOnlyIntegerCb) != 0) {
        snmp_log(LOG_ERR, "Failed to register myReadOnlyInteger!\n");
        return -1;
    }

    // Register myReadWriteInteger handler
    if (netsnmp_register_long_instance("myReadWriteInteger",
                                       myReadWriteIntegerOid, OID_LENGTH(myReadWriteIntegerOid),
                                       &myReadWriteInteger, myReadWriteIntegerCb) != 0) {
        snmp_log(LOG_ERR, "Failed to register myReadWriteInteger!\n");
        return -1;
    }

    init_snmp(snmpSubagent);

    signal(SIGTERM, stopSubagent);
    signal(SIGINT, stopSubagent);

    keepRunning = 1;

    snmp_log(LOG_INFO, "%s running...\n", snmpSubagent);

    while (keepRunning) {
        agent_check_and_process(1);
    }

    snmp_shutdown(snmpSubagent);

    snmp_log(LOG_INFO, "%s terminated!\n", snmpSubagent);

    return 0;
}


