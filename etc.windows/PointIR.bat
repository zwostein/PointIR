@echo off
start /B dbus-daemon.exe --config-file=system.conf 1> dbus-daemon-log.txt 2>&1
set DBUS_SYSTEM_BUS_ADDRESS=tcp:host=127.0.0.1,port=1234
pointird.exe
