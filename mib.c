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

// ac2Temp OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-only
//     STATUS      current
//     DESCRIPTION "The current value (in degrees Celsius) of the A/C
//                  Unit #2 temperature sensor."
//     ::= { subagentExampleMIB 2 }
static const oid ac2TempOid[] = { 1, 3, 6, 1, 3, 9999, 2, 0 };
static int ac2Temp = 0;

// ac3Temp OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-only
//     STATUS      current
//     DESCRIPTION "The current value (in degrees Celsius) of the A/C
//                  Unit #3 temperature sensor."
//     ::= { subagentExampleMIB 3 }
static const oid ac3TempOid[] = { 1, 3, 6, 1, 3, 9999, 3, 0 };
static int ac3Temp = 0;

// loTempThreshold OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-write
//     STATUS      current
//     DESCRIPTION "The temperature value (in degrees Celsius) below
//                  which to clear an A/C Unit High Temperature alarm."
//     DEFVAL      { 28 }
//     ::= { subagentExampleMIB 4 }
static const oid loTempThresholdOid[] = { 1, 3, 6, 1, 3, 9999, 4, 0 };
static long int loTempThreshold = 28;

// hiTempThreshold OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-write
//     STATUS      current
//     DESCRIPTION "The temperature value (in degrees Celsius) above
//                  which to raise an A/C Unit High Temperature alarm."
//     DEFVAL      { 30 }
//     ::= { subagentExampleMIB 4 }
static const oid hiTempThresholdOid[] = { 1, 3, 6, 1, 3, 9999, 5, 0 };
static long int hiTempThreshold = 30;

// acHiTempAlarmUnit OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  accessible-for-notify
//     STATUS      current
//     DESCRIPTION "The A/C Unit that raised a High Temperature alarm."
//     ::= { subagentExampleMIB 5 }
static const oid acHiTempAlarmUnitOid[] = { 1, 3, 6, 1, 3, 9999, 6 };

// acHiTempAlarmState OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  accessible-for-notify
//     STATUS      current
//     DESCRIPTION "Indicates the current state of the High Temperature
//                  alarm: 0 means the alarm is inactive and 1 indicates
//                  the alarm is active."
//     ::= { subagentExampleMIB 6 }
static const oid acHiTempAlarmStateOid[] = { 1, 3, 6, 1, 3, 9999, 7 };
static int acHiTempAlarmState = 0;

// acHiTempAlarmNotification NOTIFICATION-TYPE
//     OBJECTS     { acHiTempUnit }
//     STATUS      current
//     DESCRIPTION "Trap to notify a High Temperature alarm on one of
//                  the A/C Units."
//     ::= { subagentExampleMIB 7 }
static const oid acHiTempAlarmNotificationOid[] = { 1, 3, 6, 1, 3, 9999, 8 };

// SET callback handlers

static int loTempThresholdCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    netsnmp_variable_list *varBind = requests->requestvb;
    long value = *varBind->val.integer;
    snmp_log(LOG_INFO, "%s: type=%u val=%ld\n", __func__, varBind->type, value);

    // Make sure the value is lower than hiTempThreshold
    if (value >= hiTempThreshold) {
        snmp_log(LOG_ERR, "%s: loTempThreshold=%ld MUST be lower than hiTempThreshold=%ld !\n", __func__, value, hiTempThreshold);
        return SNMP_ERR_INCONSISTENTVALUE;
    }

    // TODO: Does this change in the loTempThreshold value
    // affect the status of the alarm?

    return SNMP_ERR_NOERROR;
}

static int hiTempThresholdCb(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests)
{
    netsnmp_variable_list *varBind = requests->requestvb;
    long value = *varBind->val.integer;
    snmp_log(LOG_INFO, "%s: type=%u val=%ld\n", __func__, varBind->type, *varBind->val.integer);

    // Make sure the value is higher than loTempThreshold
    if (value <= loTempThreshold) {
        snmp_log(LOG_ERR, "%s: hiTempThreshold=%ld MUST be higher than loTempThreshold=%ld !\n", __func__, value, loTempThreshold);
        return SNMP_ERR_INCONSISTENTVALUE;
    }

    // TODO: Does this change in the hiTempThreshold value
    // affect the status of the alarm?

    return SNMP_ERR_NOERROR;
}


typedef struct MibObj {
    const char *varName;
    const oid *varOid;
    size_t varOidLen;
    int *varValue;
    bool readOnly;
    Netsnmp_Node_Handler *varCbFunc;
} MibObj;

// This table contains one entry for each read-only or
// read-write object in the MIB.
static MibObj mibObjTbl[] = {
        { "ac1Temp", ac1TempOid, OID_LENGTH(ac1TempOid), &ac1Temp, true, NULL },
        { "ac2Temp", ac2TempOid, OID_LENGTH(ac2TempOid), &ac2Temp, true, NULL },
        { "ac3Temp", ac3TempOid, OID_LENGTH(ac3TempOid), &ac3Temp, true, NULL },
        { "loTempThreshold", loTempThresholdOid, OID_LENGTH(loTempThresholdOid), (int *) &loTempThreshold, false, loTempThresholdCb },
        { "hiTempThreshold", hiTempThresholdOid, OID_LENGTH(hiTempThresholdOid), (int *) &hiTempThreshold, false, hiTempThresholdCb },
        { NULL, NULL, 0, NULL, NULL }
};

static int sendHiTempAlarmTrap(const char *varName)
{
    netsnmp_variable_list *varList = NULL;
    const oid snmpTrapOid[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };
    int acUnit = 0;

    // Extract the A/C unit number
    if (sscanf(varName, "ac%dTemp", &acUnit) != 1) {
        snmp_log(LOG_ERR, "%s: Invalid A/C unit: %s\n", __func__, varName);
        return -1;
    }

    // Add varbind: snmpTrapOID = acHiTempAlarmNotificationOid
    snmp_varlist_add_variable(&varList,
            snmpTrapOid, OID_LENGTH(snmpTrapOid),
            ASN_OBJECT_ID,
            acHiTempAlarmNotificationOid, OID_LENGTH(acHiTempAlarmNotificationOid) * sizeof (oid));

    // Add varbind: acHiTempAlarmUnit = acUnit
    snmp_varlist_add_variable(&varList,
            acHiTempAlarmUnitOid, OID_LENGTH(acHiTempAlarmUnitOid),
            ASN_INTEGER,
            &acUnit, sizeof (acUnit));

    // Add varbind: acHiTempAlarmState = alarmState
    snmp_varlist_add_variable(&varList,
            acHiTempAlarmStateOid, OID_LENGTH(acHiTempAlarmStateOid),
            ASN_INTEGER,
            &acHiTempAlarmState, sizeof (acHiTempAlarmState));

    snmp_log(LOG_ERR, "%s: Sending trap for: %s\n", __func__, varName);

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

static int setReadOnlyValue(const char *varName, int value)
{
    for (MibObj *mibObj = &mibObjTbl[0]; mibObj->varName != NULL; mibObj++) {
        if (strcmp(varName, mibObj->varName) == 0) {
            // Make sure it is a read-only object
            if (!mibObj->readOnly) {
                snmp_log(LOG_ERR, "%s: MIB object \"%s\" is not read-only !\n", __func__, varName);
                return -1;
            }

            // Has the value changed?
            if (value != *mibObj->varValue) {
                // Yes! Update the value
                snmp_log(LOG_INFO, "%s: varName=%s oldValue=%d newValue=%d\n", __func__, varName, *mibObj->varValue, value);
                *mibObj->varValue = value;

                // Do we need to send a hiTempAlarm trap?
                if ((acHiTempAlarmState == 0) && (value > hiTempThreshold)) {
                    snmp_log(LOG_INFO, "%s: varName=%s newValue=%d is greater than hiTempThreshold=%ld !\n", __func__, varName, value, hiTempThreshold);
                    acHiTempAlarmState = 1; // raise the alarm
                    sendHiTempAlarmTrap(varName);
                } else if ((acHiTempAlarmState == 1) && (value < loTempThreshold)) {
                    snmp_log(LOG_INFO, "%s: varName=%s newValue=%d is lower than loTempThreshold=%ld !\n", __func__, varName, value, loTempThreshold);
                    acHiTempAlarmState = 0; // clear the alarm
                    sendHiTempAlarmTrap(varName);
                }
            }

            return 0;
        }
    }

    snmp_log(LOG_WARNING, "%s: Unsupported MIB object \"%s\" !\n", __func__, varName);

    return -1;

    int *var = NULL;
    bool acTemp = false;

    if (strcmp(varName, "ac1Temp") == 0) {
        var = &ac1Temp;
        acTemp = true;
    } else if (strcmp(varName, "ac2Temp") == 0) {
        var = &ac2Temp;
        acTemp = true;
    } else if (strcmp(varName, "ac3Temp") == 0) {
        var = &ac3Temp;
        acTemp = true;
    } else {
        return -1;
    }

    if ((var != NULL) && (value != *var)) {
        snmp_log(LOG_INFO, "%s: oid=%s oldValue=%d newValue=%d\n", __func__, varName, *var, value);

        // Update the corresponding MIB variable
        *var = value;

        // If this is an ac1Temp-ac3Temp object, do we
        // need to send a hiTempAlarm trap?
        if (acTemp && (value > hiTempThreshold)) {
            snmp_log(LOG_INFO, "%s: oid=%s newValue=%d is greater than hiTempThreshold=%ld !\n", __func__, varName, value, hiTempThreshold);
            sendHiTempAlarmTrap(varName);
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
                    setReadOnlyValue(strBuf, value);
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

    // Register with the Master Agent each of the Integer32
    // read-only and read-write objects in our MIB...
    for (MibObj *mibObj = &mibObjTbl[0]; mibObj->varName != NULL; mibObj++) {
        int s;

        snmp_log(LOG_INFO, "Registering object: varName=\"%s\" readOnly=%d ...\n", mibObj->varName, mibObj->readOnly);

        if (mibObj->readOnly) {
            s = netsnmp_register_read_only_int_instance(mibObj->varName,
                                                        mibObj->varOid,
                                                        mibObj->varOidLen,
                                                        mibObj->varValue,
                                                        mibObj->varCbFunc);
        } else {
            s = netsnmp_register_long_instance(mibObj->varName,
                                              mibObj->varOid,
                                              mibObj->varOidLen,
                                              (long int *) mibObj->varValue,
                                              mibObj->varCbFunc);
        }

        if (s != 0) {
            snmp_log(LOG_ERR, "Failed to register \"%s\" !\n", mibObj->varName);
            return -1;
        }
    }

    // Start the MIB update task
    if (pthread_create(&thread, NULL, mibUpdateTask, (void *) cmdArgs)) {
        snmp_log(LOG_ERR, "Failed to create MIB update task!\n");
        return -1;
    }

    return 0;
}
