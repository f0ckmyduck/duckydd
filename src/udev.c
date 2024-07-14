
#include <libudev.h>
#include <string.h>

#include "main.h"
#include "udev.h"

int has_tty(struct udevInfo udev)
{
	int err = 0;
	struct udev_enumerate *en;
	struct udev_list_entry *le;

	en = udev_enumerate_new(udev.udev);
	if (en == NULL) {
		LOG(-1, "udev_enumerate_new failed\n");
		err = -1;
		goto error_exit;
	}

	// match to the name starting with tty
	if (udev_enumerate_add_match_subsystem(en, "tty") < 0) {
		LOG(-1, "udev_enumerate_add_match_subsystem failed\n");
		err = -2;
		goto error_exit;
	}

	// match to major and minor number of the device
	if (udev_enumerate_add_match_property(
		    en, "MAJOR",
		    udev_device_get_property_value(udev.dev, "MAJOR")) < 0) {
		LOG(-1, "udev_enumerate_add_match_property failed\n");
		err = -3;
		goto error_exit;
	}

	if (udev_enumerate_add_match_property(
		    en, "MINOR",
		    udev_device_get_property_value(udev.dev, "MINOR")) < 0) {
		LOG(-1, "udev_enumerate_add_match_property failed\n");
		err = -4;
		goto error_exit;
	}

	// get iterator
	udev_enumerate_scan_devices(en);

	le = udev_enumerate_get_list_entry(
		en); // loop through all entities and search for a usb tty
	while (le != NULL) {
		char temp[10];

		// find ttyACM or ttyUSB devices
		snprintf(temp, 6, "%s",
			 find_file(udev_list_entry_get_name(le)));
		if (memcmp(temp, "ttyACM", 6) == 0 ||
		    memcmp(temp, "ttyUSB", 6) == 0) {
			udev_enumerate_unref(en);
			return 1;
		}
		le = udev_list_entry_get_next(le);
	}

error_exit:
	udev_enumerate_unref(en);
	return err;
}

int init_udev(struct udevInfo *udev)
{
	udev->udev = udev_new();
	if (udev->udev == NULL) {
		LOG(-1, "udev_new failed\n");
		return -1;
	}

	// create a new monitor from the context
	udev->mon = udev_monitor_new_from_netlink(udev->udev, "udev");
	if (udev->mon == NULL) {
		LOG(-1, "udev_monitor_new_from_netlink failed\n");
		return -2;
	}

	// match only input devices
	if (udev_monitor_filter_add_match_subsystem_devtype(udev->mon, "input",
							    NULL) < 0) {
		LOG(-1,
		    "udev_monitor_filter_add_match_subsystem_devtype failed (input)\n");
		return -3;
	}

	// start receiving events
	if (udev_monitor_enable_receiving(udev->mon) < 0) {
		LOG(-1, "udev_monitor_enable_receiving failed\n");
		return -4;
	}

	udev->udevfd = udev_monitor_get_fd(udev->mon);
	if (udev->udevfd == -1) {
		LOG(-1, "udev_monitor_get_fd failed\n");
		return -5;
	}
	return 0;
}

void deinit_udev(struct udevInfo *udev)
{
	udev_monitor_unref(udev->mon);
	udev_unref(udev->udev);
}
