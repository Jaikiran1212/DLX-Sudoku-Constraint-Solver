/* The renderer's only door to the solver: the C++ backend's HTTP API (see docs/API.md).
 * Every result is normalised to this contract, which the rest of the UI relies on:
 *
 *   { status: 'solved',      solution, multipleSolutions?, solutionCount?, stats? }
 *   { status: 'no-solution', stats? }
 *   { status: 'invalid',     message }
 *   { status: 'error',       code, message }
 *
 * Network problems become { status: 'error', code: 'BACKEND_NOT_CONNECTED' } so the UI can
 * show "Solver not connected" instead of a generic failure. */
(function () {
  'use strict';
  const DLX = window.DLX;

  const config = Object.assign({ apiBaseUrl: 'http://127.0.0.1:8080', requestTimeoutMs: 30000 }, window.DLX_CONFIG);
  const baseUrl = String(config.apiBaseUrl).replace(/\/+$/, '');

  const failure = (code, message) => ({ status: 'error', code, message });

  /** fetch with a timeout. Rejects with an Error whose name is 'AbortError' on timeout. */
  async function request(path, init) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), config.requestTimeoutMs);
    try {
      return await fetch(baseUrl + path, { ...init, signal: controller.signal });
    } finally {
      clearTimeout(timer);
    }
  }

  async function readJson(response) {
    try {
      return await response.json();
    } catch (error) {
      return null;
    }
  }

  async function getStatus() {
    try {
      const response = await request('/api/status', { method: 'GET' });
      const info = await readJson(response);
      if (!response.ok || !info || info.connected !== true) {
        return { connected: false, backend: 'none', reason: `The backend answered with HTTP ${response.status}.` };
      }
      return info;
    } catch (error) {
      return { connected: false, backend: 'none', reason: `Could not reach the solver backend at ${baseUrl}.` };
    }
  }

  async function solvePuzzle(grid) {
    let response;
    try {
      response = await request('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ grid }),
      });
    } catch (error) {
      if (error && error.name === 'AbortError') {
        return failure('TIMEOUT', 'The solver took too long to answer, so the request was cancelled.');
      }
      return failure('BACKEND_NOT_CONNECTED', `Could not reach the C++ solver at ${baseUrl}. Start the backend and try again.`);
    }

    const result = await readJson(response);
    if (!result || typeof result !== 'object') {
      return response.ok
        ? failure('MALFORMED_RESULT', 'The solver returned an unreadable result.')
        : failure(`HTTP_${response.status}`, `The solver backend answered with HTTP ${response.status}.`);
    }

    switch (result.status) {
      case 'solved':
        // Never display a "solution" that contradicts the puzzle that was sent.
        return DLX.solutionMatchesPuzzle(result.solution, grid)
          ? result
          : failure('INVALID_SOLUTION', 'The solver returned a grid that does not match the puzzle.');
      case 'no-solution':
        return result;
      case 'invalid':
        return { status: 'invalid', message: String(result.message || 'The solver rejected the puzzle.') };
      case 'error':
        return failure(String(result.code || 'SOLVER_ERROR'), String(result.message || 'Unknown solver error.'));
      default:
        return failure('MALFORMED_RESULT', 'The solver returned an unknown status.');
    }
  }

  DLX.solverClient = { getStatus, solvePuzzle };
})();
