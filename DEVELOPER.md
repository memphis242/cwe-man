# cwe-man Developer Guide

## High-Level Overview

`cwe-man` is a keyboard-driven terminal application for browsing CWE data from a local SQLite database, with sync support from the CWE REST API. The runtime centers on a single in-memory `AppState` object that drives rendering and event handling across panes and modes. Data retrieval and persistence are isolated in API and repository/database layers so UI code can stay focused on interaction behavior.

## Architecture

- `app/` owns lifecycle, startup configuration, command dispatch, and background sync orchestration.
- `ui/` renders panes and handles key events/mode transitions, using `AppState` as the shared state contract.
- `api/` encapsulates network access and sync flows against the CWE REST API.
- `data/` encapsulates schema, queries, and typed data models over SQLite.
- `logging/` provides file logging and critical syslog forwarding.
- `main.cpp` wires process-level exception handling and starts the app.

## Source Map (`src/`)

### app

#### `src/app/App.hpp`
This file declares the top-level `App` class that owns process lifetime and application orchestration. It defines private helpers for configuration loading, sync control, popup data population, runtime cleanup, and markdown export. It is the main integration point between state, repository access, and UI loop startup.

Main Components:
- `class App`
- `App::App()`
- `App::run()`

#### `src/app/App.cpp`
This file implements `App` construction, startup loading, command-mode behavior, and screen loop integration. It also implements config parsing from `~/.cwe-man/config.ini`, startup auto-sync checks, and command helpers (`show-config`, `rest-api`, `clear-runtime`, `print-cwes`). Background sync is dispatched on a detached thread and posts status updates back to the interactive screen.

Main Components:
- Public method implementations for `cweman::App` (from `App.hpp`)
- No standalone public free functions (implementation-only translation unit)

#### `src/app/State.hpp`
This file defines the full UI/runtime state model shared across app and UI layers. It includes mode flags, tree/search/filter state, command buffer and completions, notification selection, popup state, and prefix-count/last-key tracking. The structure is intentionally plain-data style to simplify rendering and event mutation logic.

Main Components:
- `enum class AppMode`
- `struct AppState`

#### `src/app/State.cpp`
This file is intentionally minimal and currently contains no behavior beyond namespace anchoring. It exists as a placeholder for future state-related methods if the plain-data model evolves. Keeping this file allows clean expansion without reorganizing build structure later.

Main Components:
- No public API surface (placeholder implementation unit for `AppState`)

### api

#### `src/api/CweClient.hpp`
This file declares the HTTP client abstraction over libcurl for CWE REST API requests. It exposes narrow endpoint methods (`version`, `view`, `category`, `weaknesses`) and hides raw CURL handle details behind a `void*` member. It also sets the canonical API base URL constant used by sync logic.

Main Components:
- `class CweClient`
- `CweClient::get_version()`
- `CweClient::get_view(int)`, `get_category(int)`, `get_weaknesses(const std::string&)`

#### `src/api/CweClient.cpp`
This file implements request execution, curl global initialization, and response/error handling for API calls. It centralizes logging for outbound requests and response payloads, and contains tolerances for known API connection-close behavior after successful responses. Endpoint-specific methods delegate to a single internal `get(path)` implementation.

Main Components:
- Public method implementations for `cweman::CweClient` (from `CweClient.hpp`)
- No standalone public free functions (implementation-only translation unit)

#### `src/api/Sync.hpp`
This file declares sync orchestration over API client plus repository persistence. It defines progress/completion callback types so callers can report status without coupling sync internals to UI components. It also exposes static helpers for reading/writing/staleness checks of sync timestamp state.

Main Components:
- `using SyncProgressCallback`
- `using SyncCompleteCallback`
- `class Sync` with `run(...)`, `read_sync_timestamp()`, `write_sync_timestamp()`, `sync_needed(...)`

#### `src/api/Sync.cpp`
This file implements end-to-end sync: fetch view metadata, iterate categories, batch-fetch weaknesses, and persist transformed records. It serializes sync state timing to `~/.cwe-man/sync_state.txt` and evaluates staleness windows for startup auto-sync decisions. Parsing and transformation are performed with `nlohmann::json` and persisted through `Repository` APIs.

Main Components:
- Public method implementations for `cweman::Sync` (from `Sync.hpp`)
- No standalone public free functions (implementation-only translation unit)

### data

#### `src/data/Models.hpp`
This file defines the shared domain data structures used across repository, UI, and app state layers. It contains category/CWE data, consequence and mitigation structures, notification types, sync metadata, and flattened tree node representation. The models intentionally remain lightweight POD-style types with minimal behavior.

Main Components:
- `struct Category`, `Consequence`, `Mitigation`, `Cwe`, `CwePrintRow`
- `enum class NotificationSeverity`
- `struct Notification`, `SyncState`, `TreeNode`

#### `src/data/Database.hpp`
This file declares the low-level SQLite access wrappers with RAII semantics. `Statement` provides typed bind/step/column access and reset helpers, while `Database` owns the connection and utility execution methods. It also defines `DatabaseError` for consistent error propagation from database operations.

Main Components:
- `class DatabaseError`
- `class Statement`
- `class Database`

#### `src/data/Database.cpp`
This file implements prepared statement lifecycle, typed bind/access operations, and database connection setup. Connection initialization enforces directory creation, WAL mode, and foreign key activation before use. Errors from SQLite operations are converted to `DatabaseError` exceptions with contextual messages.

Main Components:
- Public method implementations for `cweman::Statement` and `cweman::Database` (from `Database.hpp`)
- No standalone public free functions (implementation-only translation unit)

#### `src/data/Repository.hpp`
This file declares the repository abstraction that maps domain operations to SQL-backed persistence. It exposes categories/CWEs retrieval and upsert operations, export-oriented read APIs, and notification CRUD operations. It is the primary data-layer contract consumed by app and UI logic.

Main Components:
- `class Repository`
- Category/CWE public APIs: `get_categories`, `get_cwe`, `search_cwes`, `upsert_*`, `link_cwe_category`, `get_cwes_for_print`
- Notification public APIs: `get_notifications`, `insert_notification`, `mark_notification_read`, `set_notification_read`, `delete_notification`

#### `src/data/Repository.cpp`
This file implements schema creation and all repository SQL read/write behavior over `Database`. It includes JSON (de)serialization for structured CWE fields, export query assembly, and notification row mapping. It keeps SQL and domain-shaping logic centralized so upper layers do not embed persistence details.

Main Components:
- Public method implementations for `cweman::Repository` (from `Repository.hpp`)
- No standalone public free functions (implementation-only translation unit)

### logging

#### `src/logging/Logger.hpp`
This file declares the global logger facade used across modules. It exposes severity methods and internally owns file and syslog sink implementations. The singleton-style access pattern is provided through `Logger::instance()`.

Main Components:
- `class Logger`
- `Logger::instance()`
- `Logger::{debug,info,warn,error,critical}(std::string_view)`

#### `src/logging/Logger.cpp`
This file implements logger initialization and routes severity events to configured sinks. File logging is used for all levels, while critical messages are additionally sent to syslog. Log directory resolution is performed relative to `$HOME/.cwe-man/logs`.

Main Components:
- Public method implementations for `cweman::Logger` (from `Logger.hpp`)
- No standalone public free functions (implementation-only translation unit)

#### `src/logging/FileSink.hpp`
This file declares file-based logging with daily rotation support. It provides a single `write(level, message)` API and encapsulates file stream lifecycle and mutex protection. Rotation and path selection are kept internal to the sink.

Main Components:
- `class FileSink`
- `FileSink::write(std::string_view, std::string_view)`

#### `src/logging/FileSink.cpp`
This file implements timestamp formatting, date-based rotation checks, and append-mode log writes. It uses a mutex to serialize writes and protect stream rotation boundaries. Log files are named per day using the `cwe-man-YYYY-MM-DD.log` pattern.

Main Components:
- Public method implementations for `cweman::FileSink` (from `FileSink.hpp`)
- No standalone public free functions (implementation-only translation unit)

#### `src/logging/SyslogSink.hpp`
This file defines a minimal syslog sink used for critical-level forwarding. It opens/closes syslog in constructor/destructor and provides a direct write helper. The class is header-only and intentionally small.

Main Components:
- `class SyslogSink`
- `SyslogSink::write(std::string_view)`

### ui

#### `src/ui/TreePane.hpp`
This file declares tree-pane rendering and visible-node rebuilding utilities. Rendering is exposed as a pure `Element` function, while node flattening is a state-mutating helper. It is the narrow tree-pane interface used by layout and app startup flows.

Main Components:
- `ftxui::Element RenderTreePane(AppState&, bool)`
- `void rebuild_visible_nodes(AppState&, Repository&)`

#### `src/ui/TreePane.cpp`
This file implements flattened tree construction from categories and expanded-state sets. It also renders normal tree and active-filter tree views, including cursor highlighting and search match emphasis. The implementation keeps pane-specific visual behavior isolated from global event routing.

Main Components:
- Public function implementations for `RenderTreePane(...)` and `rebuild_visible_nodes(...)`
- No standalone public classes/types in this translation unit

#### `src/ui/DetailPane.hpp`
This file declares detail-pane rendering as a stateless function over `AppState` plus scroll position. It is intentionally function-based (not component-class-based) to keep integration simple in the root layout renderer. The interface includes focus awareness for dim/non-dim visual behavior.

Main Components:
- `ftxui::Element RenderDetailPane(AppState&, int&, bool)`

#### `src/ui/DetailPane.cpp`
This file implements detail-pane content formatting, word wrapping, and vertical scrolling behavior. It derives pane width from terminal size and renders sectioned CWE metadata/content with a footer hint line. Scroll bounds are clamped each render cycle to keep keyboard navigation stable.

Main Components:
- Public function implementation for `RenderDetailPane(...)`
- No standalone public classes/types in this translation unit

#### `src/ui/Layout.hpp`
This file declares the top-level UI layout component factory that binds state, repository, and command callbacks together. It defines `CommandCallback` as the command-mode handoff contract into the app layer. The header is intentionally small because most behavior lives in `Layout.cpp`.

Main Components:
- `using CommandCallback`
- `ftxui::Component RootLayout(AppState&, Repository&, CommandCallback)`

#### `src/ui/Layout.cpp`
This file implements root rendering composition, pane layout, popup overlay, and nearly all key event handling across modes. It contains search/filter matching logic, command completion behavior, prefix-count handling, notification interactions, and mode transition rules. It is effectively the interaction state machine for the TUI.

Main Components:
- Public function implementation for `RootLayout(...)`
- No standalone public classes/types in this translation unit

### entrypoint

#### `src/main.cpp`
This file provides the process entrypoint that constructs and runs `cweman::App`. It wraps startup in a top-level exception boundary and reports fatal failures to both logger and stderr. It keeps executable bootstrap responsibilities minimal and defers all behavior to `App`.

Main Components:
- `int main()`

## Coding and Testing Philosophy

`cwe-man` follows these directive documents as the canonical standard for engineering quality and performance priorities:

- [SOFTWARE-QA-DIRECTIVES.md](./SOFTWARE-QA-DIRECTIVES.md)
- [PERFORMANCE-DIRECTIVES.md](./PERFORMANCE-DIRECTIVES.md)
- [SECURITY-PLAN.md](./SECURITY-PLAN.md)

Core directive themes:
- Robustness and correctness first: assertions, strict warnings, static analysis, sanitizer usage, and broad behavioral coverage.
- Layered validation goals: unit, integration, fuzz, and manual/agent-assisted security and best-practice review.
- Performance within quality constraints: benchmark-driven function choices, data-structure selection by measured behavior, and reduced step/jump complexity at architecture level.
- Encapsulation and orthogonality: modules should keep internal details private and expose focused APIs.
