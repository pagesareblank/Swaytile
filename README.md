# Swaytile

**Swaytile** is a lightweight autotiling daemon for [Sway](https://swaywm.org/), written in C.

It dynamically adjusts split directions as you open and arrange windows, while also providing commands for switching a workspace between different layout modes.

## Features

* Dynamic autotiling for Sway
* Written in C with minimal dependencies
* Per-workspace layout control
* Toggle between dynamic autotiling and tabbed layouts
* Force a workspace into tabbed or stacked mode
* Limit split-direction changes per workspace
* Verbose logging for debugging
* Simple command-line interface
* Builds with a standard `Makefile`

## Installation

Clone the repository:

```bash
git clone https://github.com/pagesareblank/Swaytile.git
cd Swaytile
```

Build:

```bash
make
```

This produces the `swaytile` executable.

You can then install it system-wide if desired:

```bash
sudo make install
```

> **Note:** The exact installation location depends on the project's `Makefile`.

## Usage

```text
Usage: swaytile [OPTIONS]

Options:
  -t, --toggle <MODE>   Change or toggle workspace layout mode:
                        - toggle  : Toggle between tabbed and dynamic autotiling
                        - tabbed  : Flatten windows into a single tabbed row
                        - stacked : Flatten windows into a single stacked column
                        - split   : Restore dynamic autotiling
  -w, --workspace <WS>  Limit autotiling to specific workspace(s)
  -l, --limit <NUM>     Cap the number of split direction changes per workspace
  -v, --verbose         Enable verbose logging output to stderr
  -h, --help            Display this help message and exit
```

### Examples

Start Swaytile normally:

```bash
swaytile
```

Enable verbose logging:

```bash
swaytile --verbose
```

Restrict autotiling to a workspace:

```bash
swaytile --workspace 1
```

Set a workspace to tabbed mode:

```bash
swaytile --toggle tabbed
```

Set a workspace to stacked mode:

```bash
swaytile --toggle stacked
```

Return to dynamic autotiling:

```bash
swaytile --toggle split
```

Toggle between tabbed and dynamic autotiling:

```bash
swaytile --toggle
```

Limit the number of split-direction changes:

```bash
swaytile --limit 3
```

## Sway Configuration

Swaytile is intended to run as a background daemon alongside Sway.

For example, you can start it from your Sway configuration:

```ini
exec swaytile
```

You can also bind the layout controls to Sway keybindings:

```ini
bindsym $mod+t exec swaytile --toggle
bindsym $mod+Shift+t exec swaytile --toggle tabbed
bindsym $mod+Shift+s exec swaytile --toggle stacked
bindsym $mod+Shift+d exec swaytile --toggle split
```

Adjust the keybindings to your own configuration.

## Layout Modes

### Dynamic

The default mode dynamically chooses split directions as windows are added, providing an automatic tiling layout without manually selecting horizontal or vertical splits.

### Tabbed

```bash
swaytile --toggle tabbed
```

Flattens the current workspace into a single tabbed row.

### Stacked

```bash
swaytile --toggle stacked
```

Flattens the current workspace into a single stacked column.

`stacking` may also be used as an alias for `stacked`.

### Split

```bash
swaytile --toggle split
```

Restores dynamic autotiling.

`default` may also be used as an alias for `split`.

## Workspace Selection

Autotiling can be restricted to specific workspaces using:

```bash
swaytile --workspace <WORKSPACE>
```

This is useful when you want Swaytile to manage only selected workspaces while leaving others under manual control.

## Split Limit

The `--limit` option allows you to cap how many split-direction changes Swaytile can make on a workspace:

```bash
swaytile --limit 3
```

This can help prevent layouts from becoming overly fragmented as more windows are opened.

## Verbose Logging

For debugging or troubleshooting:

```bash
swaytile --verbose
```

Verbose output is written to `stderr`, making it possible to redirect or inspect logs separately from normal program output.

## Building

Swaytile uses a `Makefile` for compilation.

Build the project with:

```bash
make
```

Clean build artifacts with:

```bash
make clean
```

If your `Makefile` provides an install target:

```bash
sudo make install
```

## Requirements

* Linux
* [Sway](https://swaywm.org/)
* A C compiler
* `make`

## Why Swaytile?

Sway already provides powerful manual control over window layouts. Swaytile is designed for users who prefer an **automatic layout** while still wanting the ability to take control when necessary.

The daemon handles the repetitive work of choosing split directions, while the command-line interface makes it easy to temporarily switch a workspace to another layout.

## License

See the `LICENSE` file for licensing information.

## Contributing

Contributions, bug reports, and suggestions are welcome.

If you find a bug or have an idea for improving Swaytile, feel free to open an issue or submit a pull request.

---

**Swaytile — automatic tiling for Sway, written in C.**
