# cwe-man

`cwe-man` is a terminal TUI for browsing and reading Common Weakness Enumeration (CWE) data from a local SQLite database synchronized with the CWE REST API. It is intended for developers and security practitioners who want quick, Vim-like interaction to look up CWEs.

## Basic Usage

1. Start the app (no CLI args used):
   ```bash
   cwe-man
   ```

2. If this is your first run and the database is empty, run (this syncs the local CWE database /w data from the CWE REST API):
   ```text
   :sync
   ```

3. Core interaction modes:
   - Tree navigation: `j/k`, `{`/`}`, `o` or `Enter` to open/expand, `x`/`h` collapse, `l` open CWE.
   - Search mode: `/` then type query, `Enter` to accept.
   - Filter mode: `//` then type query, `Enter` to navigate results, `j/k/g/G`, `o`/`l` to open.
   - Command mode: `:`
   - Detail view: `j/k`, `g/G`, `q` to close.
   - Notification pane: `Ctrl+N` to toggle/focus, then `j/k`, `Enter` (read/unread), `d` (delete).

4. Prefix counts are supported for `j/k` in list-like views (for example, `10j`).

## Basic Configuration

- Config file path:
  ```text
  $HOME/.cwe-man/config.ini
  ```
- Supported keys:
  ```ini
  auto_sync_enabled=true
  auto_sync_interval_days=30
  ```
- If the file is missing or contains invalid values, defaults are used.
- Startup auto-sync runs in the background when enabled and data is stale; does not interrupt app usage and usually takes less than a minute.

## Example Workflows

### Find and open a CWE via filter
1. Launch app.
2. Press `/` then `/` to enter filter mode.
3. Type part of a CWE name or ID.
4. Press `Enter`.
5. Use `j/k` to choose a match.
6. Press `o` or `l` to open details.
7. Press `q` to close the details.
8. Press `q` to exit filter view and return to regular tree pane view.

### Open a CWE by ID
1. Press `:`
2. Run:
   ```text
   :cwe 79
   ```

### Export CWE list to Markdown
1. Press `:`
2. Run one of:
   ```text
   :print-cwes
   :print-cwes my-cwes.md
   ```

### Inspect config and API reminder popups
```text
:show-config
:rest-api
```
Close either popup with `Esc` or `q`.

### Clear runtime artifacts
1. Run:
   ```text
   :clear-runtime
   ```
2. Confirm with:
   ```text
   :yes
   ```
   or cancel with:
   ```text
   :no
   ```

## Supported Commands

- `:q` / `:quit`  
  Exit the application.

- `:sync`  
  Synchronize local data from the CWE REST API.

- `:cwe <id>`  
  Open a specific CWE directly in detail view.

- `:print-cwes [filename]`  
  Export CWE entries from the local database to a Markdown file.  
  Default output file is `cwe-list.md` in the current working directory.

- `:show-config`  
  Show configuration and runtime paths in a centered popup.

- `:rest-api`  
  Show a centered popup with CWE REST API reference details.

- `:clear-runtime`  
  Prepare runtime cleanup (logs and runtime artifacts under `$HOME/.cwe-man`), then wait for confirmation.

- `:yes`  
  Confirm pending `:clear-runtime`.

- `:no`  
  Cancel pending `:clear-runtime`.

## Build

### Requirements
- C++20 compiler
- CMake (3.20+)
- SQLite3 development package `sqlite-devel`
- libcurl development package `libcurl-devel`
- nlohmann_json `nlohmann_json-devel` or `json-devel`

[`FTXUI`](https://github.com/ArthurSonzogni/FTXUI), the TUI library dependency, is fetched automatically by CMake (`FetchContent`).

### Build Steps
```bash
# Defaults to installing in `/usr/local/bin/` on Linux systems
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target install -j
```

## Developer Guide

For architecture details, source-file mapping, and my engineering principles, see:

- [DEVELOPER.md](/home/abdullaalmosalmi/git/cwe-man/DEVELOPER.md)
