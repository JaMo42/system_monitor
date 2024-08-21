# Theme configuration

```ini
[theme]
border = 92
title = 123
proc-header = SlateBlue1
```

## Values

Color values are parsed as strings and can be either a number or the name of a color (Both are listed here for example: https://ss64.com/bash/syntax-colors.html).

## Keys

- `border` widget border
- `title` widget title and other text on widget border
- `cpu-avg` graph and label for average CPU usage
- `cpu-graph-0` graph and label for CPU core 0
- ...
- `cpu-graph-7`
- `mem-main` main memory graph and label
- `mem-swap` swap memory graph and label
- `net-receive` network receive graph and label
- `net-transmit` network transmit graph and label
- `proc-header` process viewer column headers
- `proc-processes` PID, cpu usage, and memory usage
- `proc-cursor` the entire cursor line
- `proc-highlight` lines that match search
- `proc-high-percent` high cpu or memory usage numbers
- `proc-branches` the branches in forest mode
- `proc-path` the path of programs
- `proc-command` the filename of programs
- `proc-opt` options in a command line
- `proc-arg` arguments in a command line
- `disk-free` background of donut graph
- `disk-used` fill of donut graph
- `disk-error` mount point label if an error occurred
- `battery-fill` background and text


