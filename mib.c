#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <pthread.h>

#include "mib.h"

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

// This task is used to update the values of the MIB objects; e.g.
// as when the value is obtained from reading an environmental
// sensor.
static void *mibUpdateTask(void *arg)
{
    const struct timespec pollPeriod = { .tv_sec = 1, .tv_nsec = 0 };

    while (1) {
        // Go do the work...
        snmp_log(LOG_INFO, "%s: Updating MIB data...\n", __func__);

        // Sleep until the next poll period
        nanosleep(&pollPeriod, NULL);
    }

    return NULL;
}

int mibInit(void)
{
    pthread_t thread;

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

    // Start the MIB update task
    if (pthread_create(&thread, NULL, mibUpdateTask, NULL)) {
        snmp_log(LOG_ERR, "Failed to create MIB update task!\n");
        return -1;
    }

    return 0;
}
