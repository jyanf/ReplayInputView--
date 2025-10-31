#!/usr/bin/env python
# -*- coding: utf-8 -*-
import fontforge
import sys, os

def mod_font_style(font_path, output_path):
    if not os.path.exists(font_path):
        print(f"[Error] File not found: {font_path}")
        sys.exit(1)

    font = fontforge.open(font_path)
    print(f"[Info] Opened font: {font.fullname}\n")

    # === 要编辑的 SFNT 名称表 ===
    print(font.sfnt_names)
    subfamilyname = None;
    for entry in font.sfnt_names:
        encoding, entry_nameID, string = entry
        if entry_nameID == "SubFamily":
            subfamilyname = string
    editable_fields = {
        1:  ("Family Name",   font.familyname),
        2:  ("SubFamily Name",     subfamilyname),
        4:  ("Full Name",     font.fullname),
        6:  ("PostScript",    font.fontname),
    }

    print("=== Edit font identification strings ===")
    for name_id, (desc, current) in editable_fields.items():
        print(f"\n[{desc}]")
        print(f"Current: {current}")
        new_val = input("New value (Enter = keep): ").strip()
        if new_val:
            editable_fields[name_id] = (desc, new_val)
        else:
            editable_fields[name_id] = (desc, current)

    # === 写入修改 ===
    for name_id, (_, value) in editable_fields.items():
        font.appendSFNTName("English (US)", name_id, value)
    font.appendSFNTName("English (US)", 3, f"FontForge2.0; Modified from {font.fullname}")
    # === 设置字体属性 ===
    font.encoding = "UnicodeFull"
    font.os2_codepages = (-1, -1)
    font.os2_unicoderanges = (-1, -1, -1, -1)
    font.os2_version = 4
    font.os2_panose = (2, 11, 6, 3, 5, 4, 5, 2, 3, 4)

    font.autoHint()
    font.autoInstr()

    # === 生成新字体 ===
    font.generate(output_path)
    print(f"\n[OK] Saved as: {output_path}")
    font.close()


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python font-expand.py input.ttf output.ttf")
        sys.exit(0)

    mod_font_style(sys.argv[1], sys.argv[2])
