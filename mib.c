#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_trap.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mib.h"

// SUBAGENT-EXAMPLE-MIB Object Handlers

// ac1Temp OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-only
//     STATUS      current
//     DESCRIPTION "The current value (in degrees Celsius) of the A/C
//                  Unit #1 temperature sensor."
//     ::= { subagentExampleMIB 1 }
static const oid ac1TempOid[] = { 1, 3, 6, 1, 3, 9999, 1, 0 };
static int ac1Temp = 0;
static int ac1TempCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    snmp_log(LOG_INFO, "%s: \n", __func__);
    return SNMP_ERR_NOERROR;
}

// ac2Temp OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-only
//     STATUS      current
//     DESCRIPTION "The current value (in degrees Celsius) of the A/C
//                  Unit #2 temperature sensor."
//     ::= { subagentExampleMIB 2 }
static const oid ac2TempOid[] = { 1, 3, 6, 1, 3, 9999, 2, 0 };
static int ac2Temp = 0;
static int ac2TempCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    snmp_log(LOG_INFO, "%s: \n", __func__);
    return SNMP_ERR_NOERROR;
}

// ac3Temp OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-only
//     STATUS      current
//     DESCRIPTION "The current value (in degrees Celsius) of the A/C
//                  Unit #3 temperature sensor."
//     ::= { subagentExampleMIB 3 }
static const oid ac3TempOid[] = { 1, 3, 6, 1, 3, 9999, 3, 0 };
static int ac3Temp = 0;
static int ac3TempCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    snmp_log(LOG_INFO, "%s: \n", __func__);
    return SNMP_ERR_NOERROR;
}

// hiTempThreshold OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-write
//     STATUS      current
//     DESCRIPTION "The temperature value (in degrees Celsius) above
//                  which to raise an A/C Unit High Temperature alarm."
//     DEFVAL      { 30 }
//     ::= { subagentExampleMIB 4 }
static const oid hiTempThresholdOid[] = { 1, 3, 6, 1, 3, 9999, 4, 0 };
static int hiTempThreshold = 30;
static int hiTempThresholdCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    snmp_log(LOG_INFO, "%s: \n", __func__);
    return SNMP_ERR_NOERROR;
}

// acHiTempUnit OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  accessible-for-notify
//     STATUS      current
//     DESCRIPTION "The A/C Unit thar raised a High Temperature alarm."
//     ::= { subagentExampleMIB 5 }
static const oid acHiTempUnitOid[] = { 1, 3, 6, 1, 3, 9999, 5 };

// acHiTempAlarm NOTIFICATION-TYPE
//     OBJECTS     { acHiTempUnit }
//     STATUS      current
//     DESCRIPTION "Trap to notify a High Temperature alarm on one of
//                  the A/C Units."
//     ::= { subagentExampleMIB 6 }
static const oid acHiTempAlarmOid[] = { 1, 3, 6, 1, 3, 9999, 6 };

static int sendHiTempAlarmTrap(const char *varOid)
{
    netsnmp_variable_list *varList = NULL;
    const oid snmpTrapOid[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };
    int acUnit = 0;

    if (sscanf(varOid, "ac%dTemp", &acUnit) != 1) {
        snmp_log(LOG_ERR, "%s: Invalid A/C unit: %s\n", __func__, varOid);
        return -1;
    }

    // Add snmpTrapOID = acHiTempAlarm
    snmp_varlist_add_variable(&varList,
            snmpTrapOid, OID_LENGTH(snmpTrapOid),
            ASN_OBJECT_ID,
            acHiTempAlarmOid, OID_LENGTH(acHiTempAlarmOid) * sizeof (oid));

    // Add acHiTempUnit = acUnit
    snmp_varlist_add_variable(&varList,
            acHiTempUnitOid, OID_LENGTH(acHiTempUnitOid),
            ASN_INTEGER,
            &acUnit, sizeof (acUnit));

    send_v2trap(varList);

    snmp_free_varbind(varList );

    return 0;
}

bool snmpdConfigChange = false;

static int setConfigValue(const char *tag, const char *val, FILE *wrFp)
{
    if (strcmp(tag, "agentAddress") == 0) {
        // TODO: validate the syntax of the value string
        fprintf(wrFp, "agentaddress %s", val);
    } else if (strcmp(tag, "readOnlyCommunity") == 0) {
        fprintf(wrFp, "rocommunity %s", val);
    } else if (strcmp(tag, "readWriteCommunity") == 0) {
        fprintf(wrFp, "rwcommunity %s", val);
    } else if (strcmp(tag, "trapReceiver") == 0) {
        // TODO: validate the syntax of the value string
        fprintf(wrFp, "trap2sink %s", val);
    } else if (strcmp(tag, "sysContact") == 0) {
        fprintf(wrFp, "sysContact %s", val);
    } else if (strcmp(tag, "sysLocation") == 0) {
        fprintf(wrFp, "sysLocation %s", val);
    } else {
        snmp_log(LOG_WARNING, "%s: unsupported config tag \"%s\"\n", __func__, tag);
    }

    return 0;
}

static int procConfigFile(const char *configFile)
{
    FILE *rdFp;
    FILE *wrFp;
    char strBuf[256];

    snmpdConfigChange = false;

    // Open the configFile in read-only mode
    if ((rdFp = fopen(configFile, "r")) == NULL) {
        snmp_log(LOG_ERR, "%s: failed to open config file \"%s\"\n", __func__, configFile);
        return -1;
    }

    // Open the snmpd.conf file in read-write mode
    if ((wrFp = fopen("/etc/snmp/snmpd.conf", "w+")) == NULL) {
        snmp_log(LOG_ERR, "%s: failed to open snmpd.conf file\n", __func__);
        return -1;
    }

    // Add an informational line
    fprintf(wrFp, "# This file was autogenerated by snmpSubagent.\n");
    // AgentX support is always enabled
    fprintf(wrFp, "master agentx\n");

    // Read one line at a time. Lines that start
    // with a '#' are comments and are skipped.
    while (fgets(strBuf, sizeof (strBuf), rdFp) != NULL) {
        if ((strBuf[0] != '#') && (strBuf[0] != '\0')) {
            char *comma = strchr(strBuf, ',');
            if (comma != NULL) {
                *comma = '\0';  // null-terminate tag string
                setConfigValue(strBuf, (comma + 1), wrFp);
            }
        }
    }

    // Done with snmpd.conf!
    fclose(wrFp);

    // Done with the data file!
    fclose(rdFp);

    snmp_log(LOG_INFO, "%s: Restarting snmpd service to pick up the new config...\n", __func__);

    // Now restart the "snmpd" service to pick up the
    // config changes.
    if (system("systemctl restart snmpd") == -1) {
        int errNo = errno;
        snmp_log(LOG_ERR, "%s: Failed to exec \"systemctl restart snmpd\": %s (%d)\n", __func__, strerror(errNo), errNo);
        return -1;
    }

    return 0;
}

static int setOidValue(const char *oid, int value)
{
    int *var = NULL;
    bool acTemp = false;

    if (strcmp(oid, "ac1Temp") == 0) {
        var = &ac1Temp;
        acTemp = true;
    } else if (strcmp(oid, "ac2Temp") == 0) {
        var = &ac2Temp;
        acTemp = true;
    } else if (strcmp(oid, "ac3Temp") == 0) {
        var = &ac3Temp;
        acTemp = true;
    } else {
        return -1;
    }

    if ((var != NULL) && (value != *var)) {
        snmp_log(LOG_INFO, "%s: oid=%s oldValue=%d newValue=%d\n", __func__, oid, *var, value);

        // Update the corresponding MIB variable
        *var = value;

        // If this is an ac1Temp-ac3Temp object, do we
        // need to send a hiTempAlarm trap?
        if (acTemp && (value > hiTempThreshold)) {
            snmp_log(LOG_INFO, "%s: oid=%s newValue=%d is greater than hiTempThreshold=%d !\n", __func__, oid, value, hiTempThreshold);
            sendHiTempAlarmTrap(oid);
        }
    }

    return 0;
}

// Read the latest MIB object values from the data file
static int procDataFile(const char *dataFile)
{
    FILE *fp;
    char strBuf[256];

    // Open the dataFile in read-only mode
    if ((fp = fopen(dataFile, "r")) == NULL) {
        snmp_log(LOG_WARNING, "%s: failed to open data file \"%s\"\n", __func__, dataFile);
        return -1;
    }

    // Read one line at a time. Lines that start
    // with a '#' are comments and are skipped.
    while (fgets(strBuf, sizeof (strBuf), fp) != NULL) {
        if ((strBuf[0] != '#') && (strBuf[0] != '\0')) {
            char *comma = strchr(strBuf, ',');
            if (comma != NULL) {
                int value;
                *comma = '\0';
                if (sscanf((comma + 1), "%d", &value) == 1) {
                    setOidValue(strBuf, value);
                }
            }
        }
    }

    // Done with the dataFile!
    fclose(fp);

    return 0;
}

// This task is used to update the values of the MIB objects; e.g.
// as when the value is obtained from reading an environmental
// sensor.
static void *mibUpdateTask(void *arg)
{
    const CmdArgs *cmdArgs = arg;
    const struct timespec pollPeriod = { .tv_sec = 1, .tv_nsec = 0 };

    while (true) {
        struct timeval now;
        struct tm brkDwnTime;
        static char tsBuf[32];  // YYYY-MM-DDTHH:MM:SS

        gettimeofday(&now, NULL);
        strftime(tsBuf, sizeof (tsBuf), "%Y-%m-%d %H:%M:%S", gmtime_r(&now.tv_sec, &brkDwnTime));    // %H means 24-hour time

        snmp_log(LOG_INFO, "%s: Updating MIB data from %s at %s ...\n", __func__, cmdArgs->dataFile, tsBuf);

        // Process the data file
        procDataFile(cmdArgs->dataFile);

        // Was there a config change?
        if (snmpdConfigChange) {
            procConfigFile(cmdArgs->configFile);
        }

        // Sleep until the next poll period
        nanosleep(&pollPeriod, NULL);
    }

    return NULL;
}

int mibInit(const CmdArgs *cmdArgs)
{
    pthread_t thread;

    // Register ac1Temp handler
    if (netsnmp_register_read_only_int_instance("ac1Temp",
                                                ac1TempOid, OID_LENGTH(ac1TempOid),
                                                &ac1Temp, ac1TempCb) != 0) {
        snmp_log(LOG_ERR, "Failed to register ac1Temp!\n");
        return -1;
    }

    // Register ac2Temp handler
    if (netsnmp_register_read_only_int_instance("ac2Temp",
                                                ac2TempOid, OID_LENGTH(ac2TempOid),
                                                &ac2Temp, ac2TempCb) != 0) {
        snmp_log(LOG_ERR, "Failed to register ac2Temp!\n");
        return -1;
    }

    // Register ac3Temp handler
    if (netsnmp_register_read_only_int_instance("ac3Temp",
                                                ac3TempOid, OID_LENGTH(ac3TempOid),
                                                &ac3Temp, ac3TempCb) != 0) {
        snmp_log(LOG_ERR, "Failed to register ac3Temp!\n");
        return -1;
    }

    // Register hiTempAlarm handler
    if (netsnmp_register_int_instance("hiTempThreshold",
                                       hiTempThresholdOid, OID_LENGTH(hiTempThresholdOid),
                                       &hiTempThreshold, hiTempThresholdCb) != 0) {
        snmp_log(LOG_ERR, "Failed to register hiTempThreshold!\n");
        return -1;
    }

    // Start the MIB update task
    if (pthread_create(&thread, NULL, mibUpdateTask, (void *) cmdArgs)) {
        snmp_log(LOG_ERR, "Failed to create MIB update task!\n");
        return -1;
    }

    return 0;
}
