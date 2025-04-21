#!/bin/bash

# Stop the snmptrapd service
sudo systemctl stop snmptrapd

# Get rid of the snmptrapd.socket stuff
sudo rm -f /etc/systemd/system/sockets.target.wants/snmptrapd.socket
sudo mv /usr/lib/systemd/system/snmptrapd.socket /usr/lib/systemd/system/snmptrapd.socket.ORIG
sudo rm -f /var/lib/systemd/deb-systemd-helper-enabled/sockets.target.wants/snmptrapd.socket

# Create a basic snmptrapd.service file that
# logs its messages to syslog.
sudo cp snmptrapd.service /etc/systemd/system/snmptrapd.service

# Reload the systemd daemon info
sudo systemctl daemon-reload

# Start the snmptrapd service
sudo systemctl start snmptrapd

# If you want the snmptrapd service to start
# automatically when the system boots, then
# enable it.
sudo systemctl enable snmptrapd

# Reboot the node to come back up with a fresh
# system...
sudo reboot now

