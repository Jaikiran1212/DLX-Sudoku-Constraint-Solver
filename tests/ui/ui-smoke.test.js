/* UI smoke test: loads the real frontend/index.html with all of its scripts in jsdom and
 * clicks the real Solve button against a running backend. jsdom is not a real browser, so
 * this does not cover layout, CSS, CSP enforcement or real CORS enforcement; see README.
 *
 *   API_BASE=http://127.0.0.1:8080 node tests/ui/ui-smoke.test.js */
'use strict';
const path = require('node:path');
const assert = require('node:assert/strict');
const { JSDOM, VirtualConsole } = require('jsdom');

const API = (process.env.API_BASE || 'http://127.0.0.1:8080').replace(/\/+$/, '');
const DEAD = 'http://127.0.0.1:9'; // nothing listens here
const CONFIGURED = 'http://127.0.0.1:8080'; // what frontend/js/config.js points to

const PUZZLE = ['530070000', '600195000', '098000060', '800060003', '400803001', '700020006', '060000280', '000419005', '000080079']
  .map((r) => Array.from(r, Number));
const SOLUTION = ['534678912', '672195348', '198342567', '859761423', '426853791', '713924856', '961537284', '287419635', '345286179']
  .map((r) => Array.from(r, Number));

async function openApp(target) {
  const errors = [];
  const vc = new VirtualConsole();
  vc.on('jsdomError', (e) => errors.push(e.message));
  vc.on('error', (...a) => errors.push(a.join(' ')));
  const dom = await JSDOM.fromFile(path.join(__dirname, '..', '..', 'frontend', 'index.html'), {
    runScripts: 'dangerously',
    resources: 'usable',
    pretendToBeVisual: true,
    virtualConsole: vc,
    beforeParse(window) {
      window.matchMedia = window.matchMedia || (() => ({ matches: false, addEventListener() {}, removeEventListener() {}, addListener() {}, removeListener() {} }));
      // Route the page's configured API origin to the backend under test.
      window.fetch = (url, init) => fetch(String(url).replace(CONFIGURED, target), init);
      window.AbortController = AbortController;
      window.ResizeObserver = window.ResizeObserver || class { observe() {} unobserve() {} disconnect() {} }; // not in jsdom
    },
  });
  await new Promise((resolve) => dom.window.addEventListener('load', resolve));
  await new Promise((r) => setTimeout(r, 300)); // init() awaits getStatus()
  return { dom, win: dom.window, doc: dom.window.document, errors };
}

async function waitFor(fn, what, ms = 8000) {
  const end = Date.now() + ms;
  while (Date.now() < end) { if (fn()) return; await new Promise((r) => setTimeout(r, 25)); }
  throw new Error(`timed out waiting for ${what}`);
}

let passed = 0, failed = 0;
async function test(name, fn) {
  try { await fn(); passed++; console.log(`  ok   ${name}`); }
  catch (e) { failed++; console.error(`  FAIL ${name}\n       ${e.stack || e.message}`); }
}

(async () => {
  console.log('frontend UI (jsdom) against', API);

  await test('page loads without script errors and shows the backend as Connected', async () => {
    const { doc, errors, dom } = await openApp(API);
    assert.deepEqual(errors, [], `script errors: ${errors.join(' | ')}`);
    assert.equal(doc.getElementById('ss-solver').textContent, 'Connected');
    dom.window.close();
  });

  await test('Solve button: sends the board, shows the C++ solution and real telemetry', async () => {
    const { doc, win, dom } = await openApp(API);
    win.DLX.loadPuzzle(PUZZLE.map((r) => r.slice()), null);
    const solve = doc.getElementById('btn-solve');
    assert.equal(solve.disabled, false);
    solve.click();
    await waitFor(() => win.DLX.state.status === 'solved', 'status solved');
    assert.deepEqual(win.DLX.state.solution, SOLUTION);
    assert.equal(doc.getElementById('header-status-text').textContent, 'Solved');
    assert.match(doc.getElementById('ss-time').textContent, /ms$/);
    assert.notEqual(doc.getElementById('ss-nodes').textContent, '--');
    assert.equal(doc.getElementById('ss-solutions').textContent, '1');
    assert.equal(solve.disabled, false, 'controls are released after solving');
    dom.window.close();
  });

  await test('backend down: header says Solver not connected, board untouched, controls released', async () => {
    const { doc, win, dom } = await openApp(DEAD);
    assert.equal(doc.getElementById('ss-solver').textContent, 'Not connected');
    win.DLX.loadPuzzle(PUZZLE.map((r) => r.slice()), null);
    doc.getElementById('btn-solve').click();
    await waitFor(() => win.DLX.state.status === 'backend-missing', 'status backend-missing');
    assert.equal(doc.getElementById('header-status-text').textContent, 'Solver not connected');
    assert.equal(win.DLX.state.solution, null);
    assert.equal(doc.getElementById('btn-solve').disabled, false);
    dom.window.close();
  });

  await test('backend started after page load: indicator recovers after a successful solve', async () => {
    let target = DEAD;
    const { doc, win, dom } = await openApp(DEAD);
    win.fetch = (url, init) => fetch(String(url).replace(CONFIGURED, target), init);
    assert.equal(doc.getElementById('ss-solver').textContent, 'Not connected');
    target = API;
    win.DLX.loadPuzzle(PUZZLE.map((r) => r.slice()), null);
    doc.getElementById('btn-solve').click();
    await waitFor(() => win.DLX.state.status === 'solved', 'status solved');
    assert.equal(doc.getElementById('ss-solver').textContent, 'Connected');
    dom.window.close();
  });

  console.log(`\n${passed} passed, ${failed} failed`);
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.error(e); process.exit(1); });
