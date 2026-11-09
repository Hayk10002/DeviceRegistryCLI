# Device Registry CLI

A small C++23 command-line application for registering and managing devices.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Usage

Run the executable with a subcommand:

```sh
./build/device-registry add --id sensor-1 --name "Temperature Sensor" --type sensor --priority 8 --tags room-a critical
./build/device-registry list
./build/device-registry stats
./build/device-registry set-state --id sensor-1 --state on
./build/device-registry add-to-alert-queue --id sensor-1
./build/device-registry alert
```

See all available commands and options with:

```sh
./build/device-registry --help
```

Full help output:

```text
Device Registration App


Usage: [OPTIONS] [SUBCOMMANDS]


OPTIONS:
	-h,     --help              Print this help message and exit

SUBCOMMANDS:
	add                         Add a device
	remove                      Remove a device
	list                        List all devices
	list-by-priority            List all devices sorted by priority
	list-with-tag               List all devices with the specified tag
	list-high-priority          List all devices with the specified priority or higher
	alert                       Alert (print) top 3 devices with the highest priorities in the
															alert queue, and remove them from it
	stats                       Show some statistics about the registry
	add-to-alert-queue          Add a device to the alert queue
	set-state                   Set the state of a device
```

Running without arguments starts interactive mode. Registry data is saved to
`devices.dat` in the current working directory.