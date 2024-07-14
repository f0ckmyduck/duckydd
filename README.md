# Ducky Detector Daemon
This daemon protects from pretty much every HID injection attack.

## Compatibility Note:
This daemon depends on the following libraries: `udev, libc `

The following libraries can optionally be linked against to provide the daemon with key maps from the x server: `xkbcommon, xkbcommon-x11, xcb`

Systemd is not required although you will have to write your own init script if you don't want to use the provided service file.

## Install:
1. Clone the repository (don't forget the submodules).
2. Configure the build `meson setup build`
3. Compile `meson compile -C build`

## Usage:
For an up-to-date summary of all the CLI flags, just run with the -h flag.

## Configuration:
All configuration entries are in the "config" section.

`minimum_avg = 7327733`

Sets the minimum average difference time between keystrokes in nanoseconds. If the minimum average of the
currently typed string is smaller than the value defined here then the score of the device under scrutiny will be incremented.

`max_score = 1`

The so-called "score" of an event file is an internal variable which depicts
how dangerous the HID device is. If the daemon increments the score over the set max_score
and a key is pressed then it will grab the file descriptor that was opened
and thereby block any further keystrokes from being received by any other program
that was listening for events. 

At the moment it is incremented if a device:
1. with the same major and minor id registers as a input device and a virtual com port.
2. types faster than the allowed maximum average.

After the keyboard has been locked you have to replug it
to unlock it.


`daemon_log_path ./quack.log`

Sets the path where every log file is saved in.

If the process is passed the -d flag (daemonize) then it will also write
it's log messages to a file called out.log in that directory.

Otherwise it will just use the directory for the keylogging file which is called key.log.

__Note:__ You have to set a full path because the daemon has
to be started as root. Currently the parser does not expand the string
using environment variables.


`use_xkeymaps = false`

If this option is enabled the daemon will first try to initialize the key map from the X Server.

If the use of the X Server key map fails then the daemon falls back to kernel key maps.
At the moment the use of the kernel key maps is **experimental** although alphanumberic characters
of the English language work.

## Known issues:
### No protocol specified
If you get the message "No protocol specified" when starting the daemon as a service
then you need to add the user root to the list of trusted users of the X server.
This needs to be done so that the daemon can access the master keyboard.

You can test if this is the case by issuing the following command and then restarting the daemon.
`xhost local:root`

If the error disappears then you need to make the .AUTHORITY (which is usually in your home directory)
file containing the MIT-MAGIC-COOKIE readable to the root user.

You then need to export the XAUTHORITY environment variable like this before the daemon starts.
`export XAUTHORITY=/home/<username>/.Xauthority`

If you use systemd you can add the following line to the duckydd service file in the "[Service]" section.
```
[Service]
Environment="XAUTHORITY=/home/<username>/.Xauthority"
```

