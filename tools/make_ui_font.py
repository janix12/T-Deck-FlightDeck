"""python make_ui_font.py Barlow-SemiBold.ttf > ui_font.h (requires Pillow).

Source: google/fonts, ofl/barlow/Barlow-SemiBold.ttf, SIL OFL 1.1.
TTF SHA-256: 86577cb32f8abe3673db53ca0f4221e6856751a4f6730c867e00f720f8bb1fc5
"""
import sys
from PIL import ImageFont

alphabet = "".join(chr(c) for c in range(32, 127)) + "ÁÉÍÓÖŐÚÜŰáéíóöőúüű"
print("// Generated from Barlow SemiBold. See licenses/Barlow-OFL.txt.")
print("#pragma once")
print("struct UiGlyph { uint16_t offset; uint8_t width, height, advance; int8_t left, top; };")
print("struct UiFace { const UiGlyph *glyphs; const uint8_t *pixels; int baseline, height; };")
print("static constexpr uint16_t uiAccents[] = {" + ",".join(hex(ord(c)) for c in alphabet[95:]) + "};")
for size, line in [(9, 9), (11, 10), (15, 18), (24, 27), (46, 48)]:
    font = ImageFont.truetype(sys.argv[1], size)
    metrics, pixels = [], []
    for char in (("USB",) if size == 9 else alphabet):
        mask, (left, top) = font.getmask2(char, mode="L", anchor="ls")
        width, height = mask.size
        advance = round(font.getlength(char))
        if size == 46 and char not in "0123456789:-":
            width = height = 0
        metrics.append((len(pixels), width, height, advance, left, top))
        values = [(value * 3 + 127) // 255 for value in mask] if width and height else []
        values.extend([0] * (-len(values) % 4))
        for a, b, c, d in zip(*[iter(values)] * 4):
            pixels.append(a << 6 | b << 4 | c << 2 | d)
    print(f"static constexpr UiGlyph glyphs{size}[] = {{")
    for row in metrics:
        print("  {" + ",".join(map(str, row)) + "},")
    print("};")
    print(f"static constexpr uint8_t pixels{size}[] = {{")
    for i in range(0, len(pixels), 24):
        print("  " + ",".join(f"0x{v:02x}" for v in pixels[i:i+24]) + ",")
    print("};")
    cap_height = -font.getbbox("H", anchor="ls")[1]
    print(f"static constexpr UiFace face{size} = {{glyphs{size}, pixels{size}, {(line+cap_height)//2}, {line}}};")
