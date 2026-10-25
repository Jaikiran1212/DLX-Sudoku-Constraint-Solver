/* Dependency-free tests for the pure frontend logic. Run:  node frontend/tests/run-tests.js
 * (UI behaviour that needs a DOM is not covered here.) */
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');

global.window = {}; window.DLX = {};
const js = (f) => path.join(__dirname, '..', 'js', f);
require(js('generator.js'));
const G = window.DLX.generator;

let passed = 0;
function test(name, fn) {
  try { fn(); passed++; console.log(`  ok   ${name}`); }
  catch (e) { console.error(`  FAIL ${name}\n       ${e.message}`); process.exitCode = 1; }
}

console.log('generator');
test('dialog ranges in index.html are generated from LEVELS (no hard-coded numbers)', () => {
  const html = fs.readFileSync(path.join(__dirname, '..', 'index.html'), 'utf8');
  for (const level of Object.keys(G.LEVELS)) {
    assert.match(html, new RegExp(`data-range="${level}"`), `missing data-range for ${level}`);
  }
  assert.doesNotMatch(html, /About \d+–\d+ clues/, 'hard-coded clue range found in index.html');
});

for (const level of ['easy', 'medium', 'hard']) {
  test(`${level}: exact results satisfy clue range AND technique rule, and are unique`, () => {
    const cfg = G.LEVELS[level];
    let exact = 0;
    for (let i = 0; i < 8; i++) {
      const r = G.generatePuzzle(level);
      assert.ok(['exact', 'approximate'].includes(r.status), `unexpected status ${r.status}`);
      assert.equal(r.exact, r.status === 'exact');
      assert.equal(G.countSolutions(r.grid, 2), 1, 'puzzle must have exactly one solution');
      assert.equal(G.countClues(r.grid), r.clues);
      assert.equal(G.countSolutions(r.grid, 2, null) === 1 && r.key.every((row, rr) => row.every((v, c) => !r.grid[rr][c] || r.grid[rr][c] === v)), true, 'key agrees with clues');
      if (r.exact) {
        exact++;
        assert.ok(r.clues >= cfg.min && r.clues <= cfg.max, `${level} had ${r.clues} clues`);
        assert.equal(G.assessLevel(r.grid), level);
        assert.ok(G.matchesLevel(r.grid, level));
      } else {
        assert.ok(r.reason, 'approximate result must explain how it misses');
        assert.ok(!G.matchesLevel(r.grid, level), 'approximate result must not match the criteria');
      }
    }
    assert.ok(exact >= 6, `only ${exact}/8 exact matches`);
  });
}

test('matchesLevel rejects out-of-range clue counts and wrong techniques', () => {
  const key = G.generatePuzzle('easy').key;
  assert.equal(G.matchesLevel(key, 'easy'), false, 'a full grid has 81 clues');
  const hard = G.generatePuzzle('hard');
  if (hard.exact) assert.equal(G.matchesLevel(hard.grid, 'easy'), false);
});

test('a zero time budget still returns a valid, honestly-labelled result', () => {
  const r = G.generatePuzzle('hard', 0);
  assert.ok(['exact', 'approximate'].includes(r.status));
  assert.equal(G.countSolutions(r.grid, 2), 1);
  if (r.status === 'approximate') assert.equal(r.exact, false);
});

test('uniqueness check gives up (-1) instead of running forever', () => {
  const empty = Array.from({ length: 9 }, () => Array(9).fill(0));
  assert.equal(G.countSolutions(empty, 2, null, 5), -1);
  assert.equal(G.answerKeyFor(empty).key, null);
});

test('unknown level fails safely', () => {
  const r = G.generatePuzzle('impossible');
  assert.equal(r.status, 'failed');
});

console.log('explain / trace');
window.DLX.constants = { SIZE: 9 };
require(js('dlx-trace.js'));
require(js('explain.js'));
test('buildSolveSteps places every empty cell with the solution digit and never invents digits', () => {
  const r = G.generatePuzzle('medium');
  const steps = window.DLX.explain.buildSolveSteps(r.grid, r.key);
  const empties = r.grid.flat().filter((v) => v === 0).length;
  assert.equal(steps.length, empties);
  for (const s of steps) assert.equal(s.d, r.key[s.r][s.c]);
});
test('trace replay reaches a valid solution and records reasons for each move', () => {
  const r = G.generatePuzzle('hard');
  const t = window.DLX.buildTrace(r.grid);
  assert.ok(t.solved);
  assert.equal(t.cols.length, t.events.length);
  const g = r.grid.map((row) => row.slice());
  for (const ev of t.events) {
    const undo = ev >= 729, x = ev % 729, d = (x % 9) + 1, cell = (x - (d - 1)) / 9;
    g[Math.floor(cell / 9)][cell % 9] = undo ? 0 : d;
  }
  assert.deepEqual(g, r.key);
});

console.log('algorithmic visualisation engine');
require(js('algo-engine.js'));
const A = window.DLX.algo;
function runAll(m, cap = 3e6) {
  const it = A.run(m); let n = 0, last = null; const placed = new Set();
  for (let s = it.next(); !s.done && n < cap; s = it.next()) {
    n++; last = s.value;
    if (last.place !== undefined) placed.add(last.place);
    if (last.unplace !== undefined) placed.delete(last.unplace);
    for (const g of last.glows) assert.ok(g.node >= 0 && g.node < m.nodeCount && 'LRUD'.includes(g.dir), 'glow refers to a real pointer');
  }
  return { n, last, placed };
}
test('sample matrix matches the reference diagram (column sizes 2,2,2,3,2,2,4)', () => {
  const { matrix } = A.sampleMatrix();
  assert.deepEqual([1, 2, 3, 4, 5, 6, 7].map((i) => matrix.S[i]), [2, 2, 2, 3, 2, 2, 4]);
});
test('sample run finds the known exact cover (rows 2, 4 and 6)', () => {
  const { matrix } = A.sampleMatrix();
  const { last, placed } = runAll(matrix);
  assert.equal(last.phase, 'solution');
  assert.deepEqual([...placed].sort(), [1, 3, 5]);
});
test('pointer-level run on a generated puzzle ends in a valid solution', () => {
  const r = G.generatePuzzle('medium');
  const { matrix } = A.puzzleMatrix(r.grid);
  const { last, placed } = runAll(matrix);
  assert.equal(last.phase, 'solution');
  const out = r.grid.map((row) => row.slice());
  for (const ri of placed) { const { r: rr, c, d } = matrix.rows[ri].meta; out[rr][c] = d; }
  assert.deepEqual(out, r.key);
});
test('a search with no solution restores every pointer (dancing links invariant)', () => {
  const make = () => A.createMatrix([{ name: 'a' }, { name: 'b' }, { name: 'c' }],
    [{ label: 'r1', cols: [0, 1] }, { label: 'r2', cols: [1, 2] }]); // no exact cover exists
  const m = make();
  const before = JSON.stringify([m.L, m.R, m.U, m.D, m.S]);
  const { last } = runAll(m);
  assert.notEqual(last.phase, 'solution');
  assert.equal(JSON.stringify([m.L, m.R, m.U, m.D, m.S]), before);
});

console.log('9x9 pointer grid engine');
require(js('algo-grid-engine.js'));
const GE = window.DLX.algoGrid;
function runGrid(grid, cap = 5e6) {
  const g = GE.create(grid), it = GE.run(g);
  let n = 0, last = null;
  for (let s = it.next(); !s.done && n < cap; s = it.next()) {
    n++; last = s.value;
    for (const q of last.glows) assert.ok(q.node >= 0 && q.node < g.count && 'LRUD'.includes(q.dir), 'glow refers to a real pointer');
  }
  return { g, last, n };
}
test('fresh mesh: every square points at its neighbours, edges wrap to headers', () => {
  const g = GE.create(Array.from({ length: 9 }, () => Array(9).fill(0)));
  assert.equal(g.R[0], 1); assert.equal(g.L[1], 0); assert.equal(g.D[0], 9); assert.equal(g.U[9], 0);
  assert.equal(g.L[0], GE.ROW_H); assert.equal(g.R[8], GE.ROW_H); assert.equal(g.U[0], GE.COL_H); assert.equal(g.D[72], GE.COL_H);
});
test('grid run solves a generated puzzle to the unique answer', () => {
  const r = G.generatePuzzle('medium');
  const { g, last } = runGrid(r.grid);
  assert.equal(last.phase, 'solution');
  for (let i = 0; i < 81; i++) assert.equal(g.digit[i], r.key[Math.floor(i / 9)][i % 9]);
});
test('after solving, only filled squares are unlinked and every pointer still targets a real node', () => {
  const r = G.generatePuzzle('easy');
  const { g } = runGrid(r.grid);
  for (let i = 0; i < 81; i++) for (const d of 'LRUD') assert.ok(g[d][i] >= 0 && g[d][i] < g.count);
  for (let r2 = 0; r2 < 9; r2++) assert.equal(g.R[GE.ROW_H + r2], GE.ROW_H + r2, 'row header points to itself when the row is full');
});
test('an unsolvable puzzle backtracks fully and restores every pointer and candidate', () => {
  const grid = Array.from({ length: 9 }, () => Array(9).fill(0));
  [1, 2, 3, 4, 5, 6, 7, 8].forEach((d, c) => { grid[0][c] = d; });
  grid[1][8] = 9; // (1,9) needs a 9, but column 9 already holds one
  assert.equal(G.countSolutions(grid, 2), 0, 'test puzzle must really be unsolvable');
  const g = GE.create(grid), it = GE.run(g);
  let snapshot = null, last = null;
  const state = () => JSON.stringify([g.L, g.R, g.U, g.D, g.mask, g.digit]);
  for (let s = it.next(); !s.done; s = it.next()) {
    last = s.value;
    if (!snapshot && last.text.startsWith('All given digits')) snapshot = state(); // mesh after the givens are removed
  }
  assert.ok(snapshot, 'setup phase was reached');
  assert.notEqual(last.phase, 'solution');
  assert.equal(state(), snapshot, 'pointers, candidates and digits are exactly as before the search');
});

/* ---------- solver client (HTTP) ---------- */
window.DLX.constants = { SIZE: 9, BOX: 3 };
window.DLX_CONFIG = { apiBaseUrl: 'http://api.test/', requestTimeoutMs: 60 };
require(js('validation.js'));
require(js('solver-client.js'));
const client = window.DLX.solverClient;

const PUZZLE = DLX_EXAMPLE_ROWS();
function DLX_EXAMPLE_ROWS() {
  return ['530070000', '600195000', '098000060', '800060003', '400803001', '700020006', '060000280', '000419005', '000080079']
    .map((row) => Array.from(row, Number));
}
const SOLUTION = ['534678912', '672195348', '198342567', '859761423', '426853791', '713924856', '961537284', '287419635', '345286179']
  .map((row) => Array.from(row, Number));
const reply = (status, body) => ({ ok: status >= 200 && status < 300, status, json: async () => body });

async function atest(name, fn) {
  try { await fn(); passed++; console.log(`  ok   ${name}`); }
  catch (e) { console.error(`  FAIL ${name}\n       ${e.message}`); process.exitCode = 1; }
}

(async () => {
  console.log('solver client');
  await atest('POSTs the grid as JSON to <apiBaseUrl>/api/solve and returns a solved result', async () => {
    let seen;
    global.fetch = async (url, init) => { seen = { url, init }; return reply(200, { status: 'solved', solution: SOLUTION, solutionCount: 1, stats: { executionMs: 1 } }); };
    const r = await client.solvePuzzle(PUZZLE);
    assert.equal(seen.url, 'http://api.test/api/solve');
    assert.equal(seen.init.method, 'POST');
    assert.equal(seen.init.headers['Content-Type'], 'application/json');
    assert.deepEqual(JSON.parse(seen.init.body), { grid: PUZZLE });
    assert.equal(r.status, 'solved');
    assert.deepEqual(r.solution, SOLUTION);
  });
  await atest('a solution that contradicts the clues is rejected (INVALID_SOLUTION)', async () => {
    const wrong = SOLUTION.map((row) => row.slice()); wrong[0][0] = 1; // clue is 5
    global.fetch = async () => reply(200, { status: 'solved', solution: wrong });
    assert.equal((await client.solvePuzzle(PUZZLE)).code, 'INVALID_SOLUTION');
  });
  await atest('no-solution passes through', async () => {
    global.fetch = async () => reply(200, { status: 'no-solution', solutionCount: 0, stats: {} });
    assert.equal((await client.solvePuzzle(PUZZLE)).status, 'no-solution');
  });
  await atest('HTTP 422 invalid keeps the server message', async () => {
    global.fetch = async () => reply(422, { status: 'invalid', message: 'The digit 5 appears twice in row 1.' });
    const r = await client.solvePuzzle(PUZZLE);
    assert.deepEqual(r, { status: 'invalid', message: 'The digit 5 appears twice in row 1.' });
  });
  await atest('server error bodies keep their code and message', async () => {
    global.fetch = async () => reply(422, { status: 'error', code: 'SEARCH_LIMIT_EXCEEDED', message: 'too big' });
    assert.deepEqual(await client.solvePuzzle(PUZZLE), { status: 'error', code: 'SEARCH_LIMIT_EXCEEDED', message: 'too big' });
  });
  await atest('unreachable backend becomes BACKEND_NOT_CONNECTED', async () => {
    global.fetch = async () => { throw new TypeError('fetch failed'); };
    assert.equal((await client.solvePuzzle(PUZZLE)).code, 'BACKEND_NOT_CONNECTED');
  });
  await atest('a hung request times out (TIMEOUT)', async () => {
    global.fetch = (url, init) => new Promise((_, reject) => {
      init.signal.addEventListener('abort', () => { const e = new Error('aborted'); e.name = 'AbortError'; reject(e); });
    });
    assert.equal((await client.solvePuzzle(PUZZLE)).code, 'TIMEOUT');
  });
  await atest('non-JSON error responses become HTTP_<status>', async () => {
    global.fetch = async () => ({ ok: false, status: 502, json: async () => { throw new SyntaxError('not json'); } });
    assert.equal((await client.solvePuzzle(PUZZLE)).code, 'HTTP_502');
  });
  await atest('unknown status values are MALFORMED_RESULT', async () => {
    global.fetch = async () => reply(200, { status: 'banana' });
    assert.equal((await client.solvePuzzle(PUZZLE)).code, 'MALFORMED_RESULT');
  });
  await atest('getStatus reports connected / not connected', async () => {
    global.fetch = async () => reply(200, { connected: true, backend: 'cpp-drogon' });
    assert.equal((await client.getStatus()).connected, true);
    global.fetch = async () => reply(500, {});
    assert.equal((await client.getStatus()).connected, false);
    global.fetch = async () => { throw new TypeError('down'); };
    assert.equal((await client.getStatus()).connected, false);
  });
  console.log(`\n${passed} passed${process.exitCode ? ', some FAILED' : ''}`);
})();
