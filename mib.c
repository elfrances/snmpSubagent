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
static long loTempThreshold = 28;

// hiTempThreshold OBJECT-TYPE
//     SYNTAX      Integer32
//     MAX-ACCESS  read-write
//     STATUS      current
//     DESCRIPTION "The temperature value (in degrees Celsius) above
//                  which to raise an A/C Unit High Temperature alarm."
//     DEFVAL      { 30 }
//     ::= { subagentExampleMIB 4 }
static const oid hiTempThresholdOid[] = { 1, 3, 6, 1, 3, 9999, 5, 0 };
static long hiTempThreshold = 30;

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

// This flag is set by the SIGUSR1 handler to
// indicate that the file snmpd.conf needs to
// be updated and the snmpd service restarted.
bool snmpdConfigChange = false;

typedef struct SnmpdConf {
    char dataBuf[65536];    // 65 KB big enough?
    size_t dataLen;
    bool restartSnmpd;
} SnmpdConf;

static int setConfigValue(const char *tag, const char *val, SnmpdConf *snmpdConf)
{
    char *buf = snmpdConf->dataBuf + snmpdConf->dataLen;
    size_t len = sizeof (snmpdConf->dataBuf) - snmpdConf->dataLen;
    int n = 0;

    if (strcmp(tag, "agentAddress") == 0) {
        // TODO: validate the syntax of the value string
        n = snprintf(buf, len, "agentaddress %s", val);
        snmpdConf->restartSnmpd = true;
    } else if (strcmp(tag, "readOnlyCommunity") == 0) {
        n = snprintf(buf, len, "rocommunity %s", val);
        snmpdConf->restartSnmpd = true;
    } else if (strcmp(tag, "readWriteCommunity") == 0) {
        n = snprintf(buf, len, "rwcommunity %s", val);
        snmpdConf->restartSnmpd = true;
    } else if (strcmp(tag, "trapReceiver") == 0) {
        // TODO: validate the syntax of the value string
        n = snprintf(buf, len, "trap2sink %s", val);
        snmpdConf->restartSnmpd = true;
    } else if (strcmp(tag, "sysContact") == 0) {
        n = snprintf(buf, len, "sysContact %s", val);
        snmpdConf->restartSnmpd = true;
    } else if (strcmp(tag, "sysLocation") == 0) {
        n = snprintf(buf, len, "sysLocation %s", val);
        snmpdConf->restartSnmpd = true;
    } else {
        snmp_log(LOG_WARNING, "%s: unsupported config tag \"%s\"\n", __func__, tag);
    }

    snmpdConf->dataLen += n;

    return 0;
}

static int procConfigFile(const char *configFile)
{
    static SnmpdConf snmpdConf;
    FILE *rdFp;
    char strBuf[256];

    // Clear the flag!
    snmpdConfigChange = false;

    // Init the SnmpdConf buffer
    snmpdConf.dataLen = 0;
    snmpdConf.restartSnmpd = false;

    // Open the configFile in read-only mode
    if ((rdFp = fopen(configFile, "r")) == NULL) {
        snmp_log(LOG_ERR, "%s: failed to open config file \"%s\"\n", __func__, configFile);
        return -1;
    }

    // Read one line at a time. Lines that start
    // with a '#' are comments and are skipped.
    while (fgets(strBuf, sizeof (strBuf), rdFp) != NULL) {
        if ((strBuf[0] != '#') && (strBuf[0] != '\0')) {
            char *comma = strchr(strBuf, ',');
            if (comma != NULL) {
                *comma = '\0';  // null-terminate tag string
                setConfigValue(strBuf, (comma + 1), &snmpdConf);
            }
        }
    }

    // Done with the configFile!
    fclose(rdFp);

    // Do we need to regenerate snmpd.conf and
    // restart the snmpd service?
    if (snmpdConf.restartSnmpd) {
        FILE *wrFp;
        time_t now = time(NULL);
        struct tm brkDwnTime;
        static char tsBuf[32];  // YYYY-MM-DDTHH:MM:SS

        strftime(tsBuf, sizeof (tsBuf), "%Y-%m-%d %H:%M:%S", gmtime_r(&now, &brkDwnTime));    // %H means 24-hour time

        // Open the snmpd.conf file in read-write mode
        if ((wrFp = fopen("/etc/snmp/snmpd.conf", "w+")) == NULL) {
            snmp_log(LOG_ERR, "%s: failed to open snmpd.conf file\n", __func__);
            return -1;
        }

        // Add an informational line
        fprintf(wrFp, "# This file was autogenerated by snmpSubagent on: %s\n", tsBuf);
        // AgentX support is always enabled
        fprintf(wrFp, "master agentx\n");
        // Append the rest of the configuration data
        fprintf(wrFp, "%s", snmpdConf.dataBuf);

        // Done with snmpd.conf!
        fclose(wrFp);

        snmp_log(LOG_INFO, "%s: Restarting snmpd service to pick up the new config...\n", __func__);

        // Now restart the "snmpd" service to pick up the
        // config changes.
        if (system("systemctl restart snmpd") == -1) {
            int errNo = errno;
            snmp_log(LOG_ERR, "%s: Failed to exec \"systemctl restart snmpd\": %s (%d)\n", __func__, strerror(errNo), errNo);
            return -1;
        }
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

static const __syscall_slong_t oneBillion = 1000000000; // 10^9

// Normalize a timespec value
static __inline__ void tvNorm(struct timespec *t)
{
    if (t->tv_nsec >= oneBillion) {
        t->tv_sec += (t->tv_nsec / oneBillion);
        t->tv_nsec = (t->tv_nsec % oneBillion);
    }
}

// Subtract two timespec values: result = x - y
static void tvSub(struct timespec *result, const struct timespec *x, const struct timespec *y)
{
    struct timespec X = *x;
    struct timespec Y = *y;

    tvNorm(&X);
    tvNorm(&Y);

    if (X.tv_nsec < Y.tv_nsec) {
        X.tv_sec -= 1;
        X.tv_nsec += oneBillion;
    }
    result->tv_sec = X.tv_sec - Y.tv_sec;
    result->tv_nsec = X.tv_nsec - Y.tv_nsec;
}

// Compare two timespec values
static int tvCmp(const struct timespec *x, const struct timespec *y)
{
    struct timespec X = *x;
    struct timespec Y = *y;

    tvNorm(&X);
    tvNorm(&Y);

    if (x->tv_sec > y->tv_sec) {
        return 1;
    } else if (x->tv_sec < y->tv_sec) {
        return -1;
    } else if (x->tv_nsec > y->tv_nsec) {
        return 1;
    } else if (x->tv_nsec < y->tv_nsec) {
        return -1;
    }

    return 0;
}


// This task is used to update the values of the MIB objects; e.g.
// as when the value is obtained from reading an environmental
// sensor.
static void *mibUpdateTask(void *arg)
{
    const CmdArgs *cmdArgs = arg;

    while (true) {
        const struct timespec pollTime = { .tv_sec = 1, .tv_nsec = 0 };
        struct timespec startTime, endTime, deltaTime;
        struct tm brkDwnTime;
        static char tsBuf[32];  // YYYY-MM-DDTHH:MM:SS

        clock_gettime(CLOCK_REALTIME, &startTime);

        strftime(tsBuf, sizeof (tsBuf), "%Y-%m-%d %H:%M:%S", gmtime_r(&startTime.tv_sec, &brkDwnTime));    // %H means 24-hour time

        snmp_log(LOG_INFO, "%s: Updating MIB data from %s at %s ...\n", __func__, cmdArgs->dataFile, tsBuf);

        // Process the data file
        procDataFile(cmdArgs->dataFile);

        // Was there a config change?
        if (snmpdConfigChange) {
            procConfigFile(cmdArgs->configFile);
        }

        clock_gettime(CLOCK_REALTIME, &endTime);

        // Calculate the time we spent processing
        // all the events in this pass of the work
        // loop...
        tvSub(&deltaTime, &endTime, &startTime);

        // Figure out how long to sleep to achieve
        // the desired poll period. If deltaTime is
        // greater or equal to pollTime, there's no
        // need to sleep.
        if (tvCmp(&deltaTime, &pollTime) < 0) {
            // Sleep until the next poll period
            struct timespec sleepTime;
            tvSub(&sleepTime, &pollTime, &deltaTime);
            //snmp_log(LOG_INFO, "%s: sleepTime=%ld.%09ld\n", __func__, sleepTime.tv_sec, sleepTime.tv_nsec);
            nanosleep(&sleepTime, NULL);
        }
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
