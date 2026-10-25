/* End-to-end test of the running C++ backend over real HTTP.
 *
 *   API_BASE=http://127.0.0.1:8080 node tests/integration/api.test.js
 *   (or just run scripts/test-api.sh, which starts and stops the server for you)
 *
 * It drives the frontend's own solver client and puzzle generator, so it checks the actual
 * frontend <-> backend contract, not just the backend in isolation. Needs Node 18+. */
'use strict';
const path = require('node:path');
const assert = require('node:assert/strict');

const API = (process.env.API_BASE || 'http://127.0.0.1:8080').replace(/\/+$/, '');
const FRONT_ORIGIN = process.env.FRONT_ORIGIN || 'http://localhost:3000'; // must be in DLX_CORS_ORIGINS

global.window = {};
window.DLX = {};
window.DLX_CONFIG = { apiBaseUrl: API, requestTimeoutMs: 15000 };
const js = (f) => path.join(__dirname, '..', '..', 'frontend', 'js', f);
require(js('generator.js'));
window.DLX.constants = { SIZE: 9, BOX: 3 };
require(js('validation.js'));
require(js('solver-client.js'));
const { generator: G, solverClient: client } = window.DLX;

let passed = 0, failed = 0;
async function test(name, fn) {
  try { await fn(); passed++; console.log(`  ok   ${name}`); }
  catch (e) { failed++; console.error(`  FAIL ${name}\n       ${e.message}`); }
}
const post = (body, headers = { 'Content-Type': 'application/json' }) =>
  fetch(`${API}/api/solve`, { method: 'POST', headers, body: typeof body === 'string' ? body : JSON.stringify(body) });
const empty = () => Array.from({ length: 9 }, () => Array(9).fill(0));

(async () => {
  console.log(`backend ${API}`);

  await test('GET /api/status says connected, cpp-drogon', async () => {
    const info = await client.getStatus();
    assert.equal(info.connected, true);
    assert.equal(info.backend, 'cpp-drogon');
  });
  await test('GET /api/health', async () => {
    const r = await fetch(`${API}/api/health`);
    assert.equal(r.status, 200);
    assert.deepEqual(await r.json(), { status: 'ok' });
  });

  for (const level of ['easy', 'medium', 'hard']) {
    await test(`${level}: 10 generated puzzles are solved to the frontend's answer key`, async () => {
      for (let i = 0; i < 10; i++) {
        const p = G.generatePuzzle(level);
        const r = await client.solvePuzzle(p.grid);
        assert.equal(r.status, 'solved', `puzzle ${i}: ${JSON.stringify(r).slice(0, 200)}`);
        assert.deepEqual(r.solution, p.key, `puzzle ${i}: solution differs from the unique key`);
        assert.equal(r.multipleSolutions, false);
        assert.equal(r.solutionCount, 1);
        for (const k of ['executionMs', 'recursiveCalls', 'nodesVisited', 'backtracks']) {
          assert.equal(typeof r.stats[k], 'number', `stats.${k}`);
        }
      }
    });
  }

  await test('empty board: solved, flagged as having multiple solutions', async () => {
    const r = await client.solvePuzzle(empty());
    assert.equal(r.status, 'solved');
    assert.equal(r.multipleSolutions, true);
  });
  await test('unsolvable board without duplicates -> no-solution', async () => {
    const g = empty();
    for (let c = 0; c < 8; c++) g[0][c] = c + 1;
    g[1][8] = 9;
    const r = await client.solvePuzzle(g);
    assert.equal(r.status, 'no-solution');
    assert.equal(r.solutionCount, 0);
  });
  await test('duplicate clues -> invalid (HTTP 422) with a message', async () => {
    const g = empty(); g[0][0] = 5; g[0][1] = 5;
    const r = await post({ grid: g });
    assert.equal(r.status, 422);
    const body = await r.json();
    assert.equal(body.status, 'invalid');
    assert.match(body.message, /row 1/);
    assert.equal((await client.solvePuzzle(g)).status, 'invalid');
  });

  await test('malformed requests get 4xx JSON errors', async () => {
    let r = await post('{nope');
    assert.equal(r.status, 400); assert.equal((await r.json()).code, 'INVALID_JSON');
    r = await post({ grid: [[1]] });
    assert.equal(r.status, 400); assert.equal((await r.json()).code, 'INVALID_REQUEST');
    const g = empty(); g[4][4] = 12;
    r = await post({ grid: g });
    assert.equal(r.status, 400); assert.match((await r.json()).message, /Row 5, column 5/);
    r = await post('{}', { 'Content-Type': 'text/plain' });
    assert.equal(r.status, 415);
    r = await fetch(`${API}/api/solve`);
    assert.equal(r.status, 405); assert.equal((await r.json()).code, 'METHOD_NOT_ALLOWED');
    r = await fetch(`${API}/api/does-not-exist`);
    assert.equal(r.status, 404); assert.equal((await r.json()).status, 'error');
  });

  await test('CORS: allowed origin gets headers on preflight and on the real request', async () => {
    let r = await fetch(`${API}/api/solve`, { method: 'OPTIONS', headers: { Origin: FRONT_ORIGIN, 'Access-Control-Request-Method': 'POST' } });
    assert.equal(r.status, 204);
    assert.equal(r.headers.get('access-control-allow-origin'), FRONT_ORIGIN);
    assert.match(r.headers.get('access-control-allow-methods'), /POST/);
    r = await post({ grid: empty() }, { 'Content-Type': 'application/json', Origin: FRONT_ORIGIN });
    assert.equal(r.headers.get('access-control-allow-origin'), FRONT_ORIGIN);
  });
  await test('CORS: unknown origin is refused on preflight and gets no allow header', async () => {
    let r = await fetch(`${API}/api/solve`, { method: 'OPTIONS', headers: { Origin: 'http://evil.test' } });
    assert.equal(r.status, 403);
    assert.equal(r.headers.get('access-control-allow-origin'), null);
    r = await post({ grid: empty() }, { 'Content-Type': 'application/json', Origin: 'http://evil.test' });
    assert.equal(r.headers.get('access-control-allow-origin'), null);
  });

  await test('50 concurrent solves all succeed', async () => {
    const puzzles = Array.from({ length: 50 }, () => G.generatePuzzle('medium'));
    const results = await Promise.all(puzzles.map((p) => client.solvePuzzle(p.grid)));
    results.forEach((r, i) => assert.deepEqual(r.solution, puzzles[i].key));
  });

  console.log(`\n${passed} passed, ${failed} failed`);
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.error(e); process.exit(1); });
