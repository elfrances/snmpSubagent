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

Copy the SUBAGENT-EXAMPLE-MIB file to the default MIB's folder, usually /usr/share/snmp/mibs.  Alternatively, you can create a symlink to the MIB file instead of copying it.

Running the binary with the --help option will show the supported options:

```
./snmpSubagent --help
SYNTAX:
    snmpSubagent [OPTIONS]

OPTIONS:
    --daemon
        Run in the background.
    --data-file <path>
        Path to the CSV file used to update the value of the
        SUBAGENT-EXAMPLE-MIB objects.
    --help
        Show this help and exit.
    --syslog
        Use syslog for logging.
```

Start the snmpSubagent via sudo, so that it runs with the required privileges:


```
sudo ./snmpSubagent --data-file dataFile.csv
```

You should see the following log messages:

```
NET-SNMP version 5.9.4.pre2 AgentX subagent connected
snmpSubagent running...
mibUpdateTask: Updating MIB data from dataFile.csv at 2025-04-13 19:44:28 ...
mibUpdateTask: Updating MIB data from dataFile.csv at 2025-04-13 19:44:29 ...
mibUpdateTask: Updating MIB data from dataFile.csv at 2025-04-13 19:44:30 ...
mibUpdateTask: Updating MIB data from dataFile.csv at 2025-04-13 19:44:31 ...
mibUpdateTask: Updating MIB data from dataFile.csv at 2025-04-13 19:44:32 ...

    .
    .
    .
```

# Test the snmpSubagent

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

Read the read-write Interger32 MIB variable "hiTempThreshold":

```
snmpget -v 2c -c public localhost SUBAGENT-EXAMPLE-MIB::hiTempThreshold.0
SUBAGENT-EXAMPLE-MIB::hiTempThreshold.0 = INTEGER: 30
```

Write the value 27 to the read-write Interger32 MIB variable "hiTempThreshold":

```
snmpset -v 2c -c private localhost SUBAGENT-EXAMPLE-MIB::hiTempThreshold.0 i 27
SUBAGENT-EXAMPLE-MIB::hiTempThreshold.0 = INTEGER: 27
```

Read the read-write Interger32 MIB variable "hiTempThreshold" to verify the value written:

```
snmpget -v 2c -c public localhost SUBAGENT-EXAMPLE-MIB::hiTempThreshold.0
SUBAGENT-EXAMPLE-MIB::hiTempThreshold.0 = INTEGER: 1234
```

# Control the snmpSubagent using systemd

Edit the file snmpSubagent.service as needed, and copy it to /etc/systemd/system:

```
sudo cp snmpSubagent.service /etc/systemd/system
```

Reload systemd:

```
sudo systemctl daemon-reload
```

Now you can start the snmpSubagent service using systemctl:

```
sudo systemctl start snmpSubagent
```

And you can get its runtime status:

```
● snmpSubagent.service - SNMP AgentX Subagent for the SUBAGENT-EXAMPLE-MIB.
     Loaded: loaded (/etc/systemd/system/snmpSubagent.service; disabled; preset: enabled)
     Active: active (running) since Sun 2025-04-13 13:54:06 MDT; 3s ago
   Main PID: 2362563 (snmpSubagent)
      Tasks: 2 (limit: 18844)
     Memory: 11.5M (peak: 11.7M)
        CPU: 142ms
     CGroup: /system.slice/snmpSubagent.service
             └─2362563 /home/mmourier/GitHub/snmpSubagent/snmpSubagent --data-file /home/mmourier/GitHub/snmpSubagent/dataFile.csv

Apr 13 13:54:06 aspire snmpSubagent[2362563]: Did not find 'IANAbfdSessTypeTC' in module #-1 (/usr/share/snmp/mibs/ietf/BFD-STD-MIB)
Apr 13 13:54:06 aspire snmpSubagent[2362563]: Did not find 'IANAbfdSessOperModeTC' in module #-1 (/usr/share/snmp/mibs/ietf/BFD-STD-MIB)
Apr 13 13:54:06 aspire snmpSubagent[2362563]: Did not find 'IANAbfdSessStateTC' in module #-1 (/usr/share/snmp/mibs/ietf/BFD-STD-MIB)
Apr 13 13:54:06 aspire snmpSubagent[2362563]: Did not find 'IANAbfdSessAuthenticationTypeTC' in module #-1 (/usr/share/snmp/mibs/ietf/BFD-STD-MIB)
Apr 13 13:54:06 aspire snmpSubagent[2362563]: Did not find 'IANAbfdSessAuthenticationKeyTC' in module #-1 (/usr/share/snmp/mibs/ietf/BFD-STD-MIB)
Apr 13 13:54:06 aspire snmpSubagent[2362563]: NET-SNMP version 5.9.4.pre2 AgentX subagent connected
Apr 13 13:54:06 aspire snmpSubagent[2362563]: snmpSubagent running...
Apr 13 13:54:07 aspire snmpSubagent[2362563]: mibUpdateTask: Updating MIB data from /home/mmourier/GitHub/snmpSubagent/dataFile.csv at 2025-04-13 19:54:07 ...
Apr 13 13:54:08 aspire snmpSubagent[2362563]: mibUpdateTask: Updating MIB data from /home/mmourier/GitHub/snmpSubagent/dataFile.csv at 2025-04-13 19:54:08 ...
Apr 13 13:54:09 aspire snmpSubagent[2362563]: mibUpdateTask: Updating MIB data from /home/mmourier/GitHub/snmpSubagent/dataFile.csv at 2025-04-13 19:54:09 ...
```

To start the snmpSubagent service automatically when the system boots, simply enable the service:

```
sudo systemctl enable snmpSubagent
Created symlink /etc/systemd/system/multi-user.target.wants/snmpSubagent.service → /etc/systemd/system/snmpSubagent.service.
```







