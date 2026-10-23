#!/usr/bin/env python3
"""Generate the fixed Traditional-Chinese UI subset; no network or global install.

Source: google/fonts at e7db6ca603fd4862010383ff17ffd6cc4ffac1c0,
ofl/notosanstc/NotoSansTC[wght].ttf. License: assets/fonts/OFL-NotoSansTC.txt.
Requires fonttools 4.61.1 and lv_font_conv 1.5.3 (see assets README).
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile
from fontTools.ttLib import TTFont
from fontTools import subset
from fontTools.varLib.instancer import instantiateVariableFont
ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--converter', default='lv_font_conv')
    args = parser.parse_args()
    content = args.source.read_bytes()
    expected = '864727d210d54f2537bbe23b3a839436c3992af72de9322af5270897246bd44f'
    if hashlib.sha256(content).hexdigest() != expected:
        raise SystemExit('Unexpected source-font SHA-256; use the pinned source.')
    chars = set(chr(code) for code in range(32, 127))
    for name in ['main/sax_ui.c', 'main/sax_diagram.c']:
        chars.update(ch for ch in (ROOT/name).read_text() if ord(ch) > 127)
    codes = sorted(map(ord, chars))
    font = TTFont(args.source)
    missing = set(codes) - set(font.getBestCmap())
    if missing:
        raise SystemExit(f'Source font lacks {sorted(missing)}')
    options = subset.Options()
    options.recalc_timestamp = False
    sub = subset.Subsetter(options=options)
    sub.populate(unicodes=codes)
    sub.subset(font)
    font = instantiateVariableFont(font, {'wght': 500}, inplace=True)
    for record in font['name'].names:
        if record.nameID in (1, 4, 6, 16):
            record.string = 'SaxTCUI'.encode(record.getEncoding())
    target = ROOT/'assets/fonts/sax_tc_14.c'
    with tempfile.TemporaryDirectory(prefix='sax-font-') as temp:
        source = Path(temp)/'SaxTCUI-500-subset.ttf'
        font.save(source, reorderTables=True)
        subprocess.run([
            args.converter, '--font', str(source),
            '--range', ','.join(f'0x{c:X}' for c in codes),
            '--size', '14', '--bpp', '4', '--format', 'lvgl',
            '--no-compress', '--no-kerning', '--lv-font-name', 'sax_tc_14',
            '--lv-include', 'lvgl.h', '--output', str(target),
        ], check=True)
        text = target.read_text().replace(str(source), 'SaxTCUI-500-subset.ttf')
        text = text.replace(str(target), 'assets/fonts/sax_tc_14.c')
        target.write_text(text)
    (ROOT/'assets/fonts/sax_tc_14.txt').write_text(''.join(map(chr,codes))+'\n')
    print(f'Generated {len(codes)} glyphs: {target.name}')

if __name__ == '__main__':
    main()
