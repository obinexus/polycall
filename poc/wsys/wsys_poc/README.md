# WSYS Terminal Editor

## Overview

WSYS Terminal Editor is a terminal-based text editor built using the Textual framework. It is designed around two core philosophies:

* **WSYSMI** - What You See is Structure/Meta Interface
* **WSYSGI** - What You See is Gettable Interface

These principles ensure that every visible UI element reflects its internal structure and can be interacted with programmatically. The editor displays real-time metadata like cursor position and current time, providing structural awareness and interactive scripting capabilities.

## Features

* ?? Editable text area with live updates
* ?? Real-time display of cursor position (line, column)
* ?? Live clock updates every second
* ?? Clean modular design using Textual's widget system
* ?? Robust and extendable layout

## Installation

```bash
pip install textual
```

## Usage

Run the editor with:

```bash
python wsys_editor.py
```

> **Note:** This project is intended to be run in a full-featured terminal. Some environments (e.g., online sandboxes) may not support required terminal features.

## Project Structure

```
wsys_editor.py      # Main script with editor logic
README.md           # This documentation
```

## Limitations

* May not run in environments that lack terminal support (e.g., `termios` module).
* Textual is still under development and may change APIs.

## License

MIT License

---

Built for devs who love structured editing and smart interfaces ????

