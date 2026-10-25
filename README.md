# DLX Solver – JavaScript frontend + C++ (Drogon) backend

A Sudoku / exact-cover solver UI. The browser frontend is the original plain-JavaScript app; the solver runs in a
C++20 server (Drogon) that implements Knuth's Algorithm X with Dancing Links and talks to the frontend over a JSON REST API.

## What the migration found (please read)

The uploaded `frontend.zip` contained **only the browser/renderer side**: `index.html`, `styles.css`, `renderer.js`,
`js/*.js` and a Node test script. There was **no `package.json`, no Node/Express server, no Electron main process or
preload script, no database, no authentication, no file handling and no environment configuration**. Browser-side
JavaScript (all of it) was kept as is.

The frontend did define the backend it expects. `js/solver-client.js` called `window.solver.getStatus()` and
`window.solver.solvePuzzle(grid)` (an Electron preload bridge that was not part of the ZIP) and normalised results to
`solved | no-solution | invalid | error`, with statistics `executionMs / recursiveCalls / nodesVisited / backtracks / solutionCount`.
So there was no JavaScript backend source to port; **the C++ backend implements that contract**, which is the only
backend behaviour the code revealed. Nothing was invented beyond it (no database, no login), because the app has none.

| Original (from the ZIP) | Now |
|---|---|
| `window.solver.getStatus()` (missing preload) | `GET /api/status` |
| `window.solver.solvePuzzle(grid)` (missing main process) | `POST /api/solve` |
| Result contract `solved/no-solution/invalid/error` + stats | Same JSON shape, produced by C++ |
| Error code `BACKEND_NOT_CONNECTED` → "Solver not connected" | Kept; produced when the server is unreachable |
| Node.js server files / backend-only npm dependencies | None existed, so nothing was removed |

### Changes made to the frontend (everything else is untouched)

* `js/solver-client.js` – rewritten to use `fetch` against the C++ API (timeout, network-failure handling, same normalised results; the client still rejects a "solution" that contradicts the clues).
* `js/config.js` (new) – `apiBaseUrl` and `requestTimeoutMs`.
* `index.html` – loads `config.js`; CSP gained `connect-src 'self' http://127.0.0.1:8080 http://localhost:8080` (without it the browser would block every request, because the CSP started from `default-src 'none'`).
* `renderer.js` – after each solve, the header "Connected / Not connected" indicator is refreshed (so starting the backend after the page loaded is noticed); one comment updated.
* `js/status.js` – the "not connected" message no longer says "yet".
* `tests/run-tests.js` – 10 added tests for the new client (original 18 untouched).

## Directory structure

```
dlx-solver/
├── README.md  .env.example  .gitignore  package.json
├── docs/API.md                      HTTP API reference
├── frontend/                        original app (plain JS, no build step)
│   ├── index.html  styles.css  renderer.js
│   ├── js/…  (config.js and solver-client.js changed, see above)
│   └── tests/run-tests.js
├── backend/                         C++20 / Drogon / CMake
│   ├── CMakeLists.txt  CMakePresets.json
│   ├── src/
│   │   ├── main.cpp                 startup, config, wiring
│   │   ├── Config.{h,cpp}           environment-variable configuration
│   │   ├── AppContext.h             read-only process state
│   │   ├── solver/                  DlxSolver (Algorithm X + Dancing Links), Types
│   │   ├── service/                 GridValidation, SudokuService, ResultMapper (business logic + JSON shape)
│   │   ├── controllers/             SolverController (HTTP handlers)
│   │   └── http/                    CorsPolicy, Middleware (CORS, 404/405 JSON, headers), Responses
│   └── tests/                       unit_tests.cpp (23 cases)
├── scripts/                         build-backend.sh run.sh test-api.sh test-ui.sh
├── tests/integration/api.test.js    end-to-end HTTP tests (Node)
├── tests/ui/ui-smoke.test.js        UI test in jsdom
└── .vscode/                         tasks, LLDB launch config, recommended extensions
```

**Database:** none is used, so there is no schema or migration. The solver is stateless and the app's run history lives
in the browser, exactly as before. **Authentication:** none existed; the server binds to `127.0.0.1` by default.

## Setup on macOS (Apple Silicon) with VS Code

```bash
# 1. Tools and dependencies (Homebrew). Drogon pulls in jsoncpp, openssl, zlib, etc.
xcode-select --install            # skip if the command line tools are already installed
brew install cmake ninja drogon node

# 2. Configuration (optional; defaults work)
cp .env.example .env

# 3. Build the backend and run its unit tests
scripts/build-backend.sh

# 4. Start the backend; it also serves the frontend
scripts/run.sh
# open http://127.0.0.1:8080
```

Node.js is only needed for the frontend tests and the API/UI test scripts, not to run the app.
VS Code: open the folder, install the recommended extensions (CMake Tools, CodeLLDB), use **Terminal → Run Build Task**
to build, **Run → Debug backend (LLDB)** to debug. CMake Tools picks up `backend/CMakePresets.json` (`macos-arm64`, `default`, `debug`).

On Linux (Debian/Ubuntu): `sudo apt install build-essential cmake ninja-build libdrogon-dev libjsoncpp-dev uuid-dev zlib1g-dev libssl-dev`.
Some distro Drogon packages were built with optional database drivers and need their dev packages at configure time
(on Ubuntu 24.04 I additionally needed `libpq-dev libsqlite3-dev libmariadb-dev libhiredis-dev libyaml-cpp-dev libbrotli-dev libc-ares-dev`).

## Running

| What | Command |
|---|---|
| Build backend (+ unit tests) | `scripts/build-backend.sh` |
| Start backend **and** frontend (single origin) | `scripts/run.sh` → http://127.0.0.1:8080 |
| Start backend only (API) | `DLX_STATIC_DIR= backend/build/dlx_server` |
| Serve the frontend separately | `python3 -m http.server 3000 --directory frontend` → http://localhost:3000 (backend must allow it; `http://localhost:3000` is in the default `DLX_CORS_ORIGINS`) |
| Manual build | `cmake -S backend -B backend/build -G Ninja && cmake --build backend/build` (or `cmake --preset macos-arm64` inside `backend/`) |

## Connecting the frontend to the backend

* **API address:** edit `apiBaseUrl` in `frontend/js/config.js` (default `http://127.0.0.1:8080`). If you use a
  different origin, also add it to `connect-src` in the CSP `<meta>` of `frontend/index.html` (the CSP is
  enforced by the browser, so this cannot be changed at runtime).
* **Port:** `DLX_PORT` in `.env`; then update `config.js` and the CSP to match.
* **CORS:** the backend adds CORS headers only for origins in `DLX_CORS_ORIGINS`. Serving the frontend from the backend
  (`scripts/run.sh`) is same-origin and needs no CORS at all. For `file://` pages or Electron add `null` to the list.
* **Secrets:** the app has none. All configuration is environment variables; `.env` is git-ignored.

## Tests

| Suite | Command | Covers |
|---|---|---|
| C++ unit tests (23 cases) | `ctest --test-dir backend/build --output-on-failure` | solver (known puzzles, hard puzzle, multiple/no solutions, limits, reuse), validation, config, CORS policy, JSON contract |
| Frontend logic (28) | `node frontend/tests/run-tests.js` | original 18 tests + the new HTTP client |
| API end-to-end (12) | `scripts/test-api.sh` | starts the real server; 30 puzzles from the frontend's generator solved through the real client and compared with the generator's answer keys; error codes; CORS; concurrency |
| UI smoke (4) | `npm install && scripts/test-ui.sh` | real `index.html` + scripts in jsdom: page loads, Connected indicator, Solve button end to end, backend-down handling |

## Migrated features

* Solve a puzzle (Algorithm X / Dancing Links) with first solution, uniqueness detection and real search statistics.
* Backend status / connectivity indicator.
* Result handling: solved, no-solution, invalid clues, error, backend not reachable, timeout.
* Input validation on the server (shape, range, duplicate clues) in addition to the frontend's own checks.

Everything else in the app (grid editing, notes, undo/redo, example generator, Visualize / algorithm replay, step log, export,
themes) was already pure browser JavaScript and is unchanged.

## Verification status (honest report)

Run in the build environment (Ubuntu 24.04, x86-64, GCC 13.3, CMake 3.28, **Drogon 1.8.7**, Node 22):

* Clean build with `-Wall -Wextra -Wpedantic`: no warnings. 23/23 C++ unit tests pass, also under AddressSanitizer + UBSan. I also broke the solver on purpose to confirm the tests fail when they should.
* Frontend tests 28/28, API end-to-end 12/12, UI smoke 4/4 – all against the real running C++ server.
* Manually checked with curl: all status codes, JSON/405/404 bodies, preflight, oversize body (413), path traversal on the static files (403), invalid configuration (exit code 2).

**Not verified – please check on your machine:**

* **macOS / Apple Silicon was not available.** The build scripts, presets and Homebrew instructions are written for it (and for bash 3.2) but have not been run there. Homebrew ships a newer Drogon than the 1.8.7 used here; I used only long-standing Drogon APIs, but a compile error on a newer version is possible.
* **No real browser** was available. The UI test uses jsdom, which does not enforce CSP or CORS and does not render CSS. Please open the app once in Chrome/Safari and press Solve; if the browser console reports a CSP or CORS error, the two settings above are the places to look.
* **Electron** was not tested; the ZIP contained no Electron main process. If you wrap the frontend in Electron, serve it from the backend (`http://127.0.0.1:8080`) or add `null` to `DLX_CORS_ORIGINS`.
* PostgreSQL support was not added, because the app has no data to persist.

## Remaining limitations

* `solutionCount` stops at `DLX_MAX_SOLUTIONS` (default 2), so the "Solutions" tile shows 2 for any puzzle with several solutions; the API flags this with `solutionCountIsLowerBound`, the UI does not display it.
* A solve runs on a Drogon I/O thread. Each is bounded by `DLX_MAX_NODES` (typical puzzles take well under a millisecond), but a very large limit combined with hostile input could occupy a thread.
* The server has no authentication or rate limiting; keep it on `127.0.0.1` unless you add a reverse proxy.
