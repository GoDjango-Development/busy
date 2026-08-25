# busy

`busy` is a small Linux utility designed to generate a high CPU workload for testing and observing CPU temperature and thermal behavior.

It automatically detects the number of CPUs currently online and creates one worker process per CPU. Each worker continuously consumes CPU time, allowing the system to reach a high CPU utilization level.

The main purpose is to make it easy to observe how CPU temperature changes under sustained load.

## Usage

Build the program:

```bash
make prepare
make
```

Run it:

```bash
./busy
```

In another terminal, monitor the CPU sensors:

```bash
watch -n 1 sensors
```

You can also use `busy` together with `watch` in a pipeline:

```bash
./busy | watch -n 1 sensors
```

This is a convenient way to run the CPU workload while continuously refreshing the `sensors` output.

## How it works

`busy`:

1. Detects the number of online CPUs using POSIX `sysconf()`.
2. Creates one worker process for each CPU.
3. Places the worker processes in the same process group.
4. Each worker continuously consumes CPU time.
5. `Ctrl+C` stops the workload.

The number of workers is therefore determined automatically by the system rather than being hard-coded.

## Example

Run the stress test and monitor the CPU temperature:

```bash
./busy | watch -n 1 sensors
```

Example output:

```text
Package id 0:  +75.0°C
Core 0:        +74.0°C
Core 1:        +76.0°C
Core 2:        +73.0°C
Core 3:        +75.0°C
```

You can stop the test with `Ctrl+C`.

## Requirements

* Linux
* POSIX-compatible C environment
* `make`
* A working `lm-sensors` installation for temperature monitoring

On Debian-based systems, install `lm-sensors` with:

```bash
sudo apt install lm-sensors
```

## Warning

`busy` intentionally generates a sustained high CPU load.

CPU temperature can increase significantly while it is running. Make sure the system has adequate cooling and stop the program if temperatures become excessive.

This utility is intended for **testing, diagnostics, thermal observation, and cooling-system verification**.

## License

This project is licensed under the **MIT License**.
