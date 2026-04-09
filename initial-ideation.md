# cwe-man: Initial Ideation Chat

## Overview

This document captures the initial ideation conversation that shaped the design of `cwe-man`, a C++20 terminal UI application for browsing the MITRE Common Weakness Enumeration (CWE) database.

---

## Conversation

### User

I'm a terminal junkie, and prefer to work in the terminal whenever possible. Presently, I find myself opening the Common Weakness Enumeration website's page to look up the CWEs under Software Development: https://cwe.mitre.org/data/definitions/699.html. There are many - 399 at the moment, across 40 categories. I'd prefer to have a CLI application to quickly search through the categories and specific CWEs under this Software Development subset. In fact, I'd like all the CWEs (944 at the moment) to be as easily accessible. Right now, I'm imagining this application appearing starting up somewhat like Vim or a man page, but displaying a folder-like tree structure with collapsible sections (just like NerdTREE, the popular Vim plugin). I'd like to use the Vim key bindings {h,j,k,l} to move the cursor around the tree, and '{' / '}' to move between categories. I'd also like 'o' to open a specific CWE or expand a category, and 'x' to collapse a category. Once the view of a CWE opens, I'd like the tree to still appear as a left-hand side-pane, which may be hidden from view or brought back with Ctrl+T. Lastly, with the tree pane / view active, I'd like to be able to '/' search to easily find a category/CWE and move my cursor to it. I'd also like to support a ':' command from any view to simply open a specific CWE, with auto-complete capability. Finally, I'd want this application to periodically (or on command) retrieve the CWE list using CWE's REST API: https://github.com/CWE-CAPEC/REST-API-wg/blob/main/Quick%20Start.md. Those are the high level ideas for my application. Before we start generating anything, interview me a little to help clarify my expectations to you.

### Assistant (Interview Round 1)

Great vision! This is a solid set of requirements. Before we start building, I have some clarifying questions to help shape the implementation:

**Technology & Architecture**

1. **Language preference** — Do you have a preference for the implementation language? (Rust, Go, Python, TypeScript/Node, etc.) This affects available TUI libraries and performance characteristics.

2. **Data storage** — How should the CWE data be persisted locally between runs?
   - Simple JSON files in a `.cwe-man/` directory?
   - SQLite database?
   - Something else?

3. **TUI library** — Are you open to using an existing terminal UI framework, or do you have preferences? (e.g., `ratatui` for Rust, `bubbletea` for Go, `rich` for Python)

**Feature Scope & MVP**

4. **MVP vs nice-to-have** — Of these features, which are essential for v1?
   - Tree navigation with collapsible categories
   - Vim keybindings (h/j/k/l, {/}, o, x)
   - CWE detail view with side pane
   - Search functionality (/)
   - Command mode with `:cwe-id` autocomplete
   - REST API sync capability

5. **CWE detail view** — When viewing a CWE, what information should display?
   - Just name/ID and description?
   - Full details (severity, related weaknesses, consequences, etc.)?
   - Links to external resources?

6. **Search behavior** — Should `/` search match:
   - CWE IDs only (CWE-123)?
   - Names and descriptions?
   - Category names?

**Data & Updates**

7. **CWE data scope** — Start with just Software Development (699) or fetch all 944 CWEs from day one?

8. **API sync strategy** — How should data sync work?
   - Manual command like `:update`?
   - On startup?
   - Background periodic sync?
   - With timestamps to avoid re-fetching?

9. **Filtering/Views** — Beyond tree navigation, do you want ability to filter by severity, type, or other dimensions?

---

### User (Answers Round 1)

1. Language Preference: Since I'm learning C++, I'd like you to use that, and specifically, C++20.
2. Data Storage: Local SQLite database, with an option to export DB to a JSON file. Persist it in the home directory of the user, in $HOME/.cwe-man/.
3. Yes, definitely use the most popular TUI framework for C++.
4. MVP: Basic tree navigation with all the described key bindings. Then we can prioritize the collapsibility of the "directories" (top-level view/category in the tree). Then the '/' search. Then the ':' command. Then the REST API sync-up.
5. CWE detail view: Full details, similar to what the webpage on the CWE website would show.
6. Search behavior: CWE ID or its title.
7. CWE data scope: Start with just Software Development, then we can graduate to all 944 CWEs.
8. Syncing should occur from manual command - ':sync' - and through a configurable periodicity (default to once a month). Yes, time-stamping would be necessary, probably in a persistant file in the $HOME/.cwe-man/ directory. Which reminds me, at some point, I might want to have a simple config file for this application, but that's the lowest priority.
9. Filtering/Views: Filtering is a great idea! Let's add that as one of the lower priority features.

---

### Assistant (Interview Round 2)

Perfect! That's very clear. A few final clarifications before we start:

**TUI & Exit Behavior**

1. **TUI Framework** — For C++, I'd recommend **ftxui** (modern, actively maintained, clean API). Does that work for you, or do you have another preference?

2. **Exit behavior** — How should the user exit the app?
   - `q` key?
   - `ESC` key?
   - Both?

**Architecture & Learning**

3. **Your C++ experience** — As you're learning C++, how deep should I go on explaining architectural choices? Should I aim for educational clarity over "production-perfect" code?

4. **Build system** — Preference for CMake, Meson, or something else? (CMake is most common)

5. **Dependency management** — Should we use vcpkg, Conan, or git submodules to manage dependencies (ftxui, SQLite, etc.)?

**Initial State & Errors**

6. **First run** — When the database doesn't exist:
   - Should the app automatically fetch from the REST API?
   - Or prompt the user to run `:sync` first?
   - Or just show an empty tree?

7. **Error display** — How should errors surface? (e.g., network failures during sync)
   - Toast-style notification at bottom?
   - Modal popup?
   - Status line message?

8. **Relative dates** — For the periodic sync (default monthly), should the timestamp be stored as Unix time or human-readable format in the config file?

---

### User (Answers Round 2)

1. What TUI framework options are there?
2. Exit behavior: q to exit out of a CWE view, ':q' to exit out of the app.
3. 100% prioritize production-perfect code. I'm reasonably experienced with C++, although I may need a refresher here-and-there.
4. Build system: CMake.
5. Dependency management: I'll have to explore this more with you. Definitely not vcpkg, or git submodules, but I'm unfamiliar with Conan. Part of me is tempted to just specify these as system dependencies to start off (so CMake simply finds them after installation), and at some point later on, figure out how to isolate the dependencies better.
6. First run: Prompt the user to run :sync first.
7. Error display: Model popup that also gets saved in a hide-able pane on the right for later review as needed. Also, let's discuss logging for troubleshooting; I'm personally a fan of syslog at the moment, but I also wouldn't be opposed to a log in the app's user folder.
8. Relative dates: Human-readable format.

---

### Assistant (TUI Options + Logging Questions)

**TUI Framework Options**

For C++, the main contenders are:

1. **ftxui** (recommended) — Modern, clean API, drawing-based, excellent for keybinding-heavy UIs like yours, actively maintained
2. **ncurses** — Classic, universally available, but lower-level and more verbose
3. **notcurses** — Modern ncurses alternative, more features than ncurses
4. **ImGui** — Usually desktop-focused, but terminal backends exist (less common)

Recommended: **ftxui**. Its declarative API maps well to Vim-like keybindings and the tree structure you want.

**Logging Strategy Questions**

1. **Syslog vs app logs** — Should we:
   - Log to syslog only?
   - Log to both syslog (critical errors) + a `.cwe-man/logs/` directory (detailed app logs)?
   - Log to app logs only (simpler)?

2. **Log verbosity levels** — Standard (DEBUG, INFO, WARN, ERROR)? Or something else?

3. **Error pane persistence** — When the user closes the error modal:
   - Should the error persist in the right-side pane until manually cleared?
   - Or auto-clear after N seconds?
   - Or only show in the pane when the user explicitly opens it?

4. **Sensitive data** — Should logs exclude API responses or raw data to avoid leaking CWE content?

---

### User (Final Answers)

Alright, let's go with ftxui. As for logging, let's do syslog for critical errors and a .cwe-man/logs/ with detailed, debug level logs. For the error pane persistence, the errors should always be present, with an option to manually delete. I'd also like there to be a "read"/"unread" UI, similar to email for it. Let's also change the semantics from error pane to a notification pane. Concerning sensitive data, let's log the full API response.

---

## Decisions Summary

| Concern | Decision |
|---|---|
| Language | C++20 |
| Build system | CMake |
| TUI library | ftxui (via CMake FetchContent) |
| Database | SQLite at `~/.cwe-man/cwe.db` |
| DB export | JSON export option |
| HTTP client | libcurl |
| JSON parsing | nlohmann/json (`json-devel` on Fedora) |
| Logging - critical | syslog |
| Logging - debug | File logs at `~/.cwe-man/logs/cwe-man-YYYY-MM-DD.log` |
| Log API responses | Yes, full responses at DEBUG level |
| Notification pane | Persistent, read/unread like email, manual delete |
| Error display | Modal popup + persisted in notification pane |
| Dependency mgmt | System packages via CMake `find_package`; ftxui via FetchContent |
| First run | Prompt user to run `:sync` |
| Exit behavior | `q` exits CWE detail view; `:q` exits app |
| Tree toggle | Ctrl+T |
| Notification pane toggle | Ctrl+N |
| Sync command | `:sync` |
| Sync periodicity | Configurable, default monthly |
| Sync timestamp | Human-readable ISO 8601 in `~/.cwe-man/sync_state.txt` |
| Initial CWE scope | Software Development view (699), then all 944 |
| Config file | Lowest priority future feature |
| Filtering | Lower priority future feature |

## MVP Feature Priority

1. Basic tree navigation with all key bindings (h/j/k/l, {/}, o, x)
2. Collapsible categories in tree
3. `/` search (by CWE ID or title)
4. `:` command mode with autocomplete
5. REST API sync (`:sync` + periodic)
6. Filtering (lower priority)
7. Config file (lowest priority)

## Key Bindings

| Key | Context | Action |
|---|---|---|
| h/j/k/l | Tree pane | Vim movement |
| { / } | Tree pane | Jump between categories |
| o | Tree pane | Expand category or open CWE |
| x | Tree pane | Collapse category |
| / | Tree pane | Search by ID or title |
| : | Any | Enter command mode |
| q | Detail view | Return to tree |
| :q | Any | Quit application |
| :sync | Command mode | Trigger API sync |
| :cwe \<id\> | Command mode | Open specific CWE (with Tab autocomplete) |
| Ctrl+T | Any | Toggle tree pane |
| Ctrl+N | Any | Toggle notification pane |
| d | Notification pane | Delete notification |
| Enter | Notification pane | Expand/read notification (marks as read) |
