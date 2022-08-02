# system_monitor

A terminal based graphical system monitor, inspired by [gotop](https://github.com/cjbassi/gotop).

![system_monitor](./screenshot.png)

## Building

```
git clone https://github.com/JaMo42/system_monitor
cd system_monitor
make
```

## Usage

### Command-line options

- `-a` show average CPU usage
- `-r rate` update rate in milliseconds
- `-s scale` horizontal graph scale
- `-c` always show CPU graph in range 0~100%
- `-f` ASCII art process tree (like the `--forest` option for `ps`)
- `-l layout` specifies the [layout](#layout)
- `-h` show help message

If the layout option for `-l` is `?` the current layout string (either the default or the `SM_LAYOUT` environment variable) gets printed.

### Keybindings

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
- `f`: toggle process ASCII art tree view
- Process searching
  - `/`: start search
  - `n`: select next search result
  - `N`: select previous search result
- CPU graph
  - `a`: toggle average CPU usage
  - `C`: togle CPU graph range scaling

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

## Layout

A custom layout can be specified via the `SM_LAYOUT` environment variable or the `-l` argument (overrides environment variable).

If neither is specified the default is `(rows 33% c[2] (cols (rows m[1] n[0]) p[3]))`.

Another example: `strict (cols 66% cpu[3] (rows 33% mem[1] (rows net[0] proc[2])))`.

### Syntax

```
layout-string: ["strict"] layout
layout:        "(" shape [percent-fist] child child ")"
shape:         rows | cols
rows:          "rows" | "horizontal"
cols:          "cols" | "columns" | "vertical"
percent-first: <number> "%"
child:         widget | layout
widget:        widget-name "[" priority "]"
widget-name:   "cpu" | "memory" | "network" | "processes"
priority:      <number>
```


`percent-first` is the relative size of the first child of the layout (top for rows, left for columns). If omitted it defaults to 50%, if the given value is greater than 100% it becomes 100%.

The `widget-name` can be abbreviated to any length as long as it's not ambiguous (this is why the default layout-string just uses single letters).

The `priority` is used to decide which widgets to hide first when the window is too small.

If the entire layout string is prefixed by `strict`, automatic widget resizing gets disabled.

### Small window sizes

If the window is too small to fit all widgets at their preferred relative size it tries to change the relative size of widgets to fit all of them (unless the layout string starts with `strict`).

If it still can't fit all of them it hides some based on their priority (lower priority means they get hidden first).

If no widget fits it just displays `Window too small`.
