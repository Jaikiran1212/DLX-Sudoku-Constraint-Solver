# DLX Sudoku Constraint Solver

[![CI](https://github.com/Jaikiran1212/DLX-Sudoku-Constraint-Solver/actions/workflows/ci.yml/badge.svg?branch=migrate-final-dlx-solver)](https://github.com/Jaikiran1212/DLX-Sudoku-Constraint-Solver/actions/workflows/ci.yml?query=branch%3Amigrate-final-dlx-solver)

A browser-based 9×9 Sudoku solver powered by a C++20 backend implementing Knuth's Algorithm X with Dancing Links (DLX). The frontend is plain HTML, CSS, and JavaScript; the backend exposes a JSON REST API using Drogon.

## Live demo

- **Application:** https://dlx-sudoku-solver.vercel.app
- **Health check:** https://dlx-sudoku-solver.vercel.app/api/health
- **API status:** https://dlx-sudoku-solver.vercel.app/api/status
- **API reference:** [docs/API.md](docs/API.md)

The frontend is hosted on Vercel. Its `/api/*` routes are rewritten to the C++ API hosted on Render. Render's free service may spin down when idle, so the first request after inactivity can take longer.

## Features

- Solve 9×9 Sudoku puzzles with Algorithm X and Dancing Links.
- Validate grid shape, digit range, and duplicate clues before searching.
- Distinguish solved, unsatisfiable, invalid, and failed requests.
- Detect whether a puzzle has a unique or multiple solutions, subject to the configured solution limit.
- Show search telemetry from the C++ solver.
- Visualise the algorithm, replay steps, and inspect explanations.
- Generate puzzles, manage runs/history, change themes, and export results.
- Test the frontend, API, UI integration, and native solver with repeatable scripts.

## Architecture

```text
Browser
  └── Plain JavaScript UI (frontend/)
        └── /api/solve and /api/status
              └── Vercel rewrite
                    └── Render: C++20 + Drogon
                          ├── Request/grid validation
                          ├── Sudoku service and result mapping
                          └── Algorithm X + Dancing Links solver
```

The API is stateless: it does not use a database, accounts, or server-side user history. The browser keeps UI state locally.

## Repository layout

```text
.
├── frontend/                # HTML, CSS, browser JavaScript, frontend tests
├── backend/
│   ├── src/
│   │   ├── controllers/     # HTTP controller
│   │   ├── http/            # Middleware, CORS, JSON responses
│   │   ├── service/         # Validation and service/result mapping
│   │   └── solver/          # Algorithm X / Dancing Links
│   └── tests/               # C++ unit tests
├── docs/API.md              # HTTP API contract and examples
├── scripts/                 # Build, run, API-test, and UI-test scripts
├── tests/integration/       # End-to-end API tests
├── tests/ui/                # UI smoke tests
├── Dockerfile               # Render backend container
└── vercel.json              # Static output and /api rewrite
```

## Build and run locally (Ubuntu/Debian)

### 1. Install dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build \
  libdrogon-dev libjsoncpp-dev uuid-dev zlib1g-dev libssl-dev \
  libpq-dev libsqlite3-dev libmariadb-dev libhiredis-dev \
  libyaml-cpp-dev libbrotli-dev libc-ares-dev
```

Node.js 22 or a compatible supported version of Node.js and npm are needed for the JavaScript test suites. They are not required just to compile the C++ backend.

### 2. Install test dependencies

```bash
npm ci
```

### 3. Build and run the application

```bash
npm run backend:build
npm start
```

Open **http://127.0.0.1:8080**. The run script starts the C++ backend and serves the frontend from the same origin. Press `Ctrl+C` to stop it.

Optional environment settings are documented in [.env.example](.env.example). The server defaults to loopback for local development; only bind to a public interface when that is deliberate and appropriate for the deployment.

## Tests

Run the complete suite from the repository root:

```bash
npm ci
npm run backend:build
npm run test:frontend
npm run test:api
npm run test:ui
```

| Command | What it checks |
|---|---|
| `npm run backend:build` | Configures/builds C++ and runs CTest unit tests |
| `npm run test:frontend` | Frontend logic, generation, traces, grid engine, and API client |
| `npm run test:api` | Real HTTP endpoint contract, validation, CORS, concurrency, solver responses |
| `npm run test:ui` | Browser UI smoke tests against a running C++ backend |

GitHub Actions runs the same test suites on pushes to `main` and `migrate-final-dlx-solver`, and on pull requests targeting `main`. The workflow also builds the Docker image used for the hosted backend.

## API overview

| Method | Path | Purpose |
|---|---|---|
| `GET` | `/api/health` | Lightweight health check |
| `GET` | `/api/status` | Backend and solver status |
| `POST` | `/api/solve` | Solve a Sudoku grid submitted as JSON |

See [docs/API.md](docs/API.md) for request and response schemas, error codes, limits, CORS behaviour, and a `curl` example.

## Deployment

### Frontend — Vercel

The repository root is the Vercel project root. The project uses the `Other` preset with no build or install command and `frontend` as its output directory. [`vercel.json`](vercel.json) configures the static output and rewrites `/api/*` requests to the Render service.

### Backend — Render

Render builds the root [`Dockerfile`](Dockerfile). The container installs Drogon dependencies, compiles the C++ service, listens on `0.0.0.0`, and uses Render's `PORT` environment variable unless `DLX_PORT` is explicitly set. Configure the service health check as `/api/health`.

To deploy another environment, update the destination in `vercel.json` to the corresponding backend URL and redeploy the frontend.

## Configuration

See [.env.example](.env.example) for the available local settings, including:

- `DLX_HOST` and `DLX_PORT`: bind address and port.
- `DLX_CORS_ORIGINS`: exact-match list of allowed browser origins.
- `DLX_STATIC_DIR`: frontend directory to serve; an empty value makes the service API-only.
- `DLX_MAX_SOLUTIONS` and `DLX_MAX_NODES`: search limits.
- `DLX_THREADS` and `DLX_LOG_LEVEL`: worker and logging settings.

The hosted Vercel frontend calls its own origin; Vercel forwards API requests to Render. Do not commit real secrets or local `.env` files.

## Security notes

The solver API is currently unauthenticated and does not implement application accounts or server-side data storage. The hosted service is suitable as a demo; add appropriate rate limits and abuse protection before relying on it for high-volume public traffic.

## License

See [LICENSE](LICENSE) for the project's license terms.
