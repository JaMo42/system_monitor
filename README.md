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
- `-f` ASCII art process tree (like the `--forest` option for `ps`)
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
- `f`: toggle procces ASCII art tree view
- Process searching
  - `/`: start search
  - `n`: select next search result
  - `N`: select previous search result

These can be viewed while the application is running by pressing `?`.

### Searching

During search the text can be modified using these keybindings:

- `Backspace`: Delete one character
- `Ctrl`+`Backspace`: Delete all characters until the first non-alphanumeric character
- `Ctrl`+`A`: Select everything, after this pressing `Backspace` deletes everything
   and entering a character deletes everything before adding that character.
   Pressing any special key that doesn't have a function cancels the selection.
- `Enter`: Finish search
- `Escape`: Cancel search

