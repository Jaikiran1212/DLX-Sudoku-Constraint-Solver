/* Dancing Links on the 9x9 board itself, for the "Algorithmic visualisation".
 *
 * Every square (r,c) is a node with four pointers L, R, U, D to its neighbours in the mesh of
 * squares that are still EMPTY. Filling a square unlinks it from the mesh (its four neighbours
 * skip over it) while the square keeps its own pointers, so it can be relinked exactly on
 * backtrack. That is the "dancing" of Dancing Links, drawn on the board.
 *
 * Same logic as Algorithm X: pick the most constrained square (fewest candidates, like the
 * smallest column), try each candidate, cover (unlink the square and remove the digit from
 * the squares it sees), recurse, and uncover on backtrack.
 *
 * Simplification (stated in the UI): the squares are the 81 "cell" constraints. The row, column
 * and box digit constraints appear as candidate removals: row and column peers are reached by
 * following pointers; box peers are updated by box index.
 *
 * Pointer targets are never null. Row ends wrap to a row header (nodes 81..89) and column ends
 * wrap to a column header (nodes 90..98), like the header of a circular list.
 *
 * Event: { phase, text, glows: [{ node, dir, kind }], focus, depth, move?, place?, unplace? }
 * Pure logic, no DOM: also loaded by the Node tests.
 */
(function () {
  'use strict';
  const root = typeof window !== 'undefined' ? window : globalThis;
  const DLX = root.DLX || (root.DLX = {});
  const ROW_H = 81, COL_H = 90, COUNT = 99;
  const FULL = 0x3fe;

  const popcount = (m) => { let n = 0; while (m) { m &= m - 1; n++; } return n; };
  const nameOf = (i) => (i < 81 ? `(${Math.floor(i / 9) + 1},${(i % 9) + 1})` : i < COL_H ? `row ${i - ROW_H + 1} header` : `column ${i - COL_H + 1} header`);

  function create(grid) {
    const L = new Array(COUNT), R = new Array(COUNT), U = new Array(COUNT), D = new Array(COUNT);
    for (let i = 0; i < 81; i++) {
      const r = Math.floor(i / 9), c = i % 9;
      L[i] = c > 0 ? i - 1 : ROW_H + r;
      R[i] = c < 8 ? i + 1 : ROW_H + r;
      U[i] = r > 0 ? i - 9 : COL_H + c;
      D[i] = r < 8 ? i + 9 : COL_H + c;
    }
    for (let r = 0; r < 9; r++) {
      const h = ROW_H + r;
      R[h] = r * 9; L[h] = r * 9 + 8; U[h] = D[h] = h;
    }
    for (let c = 0; c < 9; c++) {
      const h = COL_H + c;
      D[h] = c; U[h] = 72 + c; L[h] = R[h] = h;
    }
    const digit = new Array(81).fill(0), given = new Array(81).fill(false), mask = new Array(81).fill(0);
    const rows = new Array(9).fill(0), cols = new Array(9).fill(0), boxes = new Array(9).fill(0);
    for (let i = 0; i < 81; i++) {
      const r = Math.floor(i / 9), c = i % 9, d = grid[r][c];
      if (d) {
        digit[i] = d; given[i] = true;
        const bit = 1 << d;
        rows[r] |= bit; cols[c] |= bit; boxes[Math.floor(r / 3) * 3 + Math.floor(c / 3)] |= bit;
      }
    }
    for (let i = 0; i < 81; i++) {
      if (given[i]) continue;
      const r = Math.floor(i / 9), c = i % 9;
      mask[i] = FULL & ~(rows[r] | cols[c] | boxes[Math.floor(r / 3) * 3 + Math.floor(c / 3)]);
    }
    return { L, R, U, D, digit, given, mask, count: COUNT, name: nameOf };
  }

  /** Row axis (L/R) and column axis (U/D) are tracked separately so a half-finished unlink looks right. */
  const rowLinked = (g, i) => i >= 81 || g.R[g.L[i]] === i;
  const colLinked = (g, i) => i >= 81 || g.D[g.U[i]] === i;
  const isLinked = (g, i) => rowLinked(g, i) || colLinked(g, i);

  function* run(g) {
    const { L, R, U, D, digit, given, mask } = g;
    const rd = (node, dir) => ({ node, dir, kind: 'read' });
    const wr = (node, dir) => ({ node, dir, kind: 'write' });
    let depth = 0;
    const ev = (phase, text, glows, focus, extra) => ({ phase, text, glows, focus, depth, ...extra });
    const boxPeers = (i) => {
      const r = Math.floor(i / 9), c = i % 9, br = r - (r % 3), bc = c - (c % 3), out = [];
      for (let a = br; a < br + 3; a++) for (let b = bc; b < bc + 3; b++) if (a !== r && b !== c) out.push(a * 9 + b);
      return out;
    };

    /** Unlink square i from the mesh: its four neighbours skip over it. Its own pointers stay. */
    function* unlink(i, why) {
      yield ev('cover', `${why} Read L and R of ${nameOf(i)}: its left and right neighbours.`, [rd(i, 'L'), rd(i, 'R')], i);
      R[L[i]] = R[i];
      yield ev('cover', `R[L[${nameOf(i)}]] ← R[${nameOf(i)}]: the square on the left now points past ${nameOf(i)}.`, [wr(L[i], 'R')], L[i]);
      L[R[i]] = L[i];
      yield ev('cover', `L[R[${nameOf(i)}]] ← L[${nameOf(i)}]: the square on the right now points back past it.`, [wr(R[i], 'L')], R[i]);
      yield ev('cover', `Read U and D of ${nameOf(i)}: the squares above and below.`, [rd(i, 'U'), rd(i, 'D')], i);
      D[U[i]] = D[i];
      yield ev('cover', `D[U[${nameOf(i)}]] ← D[${nameOf(i)}]: the square above now points down past it.`, [wr(U[i], 'D')], U[i]);
      U[D[i]] = U[i];
      yield ev('cover', `U[D[${nameOf(i)}]] ← U[${nameOf(i)}]: the square below now points up past it. ${nameOf(i)} is out of the mesh but still remembers its neighbours.`, [wr(D[i], 'U')], D[i]);
    }

    /** Relink in exactly the reverse order. */
    function* relink(i) {
      U[D[i]] = i;
      yield ev('uncover', `U[D[${nameOf(i)}]] ← ${nameOf(i)}: the square below points up to it again.`, [wr(D[i], 'U')], D[i]);
      D[U[i]] = i;
      yield ev('uncover', `D[U[${nameOf(i)}]] ← ${nameOf(i)}: the square above points down to it again.`, [wr(U[i], 'D')], U[i]);
      L[R[i]] = i;
      yield ev('uncover', `L[R[${nameOf(i)}]] ← ${nameOf(i)}: the square on the right points back to it.`, [wr(R[i], 'L')], R[i]);
      R[L[i]] = i;
      yield ev('uncover', `R[L[${nameOf(i)}]] ← ${nameOf(i)}: the square on the left points to it. ${nameOf(i)} is back in the mesh.`, [wr(L[i], 'R')], L[i]);
    }

    /** Walk from i along one axis, applying `fn` to every square still in the mesh. */
    function* walk(i, dir, fn, label) {
      let prev = i, j = g[dir][i];
      while (j < 81) {
        const msg = fn(j);
        yield ev('cover', `Follow ${dir} from ${nameOf(prev)} to ${nameOf(j)}. ${msg}`, [rd(prev, dir)], j);
        prev = j; j = g[dir][j];
      }
      yield ev('cover', `Follow ${dir} from ${nameOf(prev)}: it points back to the ${label} header, so this direction is done.`, [rd(prev, dir)], prev);
    }

    // ---- setup: remove the given digits from the mesh ------------------------------------
    for (let i = 0; i < 81; i++) {
      if (!given[i]) continue;
      yield* unlink(i, `Given ${digit[i]} at ${nameOf(i)}.`);
    }
    yield ev('choose', 'All given digits are out of the mesh. Only empty squares remain linked. Their candidates were reduced by the givens.', [], 0);

    function* search() {
      // choose the empty square with the fewest candidates by following the R pointers
      let best = -1, bestN = 10;
      scan:
      for (let r = 0; r < 9; r++) {
        const h = ROW_H + r;
        let j = R[h];
        yield ev('choose', j === h ? `Row ${r + 1}: R from its header points back to the header, so no empty squares in this row.` : `Row ${r + 1}: follow R from the row header to its first empty square, ${nameOf(j)}.`, [rd(h, 'R')], j === h ? h : j);
        let prev = h;
        while (j < 81) {
          const n = popcount(mask[j]);
          const better = n < bestN;
          if (better) { best = j; bestN = n; }
          if (prev !== h) yield ev('choose', `Follow R from ${nameOf(prev)} to ${nameOf(j)}.`, [rd(prev, 'R')], j);
          yield ev('choose', `${nameOf(j)} has ${n} candidate${n === 1 ? '' : 's'}. ${better ? 'New smallest.' : `Not smaller than ${nameOf(best)} (${bestN}).`}`, [], j);
          if (bestN <= 1) break scan; // nothing can beat 0 or 1
          prev = j; j = R[j];
        }
      }
      if (best < 0) {
        yield ev('solution', 'Every row header points to itself: no empty squares are left. All squares are filled, so the puzzle is solved.', [rd(ROW_H, 'R')], ROW_H);
        return true;
      }
      if (bestN === 0) {
        yield ev('dead-end', `${nameOf(best)} has no candidates left. Dead end, so backtrack.`, [], best, { move: true });
        return false;
      }
      yield ev('choose', `Choose ${nameOf(best)}: ${bestN} candidate${bestN === 1 ? '' : 's'}, the fewest, so it fails fastest if it must.`, [], best);

      const cands = [];
      for (let d = 1; d <= 9; d++) if (mask[best] & (1 << d)) cands.push(d);
      for (const d of cands) {
        digit[best] = d;
        yield ev('try', `Try ${d} at ${nameOf(best)}.`, [], best, { place: { i: best, d }, move: true });
        yield* unlink(best, `Place ${d}.`);

        // remove d from the squares it sees: row and column peers via pointers, box peers by index
        const changed = [];
        const strip = (j) => {
          if (mask[j] & (1 << d)) { mask[j] &= ~(1 << d); changed.push(j); return `Candidate ${d} removed (${popcount(mask[j])} left).`; }
          return `${d} was not a candidate here.`;
        };
        yield* walk(best, 'R', strip, 'row');
        yield* walk(best, 'L', strip, 'row');
        yield* walk(best, 'D', strip, 'column');
        yield* walk(best, 'U', strip, 'column');
        const boxChanged = [];
        for (const j of boxPeers(best)) {
          if (j < 81 && !given[j] && digit[j] === 0 && mask[j] & (1 << d)) { mask[j] &= ~(1 << d); boxChanged.push(j); }
        }
        yield ev('cover', `Squares in the same 3×3 box that are still empty also lose candidate ${d}${boxChanged.length ? ` (${boxChanged.length} changed)` : ''}. Box peers are checked by box, not by following pointers.`, [], best);

        depth++;
        const ok = yield* search();
        depth--;
        if (ok) return true;

        // backtrack: restore candidates, then relink
        for (const j of boxChanged) mask[j] |= 1 << d;
        yield ev('uncover', `Backtrack: restore candidate ${d} in the box peers.`, [], best);
        const undo = (j) => { if (changed.includes(j)) { mask[j] |= 1 << d; return `Candidate ${d} restored.`; } return 'Nothing to restore.'; };
        yield* walk(best, 'U', undo, 'column');
        yield* walk(best, 'D', undo, 'column');
        yield* walk(best, 'L', undo, 'row');
        yield* walk(best, 'R', undo, 'row');
        yield* relink(best);
        digit[best] = 0;
        yield ev('backtrack', `Remove ${d} from ${nameOf(best)}. ${cands.indexOf(d) < cands.length - 1 ? 'Try the next candidate.' : 'No candidates left here; go back one level.'}`, [], best, { unplace: { i: best }, move: true });
      }
      return false;
    }

    return yield* search();
  }

  DLX.algoGrid = { create, run, isLinked, rowLinked, colLinked, nameOf, ROW_H, COL_H, COUNT };
})();
