#!/usr/bin/env python3
"""Regenerates the EBengin preview images: the menu shell with the Executor
tab open (docs/preview.png), the Config tab (docs/preview_config.png), the
Resources tab (docs/preview_resources.png) or the Menus tab
(docs/preview_menus.png) - plus the first-open start card on its own
(docs/preview_splash.png, --splash) and the FPS HUD plate over a scene
stand-in (docs/preview_hud.png, --hud), and the session key card on its
own (docs/preview_key.png, --key).

Self-contained on purpose: it needs only Pillow and the repository itself. The
sidebar/button glyphs come from the project's *own* embedded Font Awesome font
(font_awesome.cpp stores it as an stb-compressed blob, decompressed here with a
port of imgui_draw.cpp's stb_decompress), so the icons are the real ones.

Every colour, size and label is read from the repository's own constants, not
invented:

  menu_style.h   the two theme ramps and every ImGuiCol_* below
  menu.h         SidebarWidth (225), SidebarButtonHeight (46), ContentPadding (8), Accent
  menu.cpp       Begin(title###id, &IsMenuOpen, NoScrollbar|NoCollapse|NoResize), SidebarItems[]
                 labels, logo 76x87 centred at CursorPosX(74.5), bottom-anchored footer
  exec.cpp       editor size (avail.x-16, avail.y-52), Execute 116x30,
                 Load from File 180x30, toolbar save and revert icons,
                 resource combo,
                 status line (Ln/Col/lines) between editor and buttons
  gui.h          PrimaryButton fill (rest #3778B5, hover #5D9CD6, press #1D5188)
  toasts.cpp     card metrics: 320 max width, 3px kind bar, 8px inner padding
  menu.cpp       drop shadow (three stacked rects), brand rule under the title
                 bar, toast stack anchored 10px off the window's bottom-right
  TextEditor.cpp GetDarkPalette(), mLeftMargin (10), mLineSpacing (1.0)
  config.cpp       pane split (avail.x / 3), the two inert switch sections and
                   the Interface prefs, the About block lines and both of its
                   buttons - used by the --config branch only
  default.cpp      landing layout: graph height Px(60), the Copy 50x20 /
                   Clear 60x20 pair sharing a line with the heading, log
                   newest first - used by the --default branch only
  menu.cpp         key card (row 155): 460x232 card, password field 280 wide,
                   Unlock 140x30, muted gate hint - used by the --key branch

Two things CANNOT be faithful and are substituted, exactly as the README caption
says: Bahnschrift.ttf is a Windows-only font (a horizontally condensed DejaVu
Sans stands in for its DIN advance widths), and ImGui's layout engine is not
running, so positions follow the constants above. This is a render, not a
screenshot.

Usage:  python docs/render_preview.py             -> docs/preview.png          (Executor tab)
        python docs/render_preview.py --config    -> docs/preview_config.png   (Config tab)
        python docs/render_preview.py --resources -> docs/preview_resources.png
        python docs/render_preview.py --menus     -> docs/preview_menus.png
        python docs/render_preview.py --default   -> docs/preview_default.png  (landing tab)
        python docs/render_preview.py --splash    -> docs/preview_splash.png
        python docs/render_preview.py --hud       -> docs/preview_hud.png
        python docs/render_preview.py --key       -> docs/preview_key.png    (key card)
        (requires Pillow)
"""
import math
import os
import re
import struct
import sys
import tempfile
import zlib

from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Which tab the shell is rendered with.  "executor" is the product shot;
# "config" shows the Interface preferences and the two inert switches, i.e. the
# pane whose labels are narrow enough to be clipped without anyone noticing.
if "--config" in sys.argv[1:]:
    TAB = "config"
elif "--resources" in sys.argv[1:]:
    TAB = "resources"
elif "--menus" in sys.argv[1:]:
    TAB = "menus"
elif "--splash" in sys.argv[1:]:
    TAB = "splash"
elif "--hud" in sys.argv[1:]:
    TAB = "hud"
elif "--default" in sys.argv[1:]:
    TAB = "default"
elif "--key" in sys.argv[1:]:
    TAB = "key"
else:
    TAB = "executor"
OUT = os.path.join(REPO, "docs", {"executor": "preview.png",
                                  "config": "preview_config.png",
                                  "resources": "preview_resources.png",
                                  "menus": "preview_menus.png",
                                  "splash": "preview_splash.png",
                                  "hud": "preview_hud.png",
                                  "default": "preview_default.png",
                                  "key": "preview_key.png"}[TAB])

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
C_MUTED_BRAND = RGBA(125, 175, 225)  # ColMuted token, row 147 (was accent@153)
C_RAIL     = RGBA(16, 31, 52)      # sidebar CellBg: WindowBg darkened 18%
C_SCROLL   = RGBA(42, 74, 123)     # ImGuiCol_ScrollbarGrab (raised navy)

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
    '-- EBengin | Lua executor',
    'local players = GetActivePlayers()',
    '',
    'for _, id in ipairs(players) do',
    '    local ped = GetPlayerPed(id)',
    '    if DoesEntityExist(ped) then',
    '        print("player " .. tostring(id))',
    '    end',
    'end',
    '',
    'TriggerServerEvent("EB:sync", 0x1F, 3.14)',
]

# ---------------------------------------------------------------------------
# canvas
# ---------------------------------------------------------------------------
fa_path = extract_icon_font()
f_ui = ImageFont.truetype(TEXT_FONT, int(FONT_SIZE * S))
f_icon = ImageFont.truetype(fa_path, int(ICON_SIZE * S))

img = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
SURF = img          # alpha-composite target of T(); the splash branch swaps it


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
    SURF.alpha_composite(tmp, (int(round(x*S)), int(round(y*S))))


def TW(s, font=f_ui, condense=CONDENSE):
    return font.getlength(s) * condense / S


def wrap_text(s, w):
    """ImGui's word wrap (TextWrapped / CalcTextSize with wrap_width)."""
    lines, cur = [], ""
    for wd in s.split():
        cand = (cur + " " + wd).strip()
        if cur == "" or TW(cand) <= w:
            cur = cand
        else:
            lines.append(cur)
            cur = wd
    lines.append(cur)
    return lines


if TAB == "hud":
    # ------------------------------------------------------------------
    # main.cpp: while the overlay is closed and Settings::Current.Hud is
    # on, Menu::RenderHud() draws the sidebar's FPS line plus the Remade : by EB
    # credit line on the background
    # draw list - over the game scene, under any window.  The scene is
    # arbitrary in game, so this render uses a mid-tone stand-in that lets
    # the 63%-alpha plate read the way it does over a bright frame.
    # ------------------------------------------------------------------
    SCENE = RGBA(34, 44, 58)
    FW, FH = W + 2 * 28, H + 2 * 28
    canvas = Image.new("RGBA", (FW * S, FH * S), SCENE)
    d = ImageDraw.Draw(canvas)
    SURF = canvas

    HUD = "FPS 144  |  6.9 ms"
    CREDIT = "Remade : by EB"      # menu.cpp: second plate line, muted brand
    assert TW(HUD) <= FW - 40, "hud line clipped"
    assert TW(CREDIT) <= FW - 40, "hud credit clipped"
    ts_w = max(TW(HUD), TW(CREDIT))
    adv = FONT_SIZE + ITEM_SPACING_Y      # GetTextLineHeightWithSpacing()
    plate_h = adv + FONT_SIZE             # two lines
    pad, pos = 6.0, (10.0, 10.0)
    plate = Image.new("RGBA", (FW * S, FH * S), (0, 0, 0, 0))
    ImageDraw.Draw(plate).rounded_rectangle(
        [(pos[0] - pad) * S, (pos[1] - pad) * S,
         (pos[0] + ts_w + pad) * S - 1, (pos[1] + plate_h + pad) * S - 1],
        radius=6 * S, fill=(12, 18, 28, 220))   # row 147: AA backing alpha
    canvas.alpha_composite(plate)
    d = ImageDraw.Draw(canvas)
    T(pos[0], pos[1], HUD, C_TEXT)             # row 147: FPS line white, not accent
    # credit ink: opaque AA muted token (row 147)
    T(pos[0], pos[1] + adv, CREDIT, (125, 175, 225))

    canvas.save(OUT, optimize=True)
    os.unlink(fa_path)
    print("geschrieben:", OUT, "(hud)", os.path.getsize(OUT), "Bytes", canvas.size)
    sys.exit(0)

if TAB == "splash":
    # ------------------------------------------------------------------
    # menu.cpp RenderSplash(): on the first open after inject the branded
    # start card replaces the shell.  Standalone canvas: desktop background
    # plus one centred, rounded card - no sidebar, no title bar.
    # ------------------------------------------------------------------
    MC, RADc = 28, 10
    HINT1 = "Interface-level: no game state is read or written."
    HINT2 = "INSERT toggles this overlay at any time."  # code formats
    # KeyName(bound key); the preview depicts the default binding, INSERT.
    # A printable binding (hand-edited ini only, row 210) gets the qualified
    # variant "(not while typing in a text field)" instead.
    def wrap(t, maxw):
        lines, cur = [], ""
        for w in t.split(" "):
            test = (cur + " " + w).strip()
            if cur and TW(test) > maxw:
                lines.append(cur); cur = w
            else:
                cur = test
        lines.append(cur)
        return lines
    HINT1L, HINT2L = wrap(HINT1, 460.0 - 16), wrap(HINT2, 460.0 - 16)
    HINT2P = wrap("0 toggles this overlay (not while typing in a text field).",
                  460.0 - 16)
    ADV = FONT_SIZE + ITEM_SPACING_Y
    CWc = 460.0
    # UI: row 228 - the card grows by one line advance per wrapped hint line
    CHc = 288.0 + (len(HINT1L) + len(HINT2L) - 2) * ADV
    CHw = 288.0 + (len(HINT1L) + len(HINT2P) - 2) * ADV   # worst-case height
    FW, FH = W + 2 * MC, H + 2 * MC
    canvas = Image.new("RGBA", (FW * S, FH * S), (12, 18, 28, 255))
    OX, OY = (FW - CWc) / 2, (FH - CHc) / 2

    # three-rect drop shadow, same recipe the shell chrome uses
    sh = Image.new("RGBA", (FW * S, FH * S), (0, 0, 0, 0))
    ImageDraw.Draw(sh).rounded_rectangle(
        [OX * S, OY * S, (OX + CWc) * S - 1, (OY + CHc) * S - 1],
        radius=RADc * S, fill=(0, 0, 0, 150))
    sh = sh.filter(ImageFilter.GaussianBlur(9 * S))
    canvas.alpha_composite(sh, (0, int(12 * S)))

    # R()/T() below draw onto the canvas, not onto the (unused) shell surface
    d = ImageDraw.Draw(canvas)
    SURF = canvas
    d.rounded_rectangle([OX * S, OY * S, (OX + CWc) * S - 1, (OY + CHc) * S - 1],
                        radius=RADc * S, fill=C_WINDOW_BG)
    d.rounded_rectangle([OX * S, OY * S, (OX + CWc) * S - 1, (OY + CHc) * S - 1],
                        radius=RADc * S, outline=C_BORDER, width=S)

    CREDIT = "Remade : by EB"
    BTN = "Continue"
    for t in (CREDIT, BTN):
        assert TW(t) <= CWc - 16, "splash line clipped: " + t
    # UI: row 228 - the code wraps the hints at word boundaries, centres each
    # line and grows the card per extra line; prove the worst case (qualified
    # printable sentence) fits both the card width and the grown height.
    for ln in HINT1L + HINT2L + HINT2P:
        assert TW(ln) <= CWc - 16, "splash wrapped line clipped: " + ln
    assert len(HINT2P) <= 2, "printable hint needs more than two lines"

    ly = OY + 18
    lg = Image.open(os.path.join(REPO, "EBengin.png")).convert("RGBA")
    lg = lg.resize((int(110 * S), int(127 * S)), Image.LANCZOS)
    canvas.alpha_composite(lg, (int((OX + (CWc - 110) / 2) * S), int(ly * S)))
    ly += 127 + ITEM_SPACING_Y
    T(OX + (CWc - TW(CREDIT)) / 2, ly, CREDIT, C_MUTED_BRAND)
    ly += FONT_SIZE + ITEM_SPACING_Y
    ly += ITEM_SPACING_Y                              # ImGui::Spacing()
    R(OX + 8, ly, OX + CWc - 8, ly + 1, C_BORDER)     # Gui::Seperator
    ly += 1 + ITEM_SPACING_Y
    ly += ITEM_SPACING_Y                              # ImGui::Spacing()
    for ln in HINT1L:
        T(OX + (CWc - TW(ln)) / 2, ly, ln, C_TEXT)
        ly += FONT_SIZE + ITEM_SPACING_Y
    for ln in HINT2L:
        T(OX + (CWc - TW(ln)) / 2, ly, ln, C_TEXT)
        ly += FONT_SIZE + ITEM_SPACING_Y
    # worst-case height budget: with the qualified variant wrapped to two
    # lines the card grows to CHw, and the content must still end inside it
    assert ly + (len(HINT2P) - len(HINT2L)) * ADV + ITEM_SPACING_Y + 30 \
        <= OY + CHw - 8, "splash card too short for a wrapped hint"
    ly += ITEM_SPACING_Y                              # ImGui::Spacing()
    bx, BW, BH = OX + (CWc - 140) / 2, 140.0, 30.0
    d.rounded_rectangle([bx * S, ly * S, (bx + BW) * S - 1, (ly + BH) * S - 1],
                        radius=6 * S, fill=C_PRIMARY)
    d.rounded_rectangle([(bx + 1) * S, (ly + 1) * S, (bx + BW - 1) * S - 1,
                         (ly + BH / 2) * S - 1], radius=6 * S,
                        fill=blend((255, 255, 255, 26), C_PRIMARY))
    T(bx + (BW - TW(BTN)) / 2, ly + (BH - FONT_SIZE) / 2, BTN, C_TEXT)
    ly += BH
    assert ly <= OY + CHc - 8, "splash card overflows: %.1f" % ly

    canvas.save(OUT, optimize=True)
    os.unlink(fa_path)
    print("geschrieben:", OUT, "(splash)", os.path.getsize(OUT), "Bytes", canvas.size)
    sys.exit(0)

# ------------------------------------------------------------------
# menu.cpp RenderKey() (row 155): the session key card replaces splash
# and shell until the key matches.  Standalone canvas, same recipe as
# the splash card; the input is depicted empty (password field).
# ------------------------------------------------------------------
if TAB == "key":
    MC, RADk = 28, 10
    CWk, CHk = 460.0, 232.0
    FW, FH = W + 2 * MC, H + 2 * MC
    canvas = Image.new("RGBA", (FW * S, FH * S), (12, 18, 28, 255))
    OX, OY = (FW - CWk) / 2, (FH - CHk) / 2

    sh = Image.new("RGBA", (FW * S, FH * S), (0, 0, 0, 0))
    ImageDraw.Draw(sh).rounded_rectangle(
        [OX * S, OY * S, (OX + CWk) * S - 1, (OY + CHk) * S - 1],
        radius=RADk * S, fill=(0, 0, 0, 150))
    sh = sh.filter(ImageFilter.GaussianBlur(9 * S))
    canvas.alpha_composite(sh, (0, int(12 * S)))

    d = ImageDraw.Draw(canvas)
    SURF = canvas
    d.rounded_rectangle([OX * S, OY * S, (OX + CWk) * S - 1, (OY + CHk) * S - 1],
                        radius=RADk * S, fill=C_WINDOW_BG)
    d.rounded_rectangle([OX * S, OY * S, (OX + CWk) * S - 1, (OY + CHk) * S - 1],
                        radius=RADk * S, outline=C_BORDER, width=S)

    def C(text, col, cy):
        T(OX + (CWk - TW(text)) / 2, cy, text, col)
        return cy + LINE_ADV

    y = OY + 16
    y = C("EBengin", C_ACCENT, y)
    y = C("This overlay is key-protected.", C_TEXT, y)
    y += ITEM_SPACING_Y
    R(OX + 8, y, OX + CWk - 8, y + 1, C_BORDER)      # Gui::Seperator
    y += 1 + ITEM_SPACING_Y

    iw, fh = 280.0, FONT_SIZE + FRAME_PAD_Y * 2
    ix = OX + (CWk - iw) / 2
    R(ix, y, ix + iw, y + fh, C_FRAME_BG)
    d.rectangle([ix * S, y * S, (ix + iw) * S - 1, (y + fh) * S - 1],
                outline=C_BORDER, width=S)
    y += fh + ITEM_SPACING_Y

    bw = 140.0
    bx = OX + (CWk - bw) / 2
    d.rounded_rectangle([bx * S, y * S, (bx + bw) * S - 1, (y + 30) * S - 1],
                        radius=6 * S, fill=C_PRIMARY)
    d.rounded_rectangle([(bx + 1) * S, (y + 1) * S, (bx + bw - 1) * S - 1, (y + 15) * S - 1],
                        radius=6 * S, fill=blend((255, 255, 255, 26), C_PRIMARY))
    T(bx + (bw - TW("Unlock")) / 2, y + (30 - FONT_SIZE) / 2, "Unlock", C_TEXT)
    y += 30 + ITEM_SPACING_Y

    C("Local gate - no server is contacted.", C_MUTED_BRAND, y)

    canvas.save(OUT, optimize=True)
    os.unlink(fa_path)
    print("geschrieben:", OUT, "(key)", os.path.getsize(OUT), "Bytes", canvas.size)
    sys.exit(0)

# window + title bar (WindowTitleAlign 0,0.5; NoCollapse, p_open = &IsMenuOpen:
# row 144 added the close button, row 145 the tab name in the title)
R(0, 0, W, H, C_WINDOW_BG)
R(SX0, CY0, SX1 + 1, CY1, C_RAIL)          # nav rail: sidebar cell, TableSetBgColor
R(0, 0, W, TITLE_H, C_TITLE_BG)
TITLE = {"default": "EBengin", "executor": "EBengin - Executor",
         "resources": "EBengin - Resources", "menus": "EBengin - Menus",
         "config": "EBengin - Config"}[TAB]
T(WIN_PAD, (TITLE_H - FONT_SIZE) / 2, TITLE, C_TEXT)
# close button cross at the right end of the title bar (ImGui p_open glyph;
# drawn as two strokes - the renderer font has no multiplication sign)
_cx1, _cy1 = W - WIN_PAD - 9, (TITLE_H - 8) / 2
_cx2, _cy2 = W - WIN_PAD - 2, (TITLE_H + 8) / 2
d.line([(_cx1 * S, _cy1 * S), (_cx2 * S, _cy2 * S)], fill=C_TEXT, width=S)
d.line([(_cx1 * S, _cy2 * S), (_cx2 * S, _cy1 * S)], fill=C_TEXT, width=S)

# brand rule under the title bar (menu.cpp): accent -> pressed blue, left to right
for _x in range(W):
    _t = _x / (W - 1.0)
    R(_x, TITLE_H, _x + 1, TITLE_H + 2,
      tuple(int(round(C_ACCENT[i] + (C_BUTTON_ACT[i] - C_ACCENT[i]) * _t)) for i in range(3)) + (255,))

# 2-column table: BordersInnerV, SizingFixedFit, NoPadOuterX, CellPadding (0,0)
R(SX1, CY0, SX1 + 1, CY1, C_BORDER)

# sidebar: logo centred ((225-76)/2 = 74.5 since row 161) at CursorPosY + 9
logo = Image.open(os.path.join(REPO, "EBengin.png")).convert("RGBA")
logo = logo.resize((int(76*S), int(87*S)), Image.LANCZOS)
img.alpha_composite(logo, (int((SX0+74.5)*S), int((CY0+9)*S)))

SIDEBAR = [("\uf121", " Executor"), ("\uf019", " Resources"),
           ("\uf02d", " Menus"),    ("\uf085", " Config")]
ACTIVE = {"executor": 0, "resources": 1, "menus": 2, "config": 3,
          "default": -1}[TAB]   # the landing tab has no sidebar row

sep_y = 170
R(SX0, sep_y, SX1, sep_y + 1, C_BORDER)
by = sep_y + 1
for i, (glyph, label) in enumerate(SIDEBAR):
    R(SX0, by, SX1, by + SIDEBAR_BTN, C_BUTTON_ACT if i == ACTIVE else C_BUTTON)
    if i == ACTIVE:
        # accent wash left -> right over the pressed colour (menu.cpp, alpha 22->0)
        for _x in range(int(SIDEBAR_W)):
            a = 22 * (1 - _x / (SIDEBAR_W - 1)) / 255.0
            R(SX0 + _x, by, SX0 + _x + 1, by + SIDEBAR_BTN,
              tuple(int(round(C_BUTTON_ACT[k] + (C_ACCENT[k] - C_BUTTON_ACT[k]) * a))
                    for k in range(3)) + (255,))
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
FOOTER_TOP = H - WIN_PAD - 4 * LINE_ADV
FOOTER = [("FPS 144  |  6.9 ms", C_ACCENT),
          ("No license check.", C_TEXT),
          ("Build Date: Oct 22 2026", C_TEXT),
          ("Remade : by EB", C_MUTED_BRAND)]
for k, (line, col) in enumerate(FOOTER):
    assert TW(line) <= SIDEBAR_W, "Footer ueberlaeuft die Sidebar: %s" % line
    T(SX0 + (SIDEBAR_W - TW(line)) / 2, FOOTER_TOP + k * LINE_ADV, line, col)

if TAB == "executor":
    # content column: Executor::Render()
    T(CX0, CY0 + (LINE_H - FONT_SIZE) / 2, "Executor", C_TEXT)
    T(CX0 + 0.7, CY0 + (LINE_H - FONT_SIZE) / 2, "Executor", C_TEXT)  # row 151 faux bold
    y = CY0 + LINE_ADV
    R(CX0, y, CX1, y + 1, C_BORDER)
    y += 1 + ITEM_SPACING_Y

    # editor toolbar (exec.cpp): icon buttons for undo/redo/copy/paste/clear/save
    TB = 26
    # undo/redo/copy sit inside BeginDisabled on a fresh buffer (nothing to
    # undo, redo or copy): BeginDisabled multiplies style.Alpha by
    # DisabledAlpha (0.60, stock), so fill and glyph both blend 60% over what
    # is behind them.  paste/clear/save stay live; revert (last icon) is grey
    # without a file behind the buffer, which is what the preview depicts.
    for _k, _g in enumerate(["\uf0e2", "\uf01e", "\uf0c5", "\uf0ea", "\uf12d",
                             "\uf0c7", "\uf1da"]):
        _bx = CX0 + _k * (TB + ITEM_SPACING_X)
        if _k < 3 or _k == 6:
            _fill = blend(C_BUTTON + (153,), C_WINDOW_BG)
            _ink = blend((255, 255, 255, 153), _fill)
        else:
            _fill, _ink = C_BUTTON, C_TEXT
        R(_bx, y, _bx + TB, y + TB, _fill)
        T(_bx + (TB - TW(_g, f_icon, 1.0)) / 2, y + (TB - ICON_SIZE) / 2 + 1, _g, _ink, f_icon, 1.0)

    # whitespace toggle at the end of the toolbar (Gui::Toggle, off, persisted)
    _tkx = CX0 + 7 * (TB + ITEM_SPACING_X) + 6
    _tky = y + (TB - 20) / 2
    d.rounded_rectangle([_tkx*S, _tky*S, (_tkx+40)*S-1, (_tky+20)*S-1], radius=10*S, fill=C_FRAME_BG)
    d.ellipse([(_tkx+3)*S, (_tky+3)*S, (_tkx+17)*S-1, (_tky+17)*S-1], fill=C_TEXT)
    T(_tkx + 48, y + (TB - FONT_SIZE) / 2, "Whitespace", C_TEXT)

    y += TB + ITEM_SPACING_Y

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
    # code appends "| file: <name>" once a load or save happened and
    # "| modified" while the buffer holds unsaved edits; the preview
    # depicts a buffer with no file behind it and nothing unsaved
    T(CX0, ey1 + 2, "Ln 11, Col 42  |  11 lines  |  Lua", (125, 175, 225, 255))

    # button row: Execute 116x30, Load 180x30, Recent 90x30, combo fills rest
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
    R(x3, y, x3 + 90, y + bh, C_BUTTON)          # Recent button (row 146)
    T(x3 + FRAME_PAD_X, y + (bh-FONT_SIZE)/2 + (FONT_SIZE-ICON_SIZE)/2, "\uf0ca", C_TEXT, f_icon, 1.0)
    T(x3 + FRAME_PAD_X + TW("\uf0ca", f_icon, 1.0), y + (bh-FONT_SIZE)/2, " Recent", C_TEXT)
    x4 = x3 + 90 + ITEM_SPACING_X
    fh = FONT_SIZE + FRAME_PAD_Y * 2
    R(x4, y, CX1, y + fh, C_FRAME_BG)
    d.rectangle([x4*S, y*S, CX1*S-1, (y+fh)*S-1], outline=C_BORDER, width=S)
    T(x4 + FRAME_PAD_X, y + (fh-FONT_SIZE)/2, "_cfx_internal", C_TEXT)
    aw, ax, ay = 9, CX1 - FRAME_PAD_X - 9, y + fh / 2
    d.polygon([(ax*S, (ay-3)*S), ((ax+aw)*S, (ay-3)*S), ((ax+aw/2)*S, (ay+3)*S)], fill=C_TEXT)


elif TAB == "config":
    # ------------------------------------------------------------------
    # content column: Config::Render()
    #   left pane  = avail.x / 3   the two inert switches + Interface prefs
    #   splitter   = Gui::Seperator("##splitter", 1, avail.y)
    #   right pane = the rest      "Server related stuff"
    # Text rows here use ImGui's real GetTextLineHeight() == g.FontSize
    # (imgui.cpp:11835), so the vertical budget below is the one config.cpp
    # actually has.  The Executor branch keeps its published calibration.
    # ------------------------------------------------------------------
    TEXT_LH = FONT_SIZE
    C_DIS = RGBA(64, 109, 154)                      # ImGuiCol_TextDisabled (muted accent)
    C_MUTED = RGBA(125, 175, 225)  # ColMuted token, row 147
    HINT = "Tab/arrows move, Enter/Space activates. Ctrl+Tab switches tabs."

    T(CX0, CY0 + (LINE_H - FONT_SIZE) / 2, "Config", C_TEXT)
    T(CX0 + 0.7, CY0 + (LINE_H - FONT_SIZE) / 2, "Config", C_TEXT)  # row 151 faux bold
    y = CY0 + LINE_ADV
    R(CX0, y, CX1, y + 1, C_BORDER)                 # Gui::Seperator under the title
    y += 1 + ITEM_SPACING_Y

    pane_w = (CX1 - CX0) / 3.0                      # config.cpp: avail.x / 3.0f
    lx0, lx1 = CX0, CX0 + pane_w
    spl = lx1 + ITEM_SPACING_X
    rx0, rx1 = spl + 1 + ITEM_SPACING_X, CX1
    top, bot = y, CY1
    R(spl, top, spl + 1, bot, C_BORDER)             # the vertical splitter

    def sep(x0, x1, sy):
        R(x0, sy, x1, sy + 1, C_BORDER)
        return sy + 1 + ITEM_SPACING_Y

    def toggle(x, ty, label, on):
        """Gui::Toggle: 40x20 track, knob radius 7, row height = GetFrameHeight()."""
        trk_w, trk_h, t = 40.0, 20.0, (1.0 if on else 0.0)
        col = tuple(int(round(C_FRAME_BG[i] + (C_PRIMARY[i] - C_FRAME_BG[i]) * t))
                    for i in range(3)) + (255,)
        ty0 = ty + (LINE_H - trk_h) / 2
        d.rounded_rectangle([x*S, ty0*S, (x+trk_w)*S-1, (ty0+trk_h)*S-1],
                            radius=(trk_h/2)*S, fill=col)
        kr = trk_h / 2 - 3
        kx = x + trk_h / 2 + (trk_w - trk_h) * t
        d.ellipse([(kx-kr)*S, (ty0+trk_h/2-kr)*S, (kx+kr)*S-1, (ty0+trk_h/2+kr)*S-1],
                  fill=C_TEXT)
        T(x + trk_w + ITEM_SPACING_X, ty + (LINE_H - FONT_SIZE) / 2, label, C_TEXT)
        return ty + LINE_H + ITEM_SPACING_Y

    def button(x, by, w, h, label, primary=False):
        if primary:
            d.rounded_rectangle([x*S, by*S, (x+w)*S-1, (by+h)*S-1], radius=6*S, fill=C_PRIMARY)
            d.rounded_rectangle([(x+1)*S, (by+1)*S, (x+w-1)*S-1, (by+h/2)*S-1], radius=6*S,
                                fill=blend((255, 255, 255, 26), C_PRIMARY))
            T(x + (w - TW(label)) / 2, by + (h - FONT_SIZE) / 2, label, C_TEXT)
        else:
            d.rounded_rectangle([x*S, by*S, (x+w)*S-1, (by+h)*S-1], radius=6*S, fill=C_BUTTON)
            T(x + FRAME_PAD_X, by + (h - FONT_SIZE) / 2, label, C_TEXT)
        return by + h + ITEM_SPACING_Y

    def slider(x, sy, w, label, t):
        """SliderInt: frame + border + GrabMinSize (12) grab + centred format text."""
        h, grab = LINE_H, 12.0
        d.rounded_rectangle([x*S, sy*S, (x+w)*S-1, (sy+h)*S-1], radius=6*S, fill=C_FRAME_BG)
        d.rectangle([x*S, sy*S, (x+w)*S-1, (sy+h)*S-1], outline=C_BORDER, width=S)
        gx = x + grab / 2 + t * (w - grab)
        d.rounded_rectangle([(gx-grab/2)*S, sy*S, (gx+grab/2)*S-1, (sy+h)*S-1],
                            radius=6*S, fill=C_ACCENT)   # GrabRounding 6: pill grab
        T(x + (w - TW(label)) / 2, sy + (h - FONT_SIZE) / 2, label, C_TEXT)
        return sy + h + ITEM_SPACING_Y

    def heading(x, hy, s, col, q=False):
        T(x, hy, s, col)
        if q:                                         # SameLine() -> ItemSpacing.x gap
            T(x + TW(s) + ITEM_SPACING_X, hy, "(?)", C_ACCENT)
        return hy + TEXT_LH + ITEM_SPACING_Y

    # --- left pane: the two inert switches, then the Interface prefs ----
    ly = top
    ly = heading(lx0, ly, "Safe Mode", C_DIS, q=True)
    ly += ITEM_SPACING_Y                              # the Spacing() after the "(?)"
    ly = sep(lx0, lx1, ly)
    ly = toggle(lx0, ly, "Enabled (no effect)", False)   # flipped off -> the warn toast
    ly = sep(lx0, lx1, ly)
    ly = heading(lx0, ly, "ScriptHook Bypass", C_DIS, q=True)
    ly += ITEM_SPACING_Y
    ly = sep(lx0, lx1, ly)
    ly = toggle(lx0, ly, "Enabled (no effect)", True)
    ly = sep(lx0, lx1, ly)
    ly = heading(lx0, ly, "Interface", C_TEXT)
    ly = sep(lx0, lx1, ly)
    ly = button(lx0, ly, pane_w, 30, "Toggle key:  INSERT")
    ly += ITEM_SPACING_Y                              # ImGui::Spacing()
    ly = toggle(lx0, ly, "Animations", True)
    ly = toggle(lx0, ly, "Toast notifications", True)
    ly = toggle(lx0, ly, "FPS HUD", True)          # Settings::Current.Hud
    # UI: row 148 - plate corner combo, greyed only with the HUD itself;
    # ImGui draws the label left of the frame on one row
    fh_c = FONT_SIZE + FRAME_PAD_Y * 2
    T(lx0, ly + (fh_c-FONT_SIZE)/2, "Corner", C_TEXT)
    cbx = lx0 + TW("Corner") + ITEM_SPACING_X
    R(cbx, ly, lx1, ly + fh_c, C_FRAME_BG)
    d.rectangle([cbx*S, ly*S, lx1*S-1, (ly+fh_c)*S-1], outline=C_BORDER, width=S)
    T(cbx + FRAME_PAD_X, ly + (fh_c-FONT_SIZE)/2, "Top left", C_TEXT)
    aw, ax, ay = 9, lx1 - FRAME_PAD_X - 9, ly + fh_c / 2
    d.polygon([(ax*S, (ay-3)*S), ((ax+aw)*S, (ay-3)*S), ((ax+aw/2)*S, (ay+3)*S)], fill=C_TEXT)
    ly += fh_c + ITEM_SPACING_Y
    ly += ITEM_SPACING_Y
    ly = slider(lx0, ly, pane_w, "Interface scale 100%", (100 - 70) / 130.0)
    ly += ITEM_SPACING_Y
    ly = button(lx0, ly, pane_w, 30, "Reset interface settings", primary=True)

    # --- right pane ----------------------------------------------------
    ry = heading(rx0, top, "Server related stuff", C_TEXT)
    ry += ITEM_SPACING_Y                              # ImGui::Spacing()
    ry = sep(rx0, rx1, ry)
    ry = heading(rx0, ry, "Connects to no server; reads no server IP.", C_MUTED)
    ry = heading(rx0, ry, "The Dumps folder is its server artifact:", C_MUTED)
    ry = button(rx0, ry, rx1 - rx0, 26, "Open dumps folder")
    ry = sep(rx0, rx1, ry)

    # --- About block (config.cpp): visible brand name + credit ------------
    ry += ITEM_SPACING_Y                              # ImGui::Spacing()
    ry = heading(rx0, ry, "About", C_TEXT)
    ry += ITEM_SPACING_Y                              # ImGui::Spacing()
    ry = sep(rx0, rx1, ry)
    T(rx0, ry, "EBengin", C_ACCENT)
    ry += TEXT_LH + ITEM_SPACING_Y
    T(rx0, ry, "Remade : by EB", C_MUTED)
    ry += TEXT_LH + ITEM_SPACING_Y
    # code prints ImGui::GetVersion(); the preview pins the vendored tag
    T(rx0, ry, "Interface: ImGui 1.93.0 (ocornut)", C_MUTED)
    ry += TEXT_LH + ITEM_SPACING_Y
    # code prints this via #ifdef _DEBUG / _WIN64; previews depict Release x64
    T(rx0, ry, "Build: Release x64", C_MUTED)
    ry += TEXT_LH + ITEM_SPACING_Y
    # code formats DpiScale x ScalePercent; previews depict the 96-DPI default.
    # A red "Settings not writable" line can join this block when a
    # settings.ini write failed; the writable default draws nothing.
    T(rx0, ry, "Host: DPI 1.00, interface 100%", C_MUTED)
    ry += TEXT_LH + ITEM_SPACING_Y
    ry = sep(rx0, rx1, ry)
    ry = button(rx0, ry, rx1 - rx0, 26, "Copy diagnostics")
    ry = button(rx0, ry, rx1 - rx0, 26, "Open settings folder")
    ry += ITEM_SPACING_Y          # UI: row 148 - nav hint moved to this pane
    _hl = wrap_text(HINT, rx1 - rx0)
    for k, hl in enumerate(_hl):
        T(rx0, ry + k * TEXT_LH, hl, C_MUTED)
    ry += len(_hl) * TEXT_LH + ITEM_SPACING_Y

    # --- the pane is only a third of the column and ImGui::Text does not
    # wrap, so these asserts are what keeps a label from being clipped ---
    # TextWrapped wraps at the region width, so width cannot clip; the
    # vertical budget check below (ry <= bot) is what guards the hint.
    assert all(TW(hl) <= rx1 - rx0 for hl in wrap_text(HINT, rx1 - rx0)),\
        "nav hint wrap line clipped"
    for s in ("Safe Mode", "ScriptHook Bypass", "Interface"):
        assert TW(s) + TW("(?)") <= pane_w, "heading clipped: " + s
    for lab in ("Enabled (no effect)", "Animations", "Toast notifications", "FPS HUD"):
        assert 40 + ITEM_SPACING_X + TW(lab) <= pane_w, "switch clipped: " + lab
    for lab in ("Toggle key:  INSERT", "press any key... (ESC)"):
        assert TW(lab) + 2 * FRAME_PAD_X <= pane_w, "button label clipped: " + lab
    for lab in ("Reset interface settings", "Interface scale 100%"):
        assert TW(lab) <= pane_w, "label clipped: " + lab
    assert (TW("Corner") + ITEM_SPACING_X + TW("Bottom right")
            + 2 * FRAME_PAD_X + 12 <= pane_w), "corner combo row clipped"
    for s in ("Server related stuff", "Connects to no server; reads no server IP.",
              "The Dumps folder is its server artifact:", "Open dumps folder",
              "About", "EBengin", "Remade : by EB", "Interface: ImGui 1.93.0 (ocornut)",
              "Build: Release x64", "Build: Release Win32 (x86)",
              "Build: Debug x64 (asserts on, slower)",
              "Build: Debug Win32 (asserts on, slower)",
              "Host: DPI 1.00, interface 100%", "Copy diagnostics",
              "Open settings folder"):
        assert TW(s) <= rx1 - rx0, "right pane clipped: " + s
    assert ly <= bot + 0.5, "left Config pane overflows: %.1f > %.1f" % (ly, bot)
    assert ry <= bot + 0.5, "right Config pane overflows: %.1f > %.1f" % (ry, bot)


elif TAB == "resources":
    # ------------------------------------------------------------------
    # content column: Resources::Render(), File Browser sub-tab open.
    # The Triggers sub-tab is deliberately never rendered: its list is a
    # clipboard collection of server-event strings, and preview imagery does
    # not reproduce them.  Everything drawn here is repo data: the resource
    # names from exec.cpp, the dumper line format from resources.cpp, the
    # toast literal from the copy-dump-path branch.
    # ------------------------------------------------------------------
    C_DIS = RGBA(64, 109, 154)                      # ImGuiCol_TextDisabled (muted accent)
    T(CX0, CY0 + (LINE_H - FONT_SIZE) / 2, "Resources", C_TEXT)
    T(CX0 + 0.7, CY0 + (LINE_H - FONT_SIZE) / 2, "Resources", C_TEXT)  # row 151 faux bold
    y = CY0 + LINE_ADV
    R(CX0, y, CX1, y + 1, C_BORDER)
    y += 1 + ITEM_SPACING_Y
    # UI: row 230 - the tab's provenance line, muted brand token (row 147)
    T(CX0, y + (LINE_H - FONT_SIZE) / 2,
      "Compiled-in sample list - nothing here is read from a server.",
      C_MUTED_BRAND)
    y += LINE_ADV

    bw = (CX1 - CX0 - 2 * ITEM_SPACING_X) / 3.0        # resources.cpp buttonWidth
    SUBTABS = [("\uf07c", " File Browser"), ("\uf00e", " Triggers"), ("\uf02c", " Stopper")]
    for k, (glyph, lab) in enumerate(SUBTABS):
        bx = CX0 + k * (bw + ITEM_SPACING_X)
        d.rounded_rectangle([bx*S, y*S, (bx+bw)*S-1, (y+33)*S-1], radius=6*S, fill=C_BUTTON)
        lw = TW(glyph, f_icon, 1.0) + TW(lab)
        ix = bx + (bw - lw) / 2
        T(ix, y + (33-FONT_SIZE)/2 + (FONT_SIZE-ICON_SIZE)/2, glyph, C_TEXT, f_icon, 1.0)
        T(ix + TW(glyph, f_icon, 1.0), y + (33-FONT_SIZE)/2, lab, C_TEXT)
    y += 33 + ITEM_SPACING_Y
    R(CX0, y, CX1, y + 1, C_BORDER)                     # ##resources_seperator_3
    y += 1 + ITEM_SPACING_Y

    # filter input (InputTextWithHint, stock FrameBg, hint in TextDisabled grey)
    d.rounded_rectangle([CX0*S, y*S, CX1*S-1, (y+LINE_H)*S-1], radius=6*S, fill=C_FRAME_BG)
    d.rectangle([CX0*S, y*S, CX1*S-1, (y+LINE_H)*S-1], outline=C_BORDER, width=S)
    T(CX0 + FRAME_PAD_X, y + (LINE_H-FONT_SIZE)/2, "Filter resources...", C_DIS)
    y += LINE_H + ITEM_SPACING_Y

    # list box: pushed FrameBg #14263F == C_WINDOW_BG, child border, 14px scrollbar
    SAVE_BLOCK = 76.0                                   # see resources.cpp comment
    lh = (CY1 - y) - SAVE_BLOCK
    R(CX0, y, CX1, y + lh, C_WINDOW_BG)
    inner_x, inner_r = CX0 + 8, CX1 - 1 - 14
    names = ["_cfx_internal", "fivem", "hardcap", "anti-cheese", "esx_mafia",
             "esx_gopostal", "esx_jail", "mysql-async", "es_admin2", "async"]
    OPEN = "hardcap"
    row_h = LINE_H + ITEM_SPACING_Y
    ry = y + 8
    while ry < y + lh:
        name = names.pop(0) if names else None
        if name is None:
            break
        cy = ry + LINE_H / 2
        ax = inner_x + 2
        if name == OPEN:
            d.polygon([(ax*S, (cy-3)*S), ((ax+8)*S, (cy-3)*S), ((ax+4)*S, (cy+4)*S)], fill=C_TEXT)
            T(inner_x + 14, ry + (LINE_H-FONT_SIZE)/2, name, C_TEXT)
            ry += row_h
            # the read-only dumper editor, indented like tree content
            ex0, ew, eh = inner_x + 21, inner_r - (inner_x + 21), 200.0
            R(ex0, ry, ex0 + ew, ry + eh, P["bg"])
            d.rectangle([ex0*S, ry*S, (ex0+ew)*S-1, (ry+eh)*S-1], outline=C_BORDER, width=S)
            gut = TW("1") + 10                          # mLeftMargin
            T(ex0 + gut - TW("1"), ry + 4 + (LINE_ADV-FONT_SIZE)/2, "1", P["lineno"])
            cx = ex0 + gut + 4
            for txt, key in colorize(["--successfully dumped hardcap."])[0]:
                T(cx, ry + 4 + (LINE_ADV-FONT_SIZE)/2, txt, P[key])
                cx += TW(txt)
            ry += eh + ITEM_SPACING_Y
        else:
            d.polygon([(ax*S, (cy-4)*S), (ax*S, (cy+4)*S), ((ax+6)*S, cy*S)], fill=C_TEXT)
            T(inner_x + 14, ry + (LINE_H-FONT_SIZE)/2, name, C_TEXT)
            ry += row_h
    # rows past the list bottom are clipped by the child's clip rect
    if ry > y + lh:
        R(CX0, y + lh, CX1, y + lh + row_h + 4, C_WINDOW_BG)
    d.rectangle([CX0*S, y*S, CX1*S-1, (y+lh)*S-1], outline=C_BORDER, width=S)
    # scrollbar grab (overflowing child): thumb = track * visible/content
    vis, content = lh - 16, 37 * row_h + 204
    th = max(20.0, vis * vis / content)
    gx0, gx1 = CX1 - 1 - 14 + 3, CX1 - 1 - 3
    d.rounded_rectangle([gx0*S, (y+8)*S, gx1*S-1, (y+8+th)*S-1], radius=8*S, fill=C_SCROLL)
    y += lh

    # SaveResources(): separator, 33px primary, Spacing, 26px copy button = 76px
    R(CX0, y, CX1, y + 1, C_BORDER)
    y += 1 + ITEM_SPACING_Y
    d.rounded_rectangle([CX0*S, y*S, CX1*S-1, (y+33)*S-1], radius=6*S, fill=C_PRIMARY)
    d.rounded_rectangle([(CX0+1)*S, (y+1)*S, (CX1-1)*S-1, (y+16.5)*S-1], radius=6*S,
                        fill=blend((255, 255, 255, 26), C_PRIMARY))
    T(CX0 + (CX1-CX0-TW("Save All Resources"))/2, y + (33-FONT_SIZE)/2, "Save All Resources", C_TEXT)
    y += 33 + ITEM_SPACING_Y + ITEM_SPACING_Y
    d.rounded_rectangle([CX0*S, y*S, CX1*S-1, (y+26)*S-1], radius=6*S, fill=C_BUTTON)
    cl = TW("\uf0ea", f_icon, 1.0) + TW(" Copy dump path")
    cix = CX0 + (CX1-CX0-cl)/2
    T(cix, y + (26-FONT_SIZE)/2 + (FONT_SIZE-ICON_SIZE)/2, "\uf0ea", C_TEXT, f_icon, 1.0)
    T(cix + TW("\uf0ea", f_icon, 1.0), y + (26-FONT_SIZE)/2, " Copy dump path", C_TEXT)
    y += 26 + ITEM_SPACING_Y

    for glyph, lab in SUBTABS:
        assert TW(glyph, f_icon, 1.0) + TW(lab) + 2*FRAME_PAD_X <= bw, "sub-tab clipped: " + lab
    assert TW("Save All Resources") <= CX1 - CX0, "primary clipped"
    assert max(TW(nm) for nm in ["_cfx_internal", "mysql-async"]) + 14 <= inner_r - inner_x
    assert 1 + ITEM_SPACING_Y + 33 + ITEM_SPACING_Y + ITEM_SPACING_Y + 26 + ITEM_SPACING_Y == SAVE_BLOCK
    assert y <= CY1 + 0.5, "Resources column overflows: %.1f > %.1f" % (y, CY1)


elif TAB == "menus":
    # ------------------------------------------------------------------
    # content column: Menus::Render() - two list panes around an unbordered
    # 1px splitter (invisible by design), stock-FrameBg list boxes, and the
    # honest "Run <name>" stubs.  The names are the repository's own lists.
    # ------------------------------------------------------------------
    row = FONT_SIZE + ITEM_SPACING_Y
    C_HEADER = RGBA(42, 74, 123)                    # ImGuiCol_Header since row 88
    T(CX0, CY0 + (LINE_H - FONT_SIZE) / 2, "Menus", C_TEXT)
    T(CX0 + 0.7, CY0 + (LINE_H - FONT_SIZE) / 2, "Menus", C_TEXT)  # row 151 faux bold
    y = CY0 + LINE_ADV
    R(CX0, y, CX1, y + 1, C_BORDER)
    y += 1 + ITEM_SPACING_Y
    # UI: row 232 - the tab's provenance line, muted brand token (row 147)
    T(CX0, y + (LINE_H - FONT_SIZE) / 2,
      "Compiled-in name list - nothing here is bundled or launched.",
      C_MUTED_BRAND)
    y += LINE_ADV
    y += LINE_ADV                                   # ImGui::NewLine()

    half = (CX1 - CX0) / 2.0
    PANES = [(CX0, CX0 + half - 2.0, "\uf015", " Free Menus ",
              ["HoaX_2.0", "Lynx_8.1", "Lynx_10.0", "InSec_1.0", "SkidMenu_1.0",
               "TiagoMenu_3.0", "TiagoMenu_4.0", "TiagoMenu_4.1", "Strata_1.0",
               "Lydia_Enterprise"], "Run HoaX_2.0"),
             (CX0 + half + 7.0, CX1, "\uf005", " Premium Menus ",
              ["Lux", "Dopamine", "Absolute", "Reaper", "Fallout"], "Run Lux")]
    for px0, px1, glyph, title, items, run in PANES:
        ix, ir = px0 + 8, px1 - 8                   # child WindowPadding
        lw = ir - ix
        cy = y + 8
        T(ix, cy, glyph, C_TEXT, f_icon, 1.0)
        T(ix + TW(glyph, f_icon, 1.0), cy, title, C_TEXT)
        T(ix + TW(glyph, f_icon, 1.0) + TW(title), cy, glyph, C_TEXT, f_icon, 1.0)
        cy += row
        R(ix, cy, ir, cy + 1, C_BORDER)             # Gui::Seperator inside the child
        cy += 1 + ITEM_SPACING_Y
        R(ix, cy, ix + lw, cy + 300, C_FRAME_BG)    # BeginListBox, stock FrameBg
        d.rectangle([ix*S, cy*S, (ix+lw)*S-1, (cy+300)*S-1], outline=C_BORDER, width=S)
        ry = cy + 8
        for j, nm in enumerate(items):
            if j == 0:                              # selected == ImGuiCol_Header
                R(ix + 1, ry, ix + lw - 1, ry + FONT_SIZE, C_HEADER)
            T(ix + 8, ry, nm, C_TEXT)
            ry += row
        cy += 300 + ITEM_SPACING_Y
        d.rounded_rectangle([ix*S, cy*S, (ix+lw)*S-1, (cy+33)*S-1], radius=6*S, fill=C_PRIMARY)
        d.rounded_rectangle([(ix+1)*S, (cy+1)*S, (ix+lw-1)*S-1, (cy+16.5)*S-1], radius=6*S,
                            fill=blend((255, 255, 255, 26), C_PRIMARY))
        T(ix + (lw - TW(run)) / 2, cy + (33-FONT_SIZE)/2, run, C_TEXT)
        cy += 33 + ITEM_SPACING_Y

        tw = 2 * TW(glyph, f_icon, 1.0) + TW(title)
        assert tw <= lw, "menu title clipped: " + title
        assert TW(run) <= lw, "run label clipped: " + run
        assert max(TW(nm) for nm in items) + 8 <= lw, "menu name clipped"
        assert len(items) * row + 16 <= 300, "list needs a scrollbar: " + title
        assert cy <= CY1 + 0.5, "menu pane overflows: %.1f > %.1f" % (cy, CY1)


# toast stack (toasts.cpp): one card, 10px off the window's bottom-right corner.
# Card metrics straight from the source: 320px max width, 3px kind bar, 8px inner
# padding, text wrapped at maxW - barW - inner*3, height = text height + 2*inner.
def draw_toast(text, bar):
    inner, bar_w, max_w = 8.0, 3.0, 320.0
    lines = wrap_text(text, max_w - bar_w - inner * 3)
    tw = min(max_w, max(TW(l) for l in lines) + bar_w + inner * 3)
    th = len(lines) * FONT_SIZE + inner * 2      # CalcTextSize height == lines * GetTextLineHeight()
    x1, y1 = W - 10, H - 10
    x0, y0 = x1 - tw, y1 - th
    d.rounded_rectangle([x0*S, y0*S, x1*S-1, y1*S-1], radius=6*S, fill=(31, 58, 97, 242))
    d.rounded_rectangle([x0*S, y0*S, x1*S-1, y1*S-1], radius=6*S,
                        outline=(93, 156, 214, 110), width=S)
    d.rounded_rectangle([x0*S, y0*S, (x0+bar_w)*S-1, y1*S-1], radius=6*S, fill=bar,
                        corners=(True, False, False, True))
    for k, tl in enumerate(lines):
        T(x0 + bar_w + inner, y0 + inner + k * FONT_SIZE, tl, C_TEXT)


# standalone if, not an elif: the tab chain above is already closed by the
# top-level draw_toast() def, and TAB can only hold one value anyway.
if TAB == "default":
    # Landing tab (default.cpp): title, hint line, Diagnostics block (sparkline
    # panel plus the uptime/actions/FPS line), then Session activity with the
    # Copy/Clear pair and three log entries, newest first.
    C_MUT = RGBA(125, 175, 225)  # ColMuted token, row 147
    TEXT_LH = FONT_SIZE          # ImGui::GetTextLineHeight(), as in config

    def dsep(x0, x1, sy):
        R(x0, sy, x1, sy + 1, C_BORDER)
        return sy + 1 + ITEM_SPACING_Y

    T(CX0, CY0 + (LINE_H - FONT_SIZE) / 2, "Default", C_TEXT)
    T(CX0 + 0.7, CY0 + (LINE_H - FONT_SIZE) / 2, "Default", C_TEXT)  # row 151 faux bold
    y = CY0 + LINE_ADV
    R(CX0, y, CX1, y + 1, C_BORDER)
    y += 1 + ITEM_SPACING_Y
    hint = "No section has been selected. Click a button on the left navbar."
    T(CX0, y, hint, C_TEXT)
    y += TEXT_LH + ITEM_SPACING_Y
    y = dsep(CX0, CX1, y)
    T(CX0, y, "Diagnostics", C_TEXT)
    y += TEXT_LH + ITEM_SPACING_Y
    # Perf::DrawGraph: 60 px of frame-time history on a dark panel
    gh = 60
    R(CX0, y, CX1, y + gh, (16, 31, 52))
    R(CX0, y, CX1, y + 1, C_BORDER)
    R(CX0, y + gh - 1, CX1, y + gh, C_BORDER)
    pts = []
    for k in range(60):
        px = CX0 + 2 + (CX1 - CX0 - 4) * k / 59.0
        v = 0.35 + 0.22 * math.sin(k * 0.55) + 0.10 * math.sin(k * 1.7 + 1.0)
        pts.append((px, y + gh - 4 - v * (gh - 10)))
    d.line([(px * S, py * S) for px, py in pts], fill=C_ACCENT, width=S,
           joint="curve")
    y += gh + ITEM_SPACING_Y
    diag = "Menu open 00:12:34  |  87 actions  |  144 FPS now"
    T(CX0, y, diag, C_MUT)
    y += TEXT_LH + ITEM_SPACING_Y
    y = dsep(CX0, CX1, y)
    T(CX0, y, "Session activity", C_TEXT)
    bh, bw_copy, bw_clear = 20, 50, 60
    x_clear = CX1 - bw_clear
    x_copy = x_clear - 4 - bw_copy
    by = y + (TEXT_LH - bh) / 2 - 1
    for bx, bw, bl in ((x_copy, bw_copy, "Copy"), (x_clear, bw_clear, "Clear")):
        R(bx, by, bx + bw, by + bh, C_BUTTON)
        T(bx + (bw - TW(bl)) / 2, by + (bh - FONT_SIZE) / 2, bl, C_TEXT)
    y += TEXT_LH + ITEM_SPACING_Y
    for ent in ("[12:00:09] Diagnostics copied to the clipboard.",
                "[12:00:04] Execute requested for esx_extended (not implemented).",
                "[12:00:01] Loaded C:\\scripts\\sync.lua (412 bytes) into the editor."):
        T(CX0, y, ent, C_TEXT)
        y += TEXT_LH + ITEM_SPACING_Y   # TextWrapped line advance

    # --- the landing column is the widest in the shell, but the hint and the
    # uptime line are single ImGui::Text calls, and the Copy/Clear pair shares
    # its line with the heading: assert all of it fits ---
    cw = CX1 - CX0
    for t in (hint, diag, "Session activity"):
        assert TW(t) <= cw, "landing line clipped: " + t
    assert x_copy >= CX0 + TW("Session activity") + ITEM_SPACING_X, \
        "Copy/Clear row collides with the heading"
    assert y <= CY1 + 0.5, "landing overflows: %.1f > %.1f" % (y, CY1)


if TAB == "config":
    # Warn card (KindColor -> red bar): the honest answer to flipping one of the
    # two inert switches.  Safemode is drawn off above, because this is the frame
    # right after that flip.
    # row 202: the warn bar joined the project's ramp red (255,107,107).
    draw_toast("Safe Mode is not implemented; the switch has no effect.", RGBA(255, 107, 107))
elif TAB in ("resources", "menus", "default"):
    # No toast here: the frame shows the tab right after opening it, and a card
    # anchored 10px off the bottom-right would cover content these views exist
    # to show (the dump button block, row 86 / the Run stubs).
    pass
else:
    draw_toast("Dump saved to Dumps", C_ACCENT)      # Success card


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
print("geschrieben:", OUT, "(%s-Tab)" % TAB, os.path.getsize(OUT), "Bytes", canvas.size)
