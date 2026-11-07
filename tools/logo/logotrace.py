"""logotrace.py - the marquee's subtitle as vector lines (after Tempest Enhanced's).

    python tools/logo/logotrace.py [tools/logo/mhavoc_marquee.png] [src/render/logo_shape.h] [preview.svg]

Writes the C header render/logo.c draws the attract mode's subtitle with:
the marquee's PROMISED END: Enhanced in place of the game's small THE
PROMISED END under its own MAJOR HAVOC (which stays, see render/logo.h).
The picture (the owner's marquee, 2000 x 375; the first one, 2088 x 520, in _scratch/marquee) is read through ImageMagick
(magick on the PATH) and split into masks with numpy:

  promised  PROMISED END: (white letters), traced as outlines
  enhanced  Enhanced (yellow letters), traced as outlines
  (the panel's red frame is left out: broken up by the maze behind)

The points are in subtitle units: x across the lettering from -0.5 to 0.5,
y down, 0 its middle. The paths are sorted left to right within each part,
so that the beam goes across once.

LOGO_OVERLAY=FILE.png draws the paths over the marquee, for checking.
"""
import os
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, 'mhavoc_marquee.png')
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'src', 'render', 'logo_shape.h')
SVG = sys.argv[3] if len(sys.argv) > 3 else None
OVERLAY = os.environ.get('LOGO_OVERLAY')
NL = chr(10)

# regions of the marquee (pixels: x0, y0, x1, y1); the line rises to the right
# (the owner's second marquee, 2000 x 375: PROMISED END: Enhanced on one line)
PANEL_BOX = (1170, 195, 1975, 345)      # the subtitle's panel
PROMISED_BOX = (1190, 245, 1556, 335)
ENHANCED_BOX = (1545, 200, 1945, 312)

EPS = 1.3           # simplification, pixels
MIN_AREA = 40       # outlines smaller than this (pixels) are noise
MIN_FRAME = 60      # the frame's lines: only the long ones


def load(path):
    tmp = tempfile.mkdtemp()
    ppm = os.path.join(tmp, 'm.ppm')
    subprocess.run(['magick', path, '-alpha', 'off', '-depth', '8', ppm], check=True)
    with open(ppm, 'rb') as f:
        data = f.read()
    parts, i = [], 0
    while len(parts) < 4:
        while data[i:i + 1].isspace():
            i += 1
        j = i
        while not data[j:j + 1].isspace():
            j += 1
        parts.append(data[i:j])
        i = j
    i += 1
    w, h = int(parts[1]), int(parts[2])
    return np.frombuffer(data[i:i + w * h * 3], dtype=np.uint8).reshape(h, w, 3).astype(np.int32)


def box(m, b):
    out = np.zeros_like(m)
    x0, y0, x1, y1 = b
    out[y0:y1, x0:x1] = m[y0:y1, x0:x1]
    return out


def disk(r):
    return [(dx, dy) for dx in range(-r, r + 1) for dy in range(-r, r + 1) if dx * dx + dy * dy <= r * r]


def shift(m, dx, dy):
    out = np.zeros_like(m)
    h, w = m.shape
    xs0, xs1 = max(0, -dx), min(w, w - dx)
    ys0, ys1 = max(0, -dy), min(h, h - dy)
    out[ys0 + dy:ys1 + dy, xs0 + dx:xs1 + dx] = m[ys0:ys1, xs0:xs1]
    return out


def dilate(m, r):
    out = np.zeros_like(m)
    for dx, dy in disk(r):
        out |= shift(m, dx, dy)
    return out


def erode(m, r):
    out = np.ones_like(m)
    for dx, dy in disk(r):
        out &= shift(m, dx, dy)
    return out


def opening(m, r):
    return dilate(erode(m, r), r)


# --- outlines (pixels' edges), from Tempest Enhanced's logotrace.py ------------

def outlines(m):
    ys, xs = np.nonzero(m)
    on = set(zip(xs.tolist(), ys.tolist()))
    nxt = {}
    for x, y in on:
        if (x, y - 1) not in on: nxt.setdefault((x + 1, y), []).append((x, y))
        if (x - 1, y) not in on: nxt.setdefault((x, y), []).append((x, y + 1))
        if (x, y + 1) not in on: nxt.setdefault((x, y + 1), []).append((x + 1, y + 1))
        if (x + 1, y) not in on: nxt.setdefault((x + 1, y + 1), []).append((x + 1, y))
    loops = []
    while nxt:
        start = next(iter(nxt))
        loop = [start]
        cur, prev = start, None
        while True:
            outs = nxt.get(cur)
            if not outs:
                break
            if len(outs) == 1 or prev is None:
                n = outs[0]
            else:
                dx, dy = cur[0] - prev[0], cur[1] - prev[1]
                n = min(outs, key=lambda p: dx * (p[1] - cur[1]) - dy * (p[0] - cur[0]))
            outs.remove(n)
            if not outs:
                del nxt[cur]
            prev, cur = cur, n
            if cur == start:
                break
            loop.append(cur)
        if len(loop) > 2:
            loops.append(loop)
    return loops


def area(loop):
    a = 0.0
    for i in range(len(loop)):
        x0, y0 = loop[i - 1]
        x1, y1 = loop[i]
        a += x0 * y1 - x1 * y0
    return a * 0.5


def dp(pts, eps):
    if len(pts) < 3:
        return list(pts)
    keep = [False] * len(pts)
    keep[0] = keep[-1] = True
    stack = [(0, len(pts) - 1)]
    while stack:
        a, b = stack.pop()
        ax, ay = pts[a]
        bx, by = pts[b]
        dx, dy = bx - ax, by - ay
        ln = (dx * dx + dy * dy) ** 0.5
        best, bi = -1.0, -1
        for i in range(a + 1, b):
            px, py = pts[i]
            d = ((px - ax) ** 2 + (py - ay) ** 2) ** 0.5 if ln < 1e-9 else abs(dx * (ay - py) - dy * (ax - px)) / ln
            if d > best:
                best, bi = d, i
        if best > eps:
            keep[bi] = True
            stack.append((a, bi))
            stack.append((bi, b))
    return [p for p, k in zip(pts, keep) if k]


def simplify_loop(loop, eps):
    mid = [((loop[i - 1][0] + loop[i][0]) * 0.5, (loop[i - 1][1] + loop[i][1]) * 0.5) for i in range(len(loop))]
    far = max(range(len(mid)), key=lambda i: (mid[i][0] - mid[0][0]) ** 2 + (mid[i][1] - mid[0][1]) ** 2)
    a = dp(mid[:far + 1], eps)
    b = dp(mid[far:] + [mid[0]], eps)
    return a[:-1] + b[:-1]


# --- middle lines -----------------------------------------------------------

def thin(m):
    """Zhang-Suen thinning, vectorised."""
    m = m.astype(np.uint8).copy()
    while True:
        changed = False
        for step in (0, 1):
            p = [shift(m, 0, 1), shift(m, -1, 1), shift(m, -1, 0), shift(m, -1, -1),
                 shift(m, 0, -1), shift(m, 1, -1), shift(m, 1, 0), shift(m, 1, 1)]
            b = sum(q.astype(np.int32) for q in p)
            a = sum(((p[k] == 0) & (p[(k + 1) % 8] == 1)).astype(np.int32) for k in range(8))
            if step == 0:
                c = (p[0] * p[2] * p[4] == 0) & (p[2] * p[4] * p[6] == 0)
            else:
                c = (p[0] * p[2] * p[6] == 0) & (p[0] * p[4] * p[6] == 0)
            gone = (m == 1) & (b >= 2) & (b <= 6) & (a == 1) & c
            if gone.any():
                m[gone] = 0
                changed = True
        if not changed:
            return m.astype(bool)


def chains(m):
    ys, xs = np.nonzero(m)
    on = set(zip(xs.tolist(), ys.tolist()))
    def nb(p):
        x, y = p
        return [(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if (dx or dy) and (x + dx, y + dy) in on]
    deg = {p: len(nb(p)) for p in on}
    used, out = set(), []
    starts = [p for p in on if deg[p] != 2] + [p for p in on if deg[p] == 2]
    for s in starts:
        for q in nb(s):
            e = frozenset((s, q))
            if e in used:
                continue
            line = [s, q]
            used.add(e)
            prev, cur = s, q
            while deg[cur] == 2:
                nx = [r for r in nb(cur) if r != prev and frozenset((cur, r)) not in used]
                if not nx:
                    break
                used.add(frozenset((cur, nx[0])))
                prev, cur = cur, nx[0]
                line.append(cur)
                if cur == s:
                    break
            out.append(line)
    return out


def length(pts):
    return sum(((pts[i][0] - pts[i - 1][0]) ** 2 + (pts[i][1] - pts[i - 1][1]) ** 2) ** 0.5 for i in range(1, len(pts)))


def traced(m, colour, part, paths, min_h=0):
    for loop in outlines(m):
        if abs(area(loop)) < MIN_AREA:
            continue
        h = max(p[1] for p in loop) - min(p[1] for p in loop)
        w = max(p[0] for p in loop) - min(p[0] for p in loop)
        if h < min_h and w > 3 * h or abs(area(loop)) < 4.0 * max(h, w):
            continue        # (a thin line, not a letter; the letters' holes and the colon's dots stay)
        pts = simplify_loop(loop, EPS)
        if len(pts) >= 3:
            paths.append((pts, 1, colour, part))


def middles(m, colour, part, paths, min_len):
    for line in chains(thin(m)):
        if length(line) < min_len:
            continue
        pts = dp([(p[0] + 0.5, p[1] + 0.5) for p in line], EPS)
        paths.append((pts, 0, colour, part))


def frame_lines(m):
    """The panel's frame as a parallelogram: its red lines, near-level ones
    giving the top and the bottom (one slope, the outermost of each), steep
    ones the left and the right (the lettering's slant). Corners, clockwise
    from the top left."""
    import math
    level, steep = [], []
    for line in chains(thin(m)):
        if length(line) < MIN_FRAME:
            continue
        (x0, y0), (x1, y1) = line[0], line[-1]
        if abs(x1 - x0) > 3 * abs(y1 - y0):
            level.append(line)
        else:
            steep.append(line)
    a = sorted((l[-1][1] - l[0][1]) / (l[-1][0] - l[0][0]) for l in level)[len(level) // 2]
    bs = [p[1] - a * p[0] for l in level for p in l]
    top, bottom = min(bs), max(bs)
    sl = sorted((l[-1][0] - l[0][0]) / (l[-1][1] - l[0][1]) for l in steep)[len(steep) // 2]
    cs = [p[0] - sl * p[1] for l in steep for p in l]
    left, right = min(cs), max(cs)
    def corner(b, c):
        # y = a x + b, x = sl y + c
        y = (a * c + b) / (1.0 - a * sl)
        return (sl * y + c, y)
    return [corner(top, left), corner(top, right), corner(bottom, right), corner(bottom, left)]


def inset(poly, d):
    """The parallelogram moved in by d (pixels) on each side."""
    cx = sum(p[0] for p in poly) / 4.0
    cy = sum(p[1] for p in poly) / 4.0
    out = []
    for x, y in poly:
        vx, vy = cx - x, cy - y
        n = (vx * vx + vy * vy) ** 0.5
        out.append((x + vx / n * d * 1.6, y + vy / n * d * 1.6))
    return out


def main():
    img = load(SRC)
    r, g, b = img[:, :, 0], img[:, :, 1], img[:, :, 2]
    red = (r > 150) & (g < 110) & (b < 110)
    white = (r > 190) & (g > 190) & (b > 190)
    yellow = (r > 170) & (g > 170) & (b < 110)

    promised = opening(box(white, PROMISED_BOX), 1)
    enhanced = opening(box(yellow, ENHANCED_BOX), 1)
    # the panel's frame: its thin red lines (the letters above are thick)
    frame = box(red, PANEL_BOX) & ~dilate(opening(red, 4), 2)

    paths = []
    traced(promised, (255, 255, 255), 0, paths, 25)
    traced(enhanced, (255, 230, 40), 1, paths, 25)
    # (the panel's frame is left out: its lines are too broken up by the
    # maze behind to fit cleanly)

    if OVERLAY:
        mvg = os.path.join(tempfile.mkdtemp(), 'o.mvg')
        with open(mvg, 'w') as f:
            f.write('fill none stroke-width 2' + NL)
            for q in paths:
                f.write('stroke rgb(0,255,0) %s %s%s' % ('polygon' if q[1] else 'polyline',
                                                        ' '.join('%.1f,%.1f' % p for p in q[0]), NL))
        subprocess.run(['magick', SRC, '-fill', 'none', '-draw', '@' + mvg, OVERLAY], check=True)

    ys, xs = np.nonzero(promised | enhanced)
    tx0, tx1, ty0, ty1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
    cx, cy, s = (tx0 + tx1) * 0.5, (ty0 + ty1) * 0.5, 1.0 / (tx1 - tx0)

    def unit(p):
        return ((p[0] - cx) * s, (p[1] - cy) * s)

    paths = [([unit(p) for p in q[0]], q[1], q[2], q[3]) for q in paths]
    paths.sort(key=lambda q: (q[3], min(p[0] for p in q[0])))
    npts = sum(len(q[0]) for q in paths)
    segs = sum(len(q[0]) - (0 if q[1] else 1) for q in paths)
    allx = [p[0] for q in paths for p in q[0]]
    ally = [p[1] for q in paths for p in q[0]]
    out = []
    out.append('/* logo_shape.h - generated by tools/logo/logotrace.py from tools/logo/mhavoc_marquee.png; do not edit.')
    out.append(" * The marquee's subtitle in its own units (x -0.5..0.5 across the lettering, y down, 0 its")
    out.append(" * middle). Parts: 0 PROMISED END: (white), 1 Enhanced (yellow);")
    out.append(' * each left to right. */')
    out.append('#ifndef MH_LOGO_SHAPE_H')
    out.append('#define MH_LOGO_SHAPE_H')
    out.append('')
    out.append('#define MH_SUB_X0 %.4ff' % min(allx))
    out.append('#define MH_SUB_X1 %.4ff' % max(allx))
    out.append('#define MH_SUB_Y0 %.4ff' % min(ally))
    out.append('#define MH_SUB_Y1 %.4ff' % max(ally))
    out.append('#define MH_SUB_PATHS %d' % len(paths))
    out.append('#define MH_SUB_SEGS %d' % segs)
    out.append('')
    out.append('typedef struct mh_sub_path { unsigned short first, n; unsigned char closed, part, r, g, b; } mh_sub_path;')
    out.append('')
    out.append('static const float MH_SUB_PT[%d][2] = {' % npts)
    for q in paths:
        out.append('   ' + ''.join(' { %.4ff, %.4ff },' % p for p in q[0]))
    out.append('};')
    out.append('')
    out.append('static const mh_sub_path MH_SUB_PATH[MH_SUB_PATHS] = {')
    first = 0
    for q in paths:
        out.append('    { %d, %d, %d, %d, %d, %d, %d },' % (first, len(q[0]), q[1], q[3], q[2][0], q[2][1], q[2][2]))
        first += len(q[0])
    out.append('};')
    out.append('')
    out.append('#endif')
    with open(OUT, 'w', newline=NL) as f:
        f.write(NL.join(out) + NL)
    counts = [sum(1 for q in paths if q[3] == k) for k in range(2)]
    print('%s: %d paths (%s), %d segments' % (OUT, len(paths), ' '.join(map(str, counts)), segs))
    if SVG:
        with open(SVG, 'w') as f:
            f.write("<svg xmlns='http://www.w3.org/2000/svg' viewBox='-0.6 -0.25 1.2 0.5' width='1440' height='600'>"
                    "<rect x='-1' y='-1' width='2' height='2' fill='black'/>" + NL)
            for q in paths:
                pts = ' '.join('%.4f,%.4f' % p for p in q[0])
                tag = 'polygon' if q[1] else 'polyline'
                f.write("<%s points='%s' fill='none' stroke='rgb(%d,%d,%d)' stroke-width='0.002'/>%s" % (tag, pts, *q[2], NL))
            f.write('</svg>' + NL)


if __name__ == '__main__':
    main()
