# snmpSubagent

A minimalistic SNMP subagent that uses the NET-SNMP AgentX API to implement the SUBAGENT-EXAMPLE-MIB.

# Build

To build the binary simply run make:

```
make
```

# Run

First make sure your snmpd master agent supports AgentX, and that its ro and rw community strings are "public" and "private", respectively.

```
sudo ./snmpSubagent 
```

You should see the following log messages:

```
NET-SNMP version 5.9.4.pre2 AgentX subagent connected
snmpSubagent running...
```

# Test

Read the read-only Interger32 MIB variable "myReadOnlyInteger":

```
snmpget -v 2c -c public localhost -M+. -mSUBAGENT-EXAMPLE-MIB myReadOnlyInteger.0
SUBAGENT-EXAMPLE-MIB::myReadOnlyInteger.0 = INTEGER: 3662
```

Read the read-write Interger32 MIB variable "myReadWriteInteger":

```
snmpget -v 2c -c public localhost -M+. -mSUBAGENT-EXAMPLE-MIB myReadWriteInteger.0
SUBAGENT-EXAMPLE-MIB::myReadWriteInteger.0 = INTEGER: 0
```

Write the value 1234 to the read-write Interger32 MIB variable "myReadWriteInteger":

```
snmpset -v 2c -c private localhost .1.3.6.1.3.9999.1.2.0 i 1234
SNMPv2-SMI::experimental.9999.1.2.0 = INTEGER: 1234
```

Read the read-write Interger32 MIB variable "myReadWriteInteger" to verify the value written:

```
snmpget -v 2c -c public localhost -M+. -mSUBAGENT-EXAMPLE-MIB myReadWriteInteger.0
SUBAGENT-EXAMPLE-MIB::myReadWriteInteger.0 = INTEGER: 1234
```




