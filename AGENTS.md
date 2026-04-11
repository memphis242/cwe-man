# cwe-man Agent Guide

## Purpose
- `cwe-man` is a C++ terminal TUI for browsing CWE data synced from the CWE REST API into a local SQLite database.
- Primary UX is Vim-like keyboard navigation across panes and modes.

## Runtime and Storage
- App data directory: `$HOME/.cwe-man/`.
- Local SQLite database: `$HOME/.cwe-man/cwe.db`.
- App logs are stored under the same app data root.
- TUI library: `ftxui`.

## Mode and Keybinding Contract
- `Tree` mode:
  - Navigate tree rows with `j/k`, category jumps with `{` and `}`.
  - Open/expand with `o` or `Enter`.
  - Collapse with `x` or `h` (context-dependent).
  - Open detail with `l` on CWE rows.
- `Detail` mode:
  - Scroll with `j/k`, `g/G`, `Ctrl+U` and `Ctrl+D`.
  - Close detail with `q`, or return to tree focus with `h`.
- `Search` mode (`/` from tree):
  - Live-searches visible tree nodes.
  - `Enter` accepts search and returns to tree with matches retained.
  - Empty search + `/` transitions to filter mode.
- `Filter` mode (`//` from tree):
  - Renders only matching CWE rows (no categories).
  - Input phase: typing updates filter query live.
  - `Enter` with non-empty query switches to navigation phase.
  - Navigation phase supports `j/k/g/G/o`.
  - `/` in navigation phase returns to input phase.
  - `Esc` clears filter and returns to normal tree mode.
  - `Enter` with empty query exits filter mode.
- `Command` mode (`:` from tree/detail):
  - Captures command-line input with autocomplete.

## Command Contract
- `:q` / `:quit` exits app.
- `:sync` triggers API sync and refreshes loaded data.
- `:cwe <id>` opens a specific CWE in detail pane.
- `:print-cwes [filename]` exports local CWE list to markdown.
  - Default output path: `./cwe-list.md`.
  - Existing file is overwritten.
  - Output is grouped by category (category heading + bullets).
  - Rows are ordered by category name then CWE ID.
  - CWE entries may repeat across categories by design.
  - Each row includes `CWE-ID`, name, first line of description, and URL link if present.

## Data and UI Expectations
- Tree pane renders category headers + CWE leaves in normal tree mode.
- Filter mode always renders CWE-only rows from `visible_nodes` by `filter_matches`.
- Selection must be visually highlighted in both tree and filter views.
- Status/bottom bar must clearly show active mode/query context.

## Acceptance Criteria (Regression Checklist)
- `//` filter flow:
  - User can type query, press `Enter`, then navigate matches with `j/k`.
  - User can open selected result with `o`.
  - Filter footer does not remain in input-cursor state after `Enter`.
  - `Esc` exits filter cleanly to tree.
- `:print-cwes`:
  - Works with and without filename argument.
  - Produces valid markdown file in expected location.
  - Uses first line of description as summary.
  - Includes links when URL exists.
  - Overwrites existing target file.
- Existing behaviors (`:sync`, `:cwe <id>`, tree/detail navigation) remain functional.
