#!/usr/bin/env python3
"""Generate 48x48 RGB565 fruit sprite headers for the SlotMachine firmware."""
import math, os

W = H = 48

def rgb565(r, g, b):
    r, g, b = max(0,min(255,int(r))), max(0,min(255,int(g))), max(0,min(255,int(b)))
    v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return ((v & 0xFF) << 8) | (v >> 8)   # byte-swap for M5GFX

def lerp(a, b, t):
    t = max(0.0, min(1.0, t))
    return a + (b - a) * t

def lerp_col(c1, c2, t):
    return tuple(lerp(c1[i], c2[i], t) for i in range(3))

BG = (0, 0, 0)

def aa_circle(x, y, cx, cy, r, col, aa=0.8):
    d = math.sqrt((x-cx)**2 + (y-cy)**2)
    if d <= r - aa: return col
    if d <= r + aa: return lerp_col(col, BG, (d-(r-aa))/(2*aa))
    return None

def aa_ellipse(x, y, cx, cy, rx, ry, col, aa=0.8):
    nx, ny = (x-cx)/rx, (y-cy)/ry
    d = math.sqrt(nx*nx + ny*ny)
    s = aa / min(rx, ry)
    if d <= 1-s: return col
    if d <= 1+s: return lerp_col(col, BG, (d-(1-s))/(2*s))
    return None

def put(canvas, x, y, col):
    if col and 0 <= x < W and 0 <= y < H:
        canvas[y * W + x] = col

def apply(canvas, x, y, col):
    if col is not None and 0 <= x < W and 0 <= y < H:
        canvas[y * W + x] = col

def draw_circle(canvas, cx, cy, r, col):
    for py in range(max(0,int(cy-r-2)), min(H,int(cy+r+2))):
        for px in range(max(0,int(cx-r-2)), min(W,int(cx+r+2))):
            c = aa_circle(px, py, cx, cy, r, col)
            if c: canvas[py*W+px] = c

def draw_ellipse(canvas, cx, cy, rx, ry, col):
    for py in range(max(0,int(cy-ry-2)), min(H,int(cy+ry+2))):
        for px in range(max(0,int(cx-rx-2)), min(W,int(cx+rx+2))):
            c = aa_ellipse(px, py, cx, cy, rx, ry, col)
            if c: canvas[py*W+px] = c

def rect(canvas, x1, y1, x2, y2, col):
    for py in range(max(0,y1), min(H,y2+1)):
        for px in range(max(0,x1), min(W,x2+1)):
            canvas[py*W+px] = col

# ── Cherry ────────────────────────────────────────────────────────────────────
def cherry():
    cv = [BG] * (W*H)
    # stems
    rect(cv, 21,3,26,18, (60,180,60))
    rect(cv, 14,17,22,20, (60,180,60))
    rect(cv, 25,17,34,20, (60,180,60))
    # leaf
    draw_ellipse(cv, 24,8, 7,4, (34,139,34))
    # berries (shadow then main then highlight)
    draw_circle(cv, 15,33, 12, (140,8,25))
    draw_circle(cv, 33,33, 12, (140,8,25))
    draw_circle(cv, 15,32, 11, (220,20,50))
    draw_circle(cv, 33,32, 11, (220,20,50))
    draw_circle(cv, 11,27,  3, (255,170,170))
    draw_circle(cv, 29,27,  3, (255,170,170))
    return [rgb565(*c) for c in cv]

# ── Lemon ─────────────────────────────────────────────────────────────────────
def lemon():
    cv = [BG] * (W*H)
    draw_ellipse(cv, 24,9, 7,4, (34,139,34))       # leaf
    rect(cv, 22,13,25,17, (110,75,20))              # stem
    draw_ellipse(cv, 24,30, 19,14, (200,165,0))     # body dark rim
    draw_ellipse(cv, 24,30, 17,12, (255,220,0))     # body main
    draw_ellipse(cv,  5,30,  7, 5, (200,165,0))     # left tip dark
    draw_ellipse(cv,  5,30,  5, 3, (255,220,0))     # left tip
    draw_ellipse(cv, 43,30,  7, 5, (200,165,0))     # right tip dark
    draw_ellipse(cv, 43,30,  5, 3, (255,220,0))     # right tip
    draw_ellipse(cv, 17,23,  5, 3, (255,250,190))   # highlight
    return [rgb565(*c) for c in cv]

# ── Orange ────────────────────────────────────────────────────────────────────
def orange():
    cv = [BG] * (W*H)
    draw_ellipse(cv, 32,8, 7,4, (34,139,34))        # leaf
    rect(cv, 22,4,25,11, (110,75,20))               # stem
    draw_circle(cv, 24,28, 17, (190,85,0))          # dark rim
    draw_circle(cv, 24,28, 15, (255,130,0))         # main
    draw_circle(cv, 17,20,  4, (255,220,160))       # highlight
    draw_circle(cv, 24,42,  3, (180,70,0))          # navel
    return [rgb565(*c) for c in cv]

# ── Grape ─────────────────────────────────────────────────────────────────────
def grape():
    cv = [BG] * (W*H)
    grapes = [(24,14),(16,24),(32,24),(11,34),(24,34),(37,34)]
    draw_ellipse(cv, 30,5, 6,4, (34,139,34))        # leaf
    rect(cv, 22,2,25,8, (110,75,20))                # stem
    for gx,gy in grapes:
        draw_circle(cv, gx,gy, 9, (70,0,130))       # shadow
    for gx,gy in grapes:
        draw_circle(cv, gx,gy, 8, (148,50,240))     # main
    for gx,gy in grapes:
        draw_circle(cv, gx-2,gy-2, 2, (210,160,255)) # gloss
    return [rgb565(*c) for c in cv]

# ── Watermelon ────────────────────────────────────────────────────────────────
def watermelon():
    cv = [BG] * (W*H)
    CX, CY = 24, 36
    # Draw only pixels in lower semicircle (y >= 16)
    for py in range(16, H):
        for px in range(W):
            d = math.sqrt((px-CX)**2 + (py-CY)**2)
            if d <= 17+0.8:
                # pick color by zone
                if d >= 15:
                    col = lerp_col((30,140,30), BG, max(0,(d-15)/3))
                elif d >= 13:
                    col = (220,240,210)
                elif d >= 1:
                    t = max(0, (d-11)/2)
                    col = lerp_col((225,40,60), (160,20,40), t)
                else:
                    col = (225,40,60)
                cv[py*W+px] = col
    # flat top edge
    rect(cv, int(CX-17),15, int(CX+17),17, (50,170,50))
    # seeds
    seeds = [(19,36),(24,41),(29,36),(22,43),(26,43)]
    for sx,sy in seeds:
        draw_ellipse(cv, sx,sy, 2,3, (20,20,20))
    return [rgb565(*c) for c in cv]

# ── Seven ─────────────────────────────────────────────────────────────────────
def seven():
    cv = [BG] * (W*H)
    RED  = (225,20,20)
    GOLD = (255,200,0)
    # top bar with gold border
    rect(cv, 9,7,38,13, RED)
    rect(cv, 9,7,38, 8, GOLD)   # top edge gold
    rect(cv, 9,12,38,13, GOLD)  # bottom edge gold
    # diagonal stroke (thick)
    for py in range(13, 44):
        cx_d = 38 - (py-13)*22//30
        rect(cv, cx_d-4, py, cx_d+3, py, RED)
        put(cv, cx_d-4, py, GOLD)
        put(cv, cx_d+3, py, GOLD)
    # small serif at bottom-left
    rect(cv, 10,40,18,43, RED)
    return [rgb565(*c) for c in cv]

# ── Bell ──────────────────────────────────────────────────────────────────────
def bell():
    cv = [BG] * (W*H)
    YELLOW      = (255,210,0)
    YELLOW_DARK = (200,150,0)
    BROWN       = (130,80,20)
    # bell crown (small circle at top)
    draw_circle(cv, 24,8, 3, BROWN)
    # bell body — upper dome
    draw_ellipse(cv, 24,20, 14,12, YELLOW_DARK)
    draw_ellipse(cv, 24,20, 12,10, YELLOW)
    # bell lower half (wider)
    draw_ellipse(cv, 24,32, 17,10, YELLOW_DARK)
    draw_ellipse(cv, 24,32, 15, 8, YELLOW)
    # rim (horizontal bar at bottom of bell)
    draw_ellipse(cv, 24,41, 17,4, YELLOW_DARK)
    draw_ellipse(cv, 24,41, 15,3, YELLOW)
    # clapper (small circle below rim)
    draw_circle(cv, 24,46, 2, BROWN)
    # highlight
    draw_ellipse(cv, 18,15, 4,3, (255,250,200))
    return [rgb565(*c) for c in cv]

# ── Write headers ─────────────────────────────────────────────────────────────
def write_header(path, varname, pixels):
    with open(path, 'w') as f:
        f.write("#pragma once\n#include <pgmspace.h>\n\n")
        f.write(f"static const uint16_t {varname}[] PROGMEM = {{\n")
        for i in range(0, len(pixels), 12):
            chunk = pixels[i:i+12]
            f.write("  " + ", ".join(f"0x{p:04X}" for p in chunk) + ",\n")
        f.write("};\n")
    print(f"  {varname}: {len(pixels)*2} bytes")

OUT = r"C:\Users\Yersin\SlotMachine\src"
print("Generating sprites...")
write_header(f"{OUT}/img_cherry.h",     "img_cherry",     cherry())
write_header(f"{OUT}/img_lemon.h",      "img_lemon",      lemon())
write_header(f"{OUT}/img_orange.h",     "img_orange",     orange())
write_header(f"{OUT}/img_grape.h",      "img_grape",      grape())
write_header(f"{OUT}/img_watermelon.h", "img_watermelon", watermelon())
write_header(f"{OUT}/img_seven.h",      "img_seven",      seven())
write_header(f"{OUT}/img_bell.h",       "img_bell",       bell())
print("Done.")
