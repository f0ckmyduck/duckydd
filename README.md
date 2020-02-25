# Ducky Detector Daemon
This is a daemon which should protect the user from pretty much every HID injection attack.
(If configured right)

## Install:
```
mkdir bin
cd bin
cmake ..
make
sudo make install
```

## Configuration
The config file should be located under /etc/duckydd/duckydd.conf.

__Note:__ The standard config the daemon is shipped with, should
protect against any injector which serves a virtual com port over
the same usb port which is used by the attack itself.
However if the device does __not__ serve a virtual com port
then the daemon will simply __ignore__ it.

The config file format is pretty simple.

\<parameter> \<option>[,\<option>][,...];

## Example config entries:
`blacklist 29,97,56,100;`

With this option you can configure which keys will lock the keyboard.

A list of all the keycodes which identify the keys can be found in
the input-event-codes header. If you don't want to search
for that header then you can use the bash script called getkey.sh.
The script will search for the header using locate and then list
all of the key macros and their keycode.


`maxtime 10s 0ns;`

This will set the maximum time after which the device will be
removed from the watchlist. After that time period the daemon
will simply ignore all events that are generated from that event file.


`maxscore 0;`

The so called score of an event file is an internal variable which depicts
how dangerous the event file is. If the daemon increments the score over the set maxscore
and a blacklisted key is pressed then it will grab the file descriptor that was opened
and thereby block any further keystrokes from being received from any other program
that was listening for events. 

At the moment it is only incremented if a device with the same
vendor id registers as a keyboard and a virtual com port.

Therefore if you leave it at 0 the daemon will lock all keyboards
which type a single blacklisted key and haven't timed out.
If you set it to 1 then it will only lock devices which have been registered
as a keyboard and a serial com port.


After the keyboard has been locked you have to replug it
to unlock it.

## Uninstall:
```
cd bin
xargs rm < install_manifest.txt
sudo rm -rf /etc/duckydd
```
