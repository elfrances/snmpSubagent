# snmpSubagent

A minimalistic SNMP subagent that uses the NET-SNMP AgentX API to implement the SUBAGENT-EXAMPLE-MIB.

# Build

The program is written in C and uses the NET-SNMP libraries.  On Ubuntu these libraries can be installed as follows:

```
sudo apt install libsnmp-dev
```

To build the binary simply run make:

```
make
```

# Run

First make sure your snmpd master agent supports AgentX, and that its ro and rw community strings are "public" and "private", respectively, as shown below:

```
rocommunity  public 
rwcommunity  private
```

Copy the SUBAGENT-EXAMPLE-MIB file to the default MIB's folder, usually /usr/share/snmp/mibs.

Start the snmpSubagent:


```
sudo ./snmpSubagent 
```

You should see the following log messages:

```
NET-SNMP version 5.9.4.pre2 AgentX subagent connected
snmpSubagent running...
mibUpdateTask: Updating MIB data...
mibUpdateTask: Updating MIB data...
mibUpdateTask: Updating MIB data...
    .
    .
    .
```

# Test

Read the read-only Interger32 MIB variable "ac1Temp":

```
snmpget -v 2c -c public localhost SUBAGENT-EXAMPLE-MIB::ac1Temp.0
SUBAGENT-EXAMPLE-MIB::ac1Temp.0 = INTEGER: 21
```

Read the read-only Interger32 MIB variable "ac2Temp":

```
snmpget -v 2c -c public localhost SUBAGENT-EXAMPLE-MIB::ac2Temp.0
SUBAGENT-EXAMPLE-MIB::ac2Temp.0 = INTEGER: 22
```

Read the read-write Interger32 MIB variable "hiTempAlarm":

```
snmpget -v 2c -c public localhost SUBAGENT-EXAMPLE-MIB::hiTempAlarm.0
SUBAGENT-EXAMPLE-MIB::hiTempAlarm.0 = INTEGER: 30
```

Write the value 27 to the read-write Interger32 MIB variable "hiTempAlarm":

```
snmpset -v 2c -c private localhost SUBAGENT-EXAMPLE-MIB::hiTempAlarm.0 i 27
SUBAGENT-EXAMPLE-MIB::hiTempAlarm.0 = INTEGER: 27
```

Read the read-write Interger32 MIB variable "hiTempAlarm" to verify the value written:

```
snmpget -v 2c -c public localhost SUBAGENT-EXAMPLE-MIB::hiTempAlarm.0
SUBAGENT-EXAMPLE-MIB::hiTempAlarm.0 = INTEGER: 1234
```




