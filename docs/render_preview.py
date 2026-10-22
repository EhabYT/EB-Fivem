#!/usr/bin/env python3
"""Regenerates docs/preview.png - the redENGINE menu shell, Executor tab open.

Self-contained on purpose: it needs only Pillow and the repository itself. The
sidebar/button glyphs come from the project's *own* embedded Font Awesome font
(font_awesome.cpp stores it as an stb-compressed blob, decompressed here with a
port of imgui_draw.cpp's stb_decompress), so the icons are the real ones.

Every colour, size and label is read from the repository's own constants, not
invented:

  menu_style.h   the two theme ramps and every ImGuiCol_* below
  menu.h         SidebarWidth (225), SidebarButtonHeight (46), ContentPadding (8), Accent
  menu.cpp       Begin("redENGINE", NoScrollbar|NoCollapse|NoResize), SidebarItems[]
                 labels, logo 76x87 centred at CursorPosX(74), bottom-anchored footer
  exec.cpp       editor size (avail.x-16, avail.y-52), Execute 116x30,
                 Load from File 180x30, resource combo,
                 status line (Ln/Col/lines) between editor and buttons
  gui.h          PrimaryButton fill (rest #3778B5, hover #5D9CD6, press #1D5188)
  toasts.cpp     card metrics: 320 max width, 3px kind bar, 8px inner padding
  menu.cpp       drop shadow (three stacked rects), brand rule under the title
                 bar, toast stack anchored 10px off the window's bottom-right
  TextEditor.cpp GetDarkPalette(), mLeftMargin (10), mLineSpacing (1.0)

Two things CANNOT be faithful and are substituted, exactly as the README caption
says: Bahnschrift.ttf is a Windows-only font (a horizontally condensed DejaVu
Sans stands in for its DIN advance widths), and ImGui's layout engine is not
running, so positions follow the constants above. This is a render, not a
screenshot.

Usage:  python docs/render_preview.py      (requires Pillow)
"""
import os
import re
import struct
import tempfile
import zlib

from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(REPO, "docs", "preview.png")

S = 2  # DPI scale: ImGui would render the same at 2x

# ---------------------------------------------------------------------------
# the project's embedded Font Awesome font (font_awesome.cpp, stb-compressed)
# ---------------------------------------------------------------------------
def _in2(d, x): return (d[x] << 8) + d[x + 1]
def _in3(d, x): return (d[x] << 16) + _in2(d, x + 1)
def _in4(d, x): return (d[x] << 24) + _in3(d, x + 1)


def extract_icon_font():
    """Decompress font_awesome.cpp's blob; returns a path to a real .ttf."""
    src = open(os.path.join(REPO, "font_awesome.cpp"), encoding="utf-8", errors="replace").read()
    size = int(re.search(r"font_awesome_size\s*=\s*(\d+)", src).group(1))
    vals = [int(v, 16) for v in re.findall(r"0x([0-9a-fA-F]{8})", src[src.index("font_awesome_data"):])]
    blob = b"".join(struct.pack("<I", v) for v in vals)[:size]

    # stb_decompress, ported from imgui/imgui_draw.cpp
    assert _in4(blob, 0) == 0x57BC0000, "stb magic fehlt"
    assert _in4(blob, 4) == 0
    olen = (blob[8] << 24) + (blob[9] << 16) + (blob[10] << 8) + blob[11]

    def match(out, s, length):
        for k in range(length):
            out.append(out[s + k])

    out, p = bytearray(), 16
    while True:
        old, b0 = p, blob[p]
        if b0 >= 0x20:
            if b0 >= 0x80:
                match(out, len(out) - blob[p + 1] - 1, b0 - 0x80 + 1); p += 2
            elif b0 >= 0x40:
                match(out, len(out) - (_in2(blob, p) - 0x4000 + 1), blob[p + 2] + 1); p += 3
            else:
                L = b0 - 0x20 + 1; out.extend(blob[p + 1:p + 1 + L]); p += 1 + L
        else:
            if b0 >= 0x18:
                match(out, len(out) - (_in3(blob, p) - 0x180000 + 1), blob[p + 3] + 1); p += 4
            elif b0 >= 0x10:
                match(out, len(out) - (_in3(blob, p) - 0x100000 + 1), _in2(blob, p + 3) + 1); p += 5
            elif b0 >= 0x08:
                L = _in2(blob, p) - 0x0800 + 1; out.extend(blob[p + 2:p + 2 + L]); p += 2 + L
            elif b0 == 0x07:
                L = _in2(blob, p + 1) + 1; out.extend(blob[p + 3:p + 3 + L]); p += 3 + L
            elif b0 == 0x06:
                match(out, len(out) - (_in3(blob, p + 1) + 1), blob[p + 4] + 1); p += 5
            elif b0 == 0x04:
                match(out, len(out) - (_in3(blob, p + 1) + 1), _in2(blob, p + 4) + 1); p += 6
        if p == old:
            assert blob[p] == 0x05 and blob[p + 1] == 0xFA, "unerwarteter Terminator"
            assert len(out) == olen
            assert zlib.adler32(bytes(out), 1) == _in4(blob, p + 2), "Adler32 passt nicht"
            fd, path = tempfile.mkstemp(suffix=".ttf")
            os.write(fd, bytes(out)); os.close(fd)
            return path
        assert len(out) <= olen


# ---------------------------------------------------------------------------
# colours - menu_style.h, two ramps / six stops
#   interactive (light): #5D9CD6 accent -> #3778B5 hover -> #1D5188 pressed
#   surfaces   (navy)  : #2A4A7B title  -> #1F3A61 panel -> #14263F canvas
# ---------------------------------------------------------------------------
def RGBA(r, g, b, a=255): return (r, g, b, a)
def ABGR(v): return (v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF)
def blend(fg, bg):
    a = fg[3] / 255.0
    return tuple(int(round(fg[i] * a + bg[i] * (1 - a))) for i in range(3)) + (255,)

C_TITLE_BG   = RGBA(42, 74, 123)     # ImGuiCol_TitleBg / _Active   #2A4A7B
C_WINDOW_BG  = RGBA(20, 38, 63)      # ImGuiCol_WindowBg            #14263F
C_BUTTON     = RGBA(31, 58, 97)      # ImGuiCol_Button              #1F3A61
C_BUTTON_ACT = RGBA(29, 81, 136)     # ImGuiCol_ButtonActive        #1D5188
C_PRIMARY    = RGBA(55, 120, 181)     # Gui::PrimaryButton rest      #3778B5
C_ACCENT     = RGBA(93, 156, 214)    # Menu::Accent / SliderGrab    #5D9CD6
C_FRAME_BG   = RGBA(31, 58, 97)      # ImGuiCol_FrameBg             #1F3A61
C_BORDER     = RGBA(42, 74, 123)     # ImGuiCol_Border / _Separator #2A4A7B
C_TEXT       = RGBA(255, 255, 255)   # ImGuiCol_Text (default, not overridden)

# TextEditor.cpp GetDarkPalette(); Background/LineNumber are theme colours.
P = {
    "default":    RGBA(50, 50, 50),
    "keyword":    ABGR(0xFFD69C56),
    "number":     ABGR(0xFF00FF00),
    "string":     ABGR(0xFF7070E0),
    "charlit":    ABGR(0xFF70A0E0),
    "punct":      ABGR(0xFFFFFFFF),
    "preproc":    ABGR(0xFF408080),
    "ident":      ABGR(0xFFAAAAAA),
    "knownident": ABGR(0xFF9BC64D),
    "preprocid":  ABGR(0xFFC040A0),
    "comment":    ABGR(0xFF206020),
    "mlcomment":  ABGR(0xFF406020),
    "bg":         RGBA(31, 58, 97),      # #1F3A61
    "cursor":     ABGR(0xFFE0E0E0),
    "lineno":     RGBA(93, 156, 214),    # #5D9CD6
    "cline_inact": ABGR(0x40808080),
    "cline_edge":  ABGR(0x40A0A0A0),
}

# ---------------------------------------------------------------------------
# metrics from the source
# ---------------------------------------------------------------------------
FONT_SIZE, ICON_SIZE = 20, 19
FRAME_PAD_X, FRAME_PAD_Y = 8, 6
WIN_PAD, ITEM_SPACING_X, ITEM_SPACING_Y = 8, 8, 4

W, H = 900, 540                                  # style.WindowMinSize
TITLE_H = FONT_SIZE + FRAME_PAD_Y * 2            # 32
LINE_H = TITLE_H
LINE_ADV = LINE_H + ITEM_SPACING_Y               # 36
SIDEBAR_W, SIDEBAR_BTN = 225.0, 46.0
CONTENT_PAD = 8.0                                # Menu::ContentPadding

CY0, CY1 = TITLE_H + WIN_PAD, H - WIN_PAD
SX0, SX1 = WIN_PAD, int(WIN_PAD + SIDEBAR_W)
CX0, CX1 = SX1 + 1 + int(CONTENT_PAD), W - WIN_PAD

# Bahnschrift is DIN-narrow; condense the substitute text font to match its
# advance widths.  The real Font Awesome glyphs are NOT condensed.
CONDENSE = 0.86
TEXT_FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"

# ---------------------------------------------------------------------------
# Lua tokeniser - TextEditor.cpp LanguageDefinition::Lua()
# ---------------------------------------------------------------------------
LUA_KEYWORDS = {"and","break","do","else","elseif","end","false","for","function",
                "if","in","local","nil","not","or","repeat","return","then","true",
                "until","while"}
TOKEN_RES = [
    (re.compile(r'L?"(\\.|[^"])*"'), "string"),
    (re.compile(r"'[^']*'"), "string"),
    (re.compile(r"0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?"), "number"),
    (re.compile(r"[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?"), "number"),
    (re.compile(r"[+-]?[0-9]+[Uu]?[lL]?[lL]?"), "number"),
    (re.compile(r"[a-zA-Z_][a-zA-Z0-9_]*"), "ident"),
    (re.compile(r"[\[\]\{\}\!\%\^\&\*\(\)\-\+\=\~\|\<\>\?\/\;\,\.]"), "punct"),
]
# LanguageDefinition::Lua() has its mIdentifiers loop commented out, so nothing
# is ever coloured KnownIdentifier - print/ipairs/tostring stay plain grey.


def colorize(lines):
    out, in_ml = [], False
    for line in lines:
        runs, i, n = [], 0, len(line)
        while i < n:
            if in_ml:
                e = line.find("]]", i)
                if e == -1: runs.append((line[i:], "mlcomment")); i = n
                else: runs.append((line[i:e+2], "mlcomment")); i = e+2; in_ml = False
                continue
            if line.startswith("--[[", i):
                e = line.find("]]", i+4)
                if e == -1: runs.append((line[i:], "mlcomment")); i = n; in_ml = True
                else: runs.append((line[i:e+2], "mlcomment")); i = e+2
                continue
            if line.startswith("--", i): runs.append((line[i:], "comment")); i = n; continue
            if line[i].isspace():
                j = i
                while j < n and line[j].isspace(): j += 1
                runs.append((line[i:j], "default")); i = j; continue
            for rx, key in TOKEN_RES:
                m = rx.match(line, i)
                if m and m.end() > i:
                    txt = m.group(0)
                    if key == "ident" and txt in LUA_KEYWORDS: key = "keyword"
                    runs.append((txt, key)); i = m.end(); break
            else:
                runs.append((line[i], "default")); i += 1
        out.append(runs)
    return out


SAMPLE = [
    '-- redENGINE | Lua executor',
    'local players = GetActivePlayers()',
    '',
    'for _, id in ipairs(players) do',
    '    local ped = GetPlayerPed(id)',
    '    if DoesEntityExist(ped) then',
    '        print("player " .. tostring(id))',
    '    end',
    'end',
    '',
    'TriggerServerEvent("rE:sync", 0x1F, 3.14)',
]

# ---------------------------------------------------------------------------
# canvas
# ---------------------------------------------------------------------------
fa_path = extract_icon_font()
f_ui = ImageFont.truetype(TEXT_FONT, int(FONT_SIZE * S))
f_icon = ImageFont.truetype(fa_path, int(ICON_SIZE * S))

img = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)


def R(x0, y0, x1, y1, fill):
    d.rectangle([x0*S, y0*S, x1*S-1, y1*S-1], fill=fill)


def _run(s, font, fill, condense):
    bb = font.getbbox(s, anchor="la")
    adv = int(font.getlength(s))
    h = bb[3] + 2
    tmp = Image.new("RGBA", (adv + 2, h), (0, 0, 0, 0))
    ImageDraw.Draw(tmp).text((0, 0), s, font=font, fill=fill, anchor="la")
    cw = int(adv * condense)
    return tmp.resize((cw + 2, h), Image.LANCZOS), cw


def T(x, y, s, fill, font=f_ui, condense=CONDENSE):
    tmp, _ = _run(s, font, fill, condense)
    img.alpha_composite(tmp, (int(round(x*S)), int(round(y*S))))


def TW(s, font=f_ui, condense=CONDENSE):
    return font.getlength(s) * condense / S


# window + title bar (WindowTitleAlign 0,0.5; NoCollapse + p_open NULL)
R(0, 0, W, H, C_WINDOW_BG)
R(0, 0, W, TITLE_H, C_TITLE_BG)
T(WIN_PAD, (TITLE_H - FONT_SIZE) / 2, "redENGINE", C_TEXT)

# brand rule under the title bar (menu.cpp): accent -> pressed blue, left to right
for _x in range(W):
    _t = _x / (W - 1.0)
    R(_x, TITLE_H, _x + 1, TITLE_H + 2,
      tuple(int(round(C_ACCENT[i] + (C_BUTTON_ACT[i] - C_ACCENT[i]) * _t)) for i in range(3)) + (255,))

# 2-column table: BordersInnerV, SizingFixedFit, NoPadOuterX, CellPadding (0,0)
R(SX1, CY0, SX1 + 1, CY1, C_BORDER)

# sidebar: logo centred (74 == (225-76)/2) at CursorPosY + 9
logo = Image.open(os.path.join(REPO, "rE.png")).convert("RGBA")
logo = logo.resize((int(76*S), int(87*S)), Image.LANCZOS)
img.alpha_composite(logo, (int((SX0+74)*S), int((CY0+9)*S)))

SIDEBAR = [("\uf121", " Executor"), ("\uf019", " Resources"),
           ("\uf02d", " Menus"),    ("\uf085", " Config")]
ACTIVE = 0

sep_y = 170
R(SX0, sep_y, SX1, sep_y + 1, C_BORDER)
by = sep_y + 1
for i, (glyph, label) in enumerate(SIDEBAR):
    R(SX0, by, SX1, by + SIDEBAR_BTN, C_BUTTON_ACT if i == ACTIVE else C_BUTTON)
    if i == ACTIVE:
        R(SX0, by, SX0 + 3, by + SIDEBAR_BTN, C_ACCENT)      # selected-tab marker
    ty = by + (SIDEBAR_BTN - FONT_SIZE) / 2
    ix = SX0 + FRAME_PAD_X
    T(ix, ty + (FONT_SIZE - ICON_SIZE) / 2, glyph, C_TEXT, f_icon, 1.0)
    T(ix + TW(glyph, f_icon, 1.0), ty, label, C_TEXT)
    by += SIDEBAR_BTN
    R(SX0, by, SX1, by + 1, C_BORDER)
    by += 1

# footer: bottom-anchored, each line centred in the sidebar column
FOOTER_TOP = H - WIN_PAD - 3 * LINE_ADV
FOOTER = [("FPS 144  |  6.9 ms", C_ACCENT),
          ("Licensed to #4278", C_TEXT),
          ("Build Date: Oct 22 2026", C_TEXT)]
for k, (line, col) in enumerate(FOOTER):
    assert TW(line) <= SIDEBAR_W, "Footer ueberlaeuft die Sidebar: %s" % line
    T(SX0 + (SIDEBAR_W - TW(line)) / 2, FOOTER_TOP + k * LINE_ADV, line, col)

# content column: Executor::Render()
T(CX0, CY0 + (LINE_H - FONT_SIZE) / 2, "Executor", C_TEXT)
y = CY0 + LINE_ADV
R(CX0, y, CX1, y + 1, C_WINDOW_BG)
y += 1 + ITEM_SPACING_Y

avail_w, avail_h = CX1 - CX0, CY1 - y
ed_w, ed_h = int(avail_w - 16), int(avail_h - 52 - LINE_ADV)
ex0, ey0, ex1, ey1 = CX0, y, CX0 + ed_w, y + ed_h

R(ex0, ey0, ex1, ey1, P["bg"])
d.rectangle([ex0*S, ey0*S, ex1*S-1, ey1*S-1], outline=C_BORDER, width=S)

gutter = TW("0" * len(str(len(SAMPLE)))) + 10      # mTextStart = digits + mLeftMargin
tx = ex0 + gutter + 4
CURSOR_LINE = len(SAMPLE) - 1

# the widget scrolls the cursor into view when the box is shorter than the
# document, so only the last cap lines are visible
_cols = colorize(SAMPLE)
_cap = max(1, int((ed_h - 8) // LINE_ADV))
_first = max(0, len(SAMPLE) - _cap)
for ln in range(_first, len(SAMPLE)):
    runs = _cols[ln]
    ly = ey0 + 4 + (ln - _first) * LINE_ADV
    if ln == CURSOR_LINE:
        R(ex0+1, ly, ex1-1, ly+LINE_ADV, blend(P["cline_inact"], P["bg"]))
        R(ex0+1, ly, ex1-1, ly+1, blend(P["cline_edge"], P["bg"]))
        R(ex0+1, ly+LINE_ADV-1, ex1-1, ly+LINE_ADV, blend(P["cline_edge"], P["bg"]))
    num = str(ln + 1)
    T(ex0 + gutter - TW(num), ly + (LINE_ADV - FONT_SIZE) / 2, num, P["lineno"])
    cx, top = tx, ly + (LINE_ADV - FONT_SIZE) / 2
    for txt, key in runs:
        T(cx, top, txt, P[key]); cx += TW(txt)
    if ln == CURSOR_LINE:
        R(cx + 1, ly + 6, cx + 2, ly + LINE_ADV - 6, P["cursor"])

# status line (exec.cpp): cursor + document size, muted accent
T(CX0, ey1 + 2, "Ln 11, Col 42  |  11 lines  |  Lua", blend((93, 156, 214, 153), C_WINDOW_BG))

# button row: Execute 116x30, Load from File 180x30, combo fills the rest
y = ey1 + LINE_ADV + ITEM_SPACING_Y
bh = 30
# Execute is a Gui::PrimaryButton: brand fill, soft top highlight, centred label
d.rounded_rectangle([CX0*S, y*S, (CX0+116)*S-1, (y+bh)*S-1], radius=6*S, fill=C_PRIMARY)
d.rounded_rectangle([(CX0+1)*S, (y+1)*S, (CX0+115)*S-1, (y+bh/2)*S-1], radius=6*S,
                    fill=blend((255, 255, 255, 26), C_PRIMARY))
_exe_w = TW("\uf121", f_icon, 1.0) + TW(" Execute")
_exe_x = CX0 + (116 - _exe_w) / 2
T(_exe_x, y + (bh-FONT_SIZE)/2 + (FONT_SIZE-ICON_SIZE)/2, "\uf121", C_TEXT, f_icon, 1.0)
T(_exe_x + TW("\uf121", f_icon, 1.0), y + (bh-FONT_SIZE)/2, " Execute", C_TEXT)

x2 = CX0 + 116 + ITEM_SPACING_X
R(x2, y, x2 + 180, y + bh, C_BUTTON)
T(x2 + FRAME_PAD_X, y + (bh-FONT_SIZE)/2 + (FONT_SIZE-ICON_SIZE)/2, "\uf15b", C_TEXT, f_icon, 1.0)
T(x2 + FRAME_PAD_X + TW("\uf15b", f_icon, 1.0), y + (bh-FONT_SIZE)/2, " Load from File", C_TEXT)

x3 = x2 + 180 + ITEM_SPACING_X
fh = FONT_SIZE + FRAME_PAD_Y * 2
R(x3, y, CX1, y + fh, C_FRAME_BG)
d.rectangle([x3*S, y*S, CX1*S-1, (y+fh)*S-1], outline=C_BORDER, width=S)
T(x3 + FRAME_PAD_X, y + (fh-FONT_SIZE)/2, "_cfx_internal", C_TEXT)
aw, ax, ay = 9, CX1 - FRAME_PAD_X - 9, y + fh / 2
d.polygon([(ax*S, (ay-3)*S), ((ax+aw)*S, (ay-3)*S), ((ax+aw/2)*S, (ay+3)*S)], fill=C_TEXT)

# toast stack (toasts.cpp): one Success card, 10px off the bottom-right corner
_toast = "Dump saved to Dumps"
_tw = min(320.0, TW(_toast) + 3 + 8 * 3)
_th = FONT_SIZE + 8 * 2
_tx1, _ty1 = W - 10, H - 10
_tx0, _ty0 = _tx1 - _tw, _ty1 - _th
d.rounded_rectangle([_tx0*S, _ty0*S, _tx1*S-1, _ty1*S-1], radius=6*S, fill=(31, 58, 97, 242))
d.rounded_rectangle([_tx0*S, _ty0*S, _tx1*S-1, _ty1*S-1], radius=6*S,
                    outline=(93, 156, 214, 110), width=S)
d.rounded_rectangle([_tx0*S, _ty0*S, (_tx0+3)*S-1, _ty1*S-1], radius=6*S, fill=C_ACCENT,
                    corners=(True, False, False, True))
T(_tx0 + 3 + 8, _ty0 + 8, _toast, C_TEXT)

# window chrome (menu_style.h + menu.cpp): rounded corners on a desktop
# background, three-rect drop shadow behind, exactly as the DLL draws it
M, RAD = 28, 10
_mask = Image.new("L", (W * S, H * S), 0)
ImageDraw.Draw(_mask).rounded_rectangle([0, 0, W * S - 1, H * S - 1], radius=RAD * S, fill=255)
canvas = Image.new("RGBA", ((W + 2 * M) * S, (H + 2 * M) * S), (12, 18, 28, 255))
_shadow = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
ImageDraw.Draw(_shadow).rounded_rectangle([0, 0, W * S - 1, H * S - 1], radius=RAD * S,
                                          fill=(0, 0, 0, 150))
_shadow = _shadow.filter(ImageFilter.GaussianBlur(9 * S))
canvas.alpha_composite(_shadow, (M * S, (M + 12) * S))
canvas.paste(img, (M * S, M * S), _mask)

canvas.save(OUT, optimize=True)
os.unlink(fa_path)
print("geschrieben:", OUT, os.path.getsize(OUT), "Bytes", canvas.size)
