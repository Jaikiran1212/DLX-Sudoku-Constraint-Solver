# DLX Solver HTTP API

Base URL: `http://127.0.0.1:8080` (configurable, see `.env.example`). All bodies are JSON (`Content-Type: application/json`).
There is no authentication: the backend is a stateless calculator, holds no user data and binds to `127.0.0.1` by default.

Grids are 9×9 arrays of integers: `0` = empty cell, `1`–`9` = digit (the same representation the frontend uses).

## `POST /api/solve`

Request:

```json
{ "grid": [[5,3,0,0,7,0,0,0,0], [6,0,0,1,9,5,0,0,0], "... 9 rows in total ..."] }
```

### Responses

| HTTP | `status` | When |
|-----:|----------|------|
| 200 | `solved` | At least one solution exists |
| 200 | `no-solution` | The clues do not repeat a digit, but no solution exists |
| 422 | `invalid` | Two clues repeat a digit in a row, column or box |
| 422 | `error` (`SEARCH_LIMIT_EXCEEDED`) | The search used more than `DLX_MAX_NODES` placements |
| 400 | `error` (`INVALID_JSON`, `INVALID_REQUEST`) | Body is not JSON / not a 9×9 grid of integers 0–9 |
| 413 | – | Body larger than 64 KB (empty body) |
| 415 | `error` (`UNSUPPORTED_MEDIA_TYPE`) | Content-Type is not `application/json` |
| 500 | `error` (`INTERNAL_ERROR`) | Unexpected failure (details are logged, not returned) |

`solved`:

```json
{
  "status": "solved",
  "solution": [[5,3,4,6,7,8,9,1,2], "... 9 rows ..."],
  "multipleSolutions": false,
  "solutionCount": 1,
  "solutionCountIsLowerBound": false,
  "stats": { "executionMs": 0.04, "recursiveCalls": 52, "nodesVisited": 51, "backtracks": 0 }
}
```

* `solution` is the first solution found; clues are unchanged.
* The search stops after `DLX_MAX_SOLUTIONS` solutions (default 2). `solutionCount` is therefore a count **up to that cap**;
  when the cap was hit, `solutionCountIsLowerBound` is `true` and `multipleSolutions` is `true` (the real number may be larger).
* `stats` (search only, clues are applied beforehand and not counted):
  `recursiveCalls` = calls of the recursive search; `nodesVisited` = candidate placements tried;
  `backtracks` = placements that were undone because nothing below them led to a solution (a puzzle solvable by pure
  deduction has 0); `executionMs` = matrix construction + search, measured on the server (excludes network time).

`no-solution`: `{ "status": "no-solution", "solutionCount": 0, "stats": { ... } }`

`invalid`: `{ "status": "invalid", "message": "The digit 5 appears twice in row 1 (columns 1 and 2)." }`

Errors: `{ "status": "error", "code": "INVALID_REQUEST", "message": "Row 4 must be an array of exactly 9 cells." }`

## `GET /api/status`

```json
{ "connected": true, "backend": "cpp-drogon", "solver": "dlx", "version": "1.0.0",
  "limits": { "maxSolutions": 2, "maxNodes": 5000000 } }
```

## `GET /api/health`

`{ "status": "ok" }`

## Other behaviour

* Wrong method on a known endpoint → `405` with `Allow` header and `{"status":"error","code":"METHOD_NOT_ALLOWED",...}`.
* Unknown path → `404` `{"status":"error","code":"NOT_FOUND",...}`.
* **CORS**: `Access-Control-Allow-Origin` is sent only for origins listed in `DLX_CORS_ORIGINS` (exact match). Preflight
  (`OPTIONS /api/*`) from an unlisted origin gets `403 CORS_ORIGIN_NOT_ALLOWED`. Allowed methods: `GET, POST, OPTIONS`; allowed header: `Content-Type`.
* All responses carry `X-Content-Type-Options: nosniff`; API responses carry `Cache-Control: no-store`.

## Example

```bash
curl -s -X POST http://127.0.0.1:8080/api/solve -H 'Content-Type: application/json' \
  -d '{"grid":[[5,3,0,0,7,0,0,0,0],[6,0,0,1,9,5,0,0,0],[0,9,8,0,0,0,0,6,0],[8,0,0,0,6,0,0,0,3],[4,0,0,8,0,3,0,0,1],[7,0,0,0,2,0,0,0,6],[0,6,0,0,0,0,2,8,0],[0,0,0,4,1,9,0,0,5],[0,0,0,0,8,0,0,7,9]]}'
```
