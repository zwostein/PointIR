To run pointird in windows, a dbus daemon must be running.
This is a brief description on how to get it working:

First start a system message bus instance using the provided example configuration file:
# dbus-daemon.exe --config-file=system.conf

Then export the system bus address as an environment variable:
# set DBUS_SYSTEM_BUS_ADDRESS=tcp:host=127.0.0.1,port=1234

Now pointird should be able to connect to the system bus:
# pointird.exe

Run a calibration tool. (don't forget to export DBUS_SYSTEM_BUS_ADDRESS for this one, too)
