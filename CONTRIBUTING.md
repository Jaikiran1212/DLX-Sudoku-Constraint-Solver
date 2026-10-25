# Contributing

Thank you for considering a contribution to DLX Sudoku Constraint Solver. Bug reports, reproducible test cases, documentation improvements, and focused code changes are welcome.

## Before you start

1. Check existing issues and pull requests for related work.
2. For a substantial change, open an issue first to discuss the design and scope.
3. Keep changes focused. Avoid unrelated formatting or generated-file changes.
4. Never commit credentials, production secrets, local `.env` files, build output, or editor-specific settings.

## Development setup

The documented local setup targets Ubuntu/Debian. Install the C++/Drogon dependencies listed in the root [README](README.md), install Node.js 22 or a compatible supported version, and run:

```bash
npm ci
npm run backend:build
```

Start the local application with:

```bash
npm start
```

The app is served at `http://127.0.0.1:8080` by default.

## Validation before a pull request

Run the full test suite from the repository root:

```bash
npm run backend:build
npm run test:frontend
npm run test:api
npm run test:ui
git diff --check
```

The API and UI test scripts may start a local backend as part of their run. Allow each command to finish and inspect any failures before opening the pull request.

## Pull request workflow

1. Create a descriptive branch from the latest `main`, for example `docs/improve-api-guide` or `fix/grid-validation`.
2. Make the smallest coherent change that addresses the issue.
3. Add or update tests when behaviour changes.
4. Update documentation and configuration examples when the public API or setup changes.
5. Run the validation commands above.
6. Open a pull request targeting `main`. Explain the motivation, implementation, tests run, and any deployment/configuration impact.
7. Wait for CI and deployment preview checks to finish; resolve review feedback before merging.

## Coding and documentation expectations

- Preserve clear separation between the frontend, HTTP/API layer, service logic, and solver.
- Keep API responses and error behaviour consistent with [docs/API.md](docs/API.md).
- Prefer explicit names and small functions over clever abstractions.
- Document non-obvious algorithmic decisions and configuration variables.
- Do not claim tests, benchmarks, or security checks passed unless they were actually run.
