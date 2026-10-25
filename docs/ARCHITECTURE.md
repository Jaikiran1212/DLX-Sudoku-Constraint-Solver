# Architecture

## Overview

DLX Sudoku Constraint Solver is a browser-based 9×9 Sudoku application. The user interface is built with HTML, CSS, and browser JavaScript. A C++20 service built with Drogon validates requests and solves puzzles using Knuth's Algorithm X with Dancing Links (DLX).

The hosted deployment separates the static frontend from the API:

- **Frontend:** Vercel serves the files in `frontend/`.
- **API:** Render builds and runs the C++ service from the root `Dockerfile`.
- **Routing:** `vercel.json` rewrites browser requests under `/api/*` to the Render API. The browser can therefore call a same-origin `/api` path.

## Request flow

```text
Browser UI
  │
  ├── validates and manages interactive UI state
  │
  └── frontend API client: /api/solve, /api/status
          │
          ▼
      Vercel rewrite
          │
          ▼
      Render-hosted Drogon API
          │
          ├── HTTP controller and middleware
          ├── request and grid validation
          ├── Sudoku service and response mapping
          └── Algorithm X + Dancing Links solver
```

The `GET /api/health` endpoint supports health checks and deployment monitoring.

## Backend responsibilities

1. **HTTP layer:** Accepts JSON requests, applies middleware and CORS policy, and returns consistent HTTP/JSON responses.
2. **Validation/service layer:** Checks the request structure and Sudoku clues before invoking the solver; maps solver outcomes to API response statuses.
3. **Solver layer:** Represents the exact-cover problem and searches for solutions with Algorithm X and Dancing Links.
4. **Limits and telemetry:** Configurable search limits bound work, while response statistics describe the search performed by the backend.

The API is stateless. It does not require a database or user accounts, and it does not persist user puzzles on the server. Browser-only UI state is managed by the frontend.

## Repository map

| Path | Responsibility |
|---|---|
| `frontend/` | Static UI, styles, browser logic, and frontend tests |
| `backend/src/controllers/` | HTTP endpoints |
| `backend/src/http/` | HTTP middleware, CORS, and JSON responses |
| `backend/src/service/` | Validation and result mapping |
| `backend/src/solver/` | Algorithm X / DLX implementation |
| `backend/tests/` | Native C++ unit tests |
| `tests/integration/` | HTTP API integration tests |
| `tests/ui/` | UI smoke tests |
| `scripts/` | Build, run, and test entry points |
| `docs/API.md` | HTTP contract and examples |
| `Dockerfile` | Container build for the hosted backend |
| `vercel.json` | Static output configuration and API rewrite |

## Runtime configuration

Local settings are documented in `.env.example`. Important variables include:

- `DLX_HOST`, `DLX_PORT`: server bind address and port.
- `DLX_CORS_ORIGINS`: exact-match list of permitted browser origins.
- `DLX_STATIC_DIR`: frontend directory to serve locally; an empty value runs API-only mode.
- `DLX_MAX_SOLUTIONS`, `DLX_MAX_NODES`: solver search limits.
- `DLX_THREADS`, `DLX_LOG_LEVEL`: worker and logging configuration.

The Render container binds to `0.0.0.0` and respects Render's `PORT` environment variable unless `DLX_PORT` is explicitly configured. Do not expose a local development server to an untrusted network without reviewing its host and CORS settings.

## Design boundaries

- The solver is the source of truth for the solution and search telemetry.
- The frontend should not infer backend execution metrics or claim that an educational explanation is the exact internal search trace unless the implementation explicitly records that trace.
- The API currently has no authentication. Add rate limiting and abuse protection before using it for high-volume public traffic.
- Third-party packages retain their own licenses; see their package and distribution notices.
