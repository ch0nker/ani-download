# ani-download
A personal tool for downloading anime.

---

## Installation

### Required Packages
- libxml
- lua
- ncurses

### Arch Linux
```bash
sudo pacman -S lua libxml2 ncurses
```

### Building
To build and install, run the install.sh script:
```bash
./install.sh
```

---

## Usage

```bash
ani-download <anime> [flags]
```

### Flags
- `-h`, `--help`: Outputs the help message.
- `-l`, `--core-list`: Lists out the available cores.
- `-c`, `--core`: Sets the core to search from. If no core is given it'll default to nyaa.

### Example
The following command will show you how to search for an anime using the core nyaa.

```bash
ani-download "clannad after story" -c nyaa
```

---

## Lua

### Guides
- [Creating a core](./docs/guides/cores.md)
- [XPath reference](https://quickref.me/xpath.html)

### Libraries
This project includes a collection of built-in Lua libraries to enhance functionality.

- [JSON](./docs/lua/json.md) - Library for encoding and decoding JSON.
- [Requests](./docs/lua/requests.md) - HTTP request handling library.
- [HTML](./docs/lua/html.md) - Library for parsing and querying HTML documents.
- [UI](./docs/lua/ui.md) - UI library that uses ncurses.
---
