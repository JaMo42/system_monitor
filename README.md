# system_monitor

A terminal based graphical system monitor, inspired by [gotop](https://github.com/cjbassi/gotop).

## Building

```
git clone https://github.com/JaMo42/system_monitor
cd system_monitor
make
```

## Usage

### Commandline options

- `-a` show average CPU usage
- `-r rate` update rate in milliseconds
- `-s scale` horizontal graph scale
- `-c` always show CPU graph in range 0~100%
- `-h` show help message

### Keybinds

- `q`: Quit
- Process navigation
  - `k` and `<Up>`: up
  - `j` and `<Down>`: down
  - `K`: up by 5
  - `J`: down by 5
  - `g`: jump to top
  - `G`: jump to bottom
- Process sorting
  - `c`: sort by CPU usage (default)
  - `m`: sort by memory usage
  - `p`: sort by PID (ascending)
  - `P`: sort by PID (descending)

These can be viewed while the application is running by pressing `?`.

