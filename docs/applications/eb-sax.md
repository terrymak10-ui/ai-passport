**English** · [简体中文](eb-sax.zh_CN.md)

# Eb alto saxophone practice app

Offline ESP32-C3 / 8 MB Flash / no-PSRAM application, based on main commit
`3d685a90bdb96dd8526d84edde1e63fec25b460e`. Entry: `main/sax_app.c`.
The BSP, GPIO map and default partitions are unchanged. No radio stack starts.

## Notes and controls

The browser now covers all **33 consecutive semitones, written Bb3–F#6**
(MIDI 58–90). Startup selects written C5. A4=440 Hz; sounding MIDI = written MIDI - 9.
The concert range is Db3–A5, approximately 138.59–880 Hz. Written C5 plays concert
Eb4 at 311.12698 Hz. Display defaults to written pitch; changing to concert pitch does not
change the fingering or playback frequency.

- Short UP/DOWN selects a lower/higher semitone, clamped at both ends.
- Short OK toggles continuous play/stop. A double click is treated as one action.
- Hold OK switches written/concert display while stopped.
- Hold UP/DOWN changes codec volume in steps of 10 within 40–100. Release before
  each step. Startup volume is now **90** (previously 80); it is not persisted.
- Note, mode and volume changes are locked during playback. OK remains active to
  stop; wait for STOPPING to finish before starting or changing settings.
- Idle backlight dims after 60 seconds when stopped; keys restore brightness.
- Battery appears top-right or `--%` if unavailable. UI labels and finger instructions use a fixed Traditional Chinese subset.

Flat names use ASCII b; F# uses #. The alias below the large note shows the enharmonic
spelling (for example Db4 / C#4, F#4 / Gb4). Numbering uses scientific pitch octaves.

## Fingering diagram

An original 240 x 185 schematic replaces the auxiliary-key grid. Six main
circles run vertically: the upper three are left index/middle/ring, the lower
three right index/middle/ring. Traditional Chinese names sit beside their keys.
**Gold fill and gold finger names mean press; hollow gray keys mean release.**
Two lines below list the fingers to press for each hand, or all fingers released.
For C5 this is left middle only; for C4 it is left index/middle/ring and right
index/middle/ring/pinky. Palm and side actions are distinguished from main keys.

The drawing shows player-operated touches, not all coupled pads. It is a
schematic front view, with the back octave touch offset as a thumb callout.
Small auxiliary labels are grouped by the hand that operates them:

| Label / location | Meaning / hand |
| --- | --- |
| T, next to left-thumb caption | Left-thumb octave key |
| G# / C# / B / Bb, lower left | Left-pinky G# / low C# / B / Bb |
| C / Eb, lower right | Right-pinky low C / Eb |
| Bb, upper right | Right-index side Bb, not LH bis Bb |
| D / Eb / F, upper left | Left-hand palm keys |
| E, upper right | Right-index upper side E |
| F#, beside right-middle-side caption | Dedicated right-middle high F# touch |

Choose one primary common fingering per note: side Bb for Bb4/Bb5, RH middle
finger for F#4/F#5, palm keys for D6 and above. F#6 uses the high-F# key; its
screen explicitly says **High-F# key required**. A horn without that key needs an
instrument-appropriate alternate, which is not presented as universal here.
Other alternate, front-key, bis and altissimo fingerings are outside this version.

Sources used to cross-check key combinations (no source graphics copied):

- [Yamaha acoustic saxophone fingering guide](https://www.yamaha.com/en/musical_instrument_guide/saxophone/play/play002.html).
- Timothy Reichard's Woodwind Fingering Guide: [first octave](https://www.wfg.woodwind.org/sax/sax_bas_1.html), [second octave](https://www.wfg.woodwind.org/sax/sax_bas_2.html), [key scheme](https://www.wfg.woodwind.org/sax/sax_fing.html), and [high F# variants](https://www.wfg.woodwind.org/sax/sax_alt_4.html).
- [NoteAtlas high F# primary key combination](https://www.noteatlas.org/instruments/alto-sax/notes/high-f-sharp/).

These are reference fingerings, not a measured guarantee for the user's horn.
A saxophone teacher/player should check all 33 indicators on the actual device.

## Low-note playback profile

Earlier hardware tests found C6 audible but low notes very weak even at VOL 100.
This version changes timbre rather than attempting unbounded gain:

- At sounding frequencies up to 311.13 Hz (written C5 and below), use the
  fundamental plus its second/third/fourth harmonics, with weights
  `1 : 0.65 : 0.45 : 0.30`, normalized to a PCM peak of **20000**.
- Crossfade toward a pure sine from 311.13 to 440 Hz. At and above concert A4
  (written F#5), retain the **16000**-peak sine.
- Every added partial is an integer multiple of the correct concert fundamental;
  no octave substitution is used. This reference is a synthesized rich tone,
  not a recording or simulation of a saxophone.
- A higher startup volume (90) is 5 dB above the previous 80 on the dependency's
  default volume curve, before board gain compensation. Actual SPL is unmeasured.
- Table interpolation and profile crossfade are convex. The generated PCM never
  exceeds +/-20000, with no hard clipping; 10 ms start/stop fades remain.

Both tables are constant Flash data; the additional table is 514 bytes. Rendering
uses integer table interpolation per sample, not per-sample trigonometry. The
512-byte PCM worker buffer, 24 KiB LVGL pool and LCD DMA configuration are unchanged.
The entire diagram is one custom-drawn LVGL object, rather than 19 key widgets.
`main/sax_diagram.c` owns geometry and finger projection; `main/sax_ui.c` owns
rendering and labels. The controller and audio worker remain in `sax_app.c`.

The user reported clearer playback after flashing the previous 33-note/rich-tone
image (SHA-256 `e9aac0bf7143ad128a1b590dd1aee09568bfce8518a932b348f6bfe36cb1d572`).
This is subjective feedback; every frequency and listening distance remain unmeasured. Digital peak bounds do not establish
analog headroom, speaker response, normal-room loudness, power or thermal safety.
Start with the speaker away from ears, then adjust volume gradually. Do not
assume VOL 100 or a successful PCM write means the acoustic output is acceptable.

## Playback architecture and verification

`main/sax_music.c` owns hardware-independent notes, naming, frequency conversion,
volume steps, waveform profiles, envelope and play/stop state. A dedicated worker
owns PCM I/O. Input callbacks enqueue only; UI objects use the BSP LVGL lock.
STOPPED/PLAYING/STOPPING states and generation-tagged stop notifications prevent
stale stops from cancelling a newer playback. Phase survives block boundaries;
the envelope counter saturates after attack. Stop emits a 10 ms release and
silence drain before acknowledgement. Actual stop latency is unmeasured.

`tests/test_sax_music.c` checks all 33 consecutive MIDI notes, transposition,
octave-related main/aux fingerings, lower/palm/high-F# cases, aliases, semitone
frequency ratios, volume bounds, continuous playback, fades, and peak/RMS bounds.
Spectral tests check that low-tone fundamental and harmonics have the intended
ratios and that concert A4 remains a pure tone. Host checks cannot validate
actual speaker sound or physical fingering ergonomics.

```sh
./tools/validate.sh
```

## Font and host preview

`assets/fonts/sax_tc_14.c` is an uncompressed 14 px / 4 bpp, 131-glyph fixed UI
subset with 17 px line height, generated from a pinned OFL font. See the
[asset inventory and reproduction command](../../assets/README.md#sax-ui-font).
All body labels and drawing callouts explicitly select it; the large note and
auxiliary ASCII glyphs deliberately use Montserrat. This does not support
arbitrary Chinese user input. Defaults explicitly enable UTF-8 and placeholders.

The optional real-LVGL host renderer uses the same UI/font, 24 KiB LVGL pool,
19.2 KiB RGB565 partial buffer and the BSP's rounded-corner predicate. Run after
ESP-IDF has downloaded the pinned Managed Components (CMake/Ninja/C compiler):

```sh
cmake -S tests/sax_ui_host -B /tmp/sax-ui-host -G Ninja
cmake --build /tmp/sax-ui-host -j3
/tmp/sax-ui-host/preview assets/fonts/sax_tc_14.txt /tmp/sax-preview
```

It checks glyph coverage (including a known absent character), actual widget
font binding, diagram bounds, both hand-text widths for all 33 notes, 66 renders
and 100 playback-state updates. It emits C4/C5/F#6 PPM previews. A host run with
assertions enabled retained 7032 bytes free / 6632 bytes largest block in the
24 KiB pool; this is not the board's system heap. The static gate separately
checks diagram geometry, complete main/aux mapping and UTF-8 all-or-empty copies.
[Preview PNGs](../../assets/README.md#sax-ui-previews) are simulated, not device photos.

## Hardware acceptance and remaining work

Current version: **device tests not run**. Verify all 33 notes/auxiliary labels,
readability/clipping, high-F# warning, audible Bb3/B3/C4/B4/C5/C6 at a normal
listening distance, timbre/pitch with an independent tuner, distortion, continuous
play/stop/restart, battery fallback, idle dimming, minimum heap/largest free block,
watchdogs, power and temperature during longer playback. If low notes remain
inaudible, investigate the actual speaker/amp path instead of assuming more gain.

Previous 33-note version: user confirmed successful flashing and substantially clearer
volume. The new schematic/Traditional Chinese UI has not been tested on hardware.

Pending: microphone/YIN/cents, metronome, scales/follow-along, long-tone training,
dated statistics, alternate/altissimo fingerings and automatic sleep. No +/-1-cent, 50-ms or 1-ms performance claim is made. Default
partitions have no OTA/filesystem; dated records after power loss need a suitable
clock source or manual date. Publication is limited to the authorized feature
branch/draft PR; no merge, release or device flash is automatic. A merged image
at 0x0 may reset NVS; confirm the exact build and target before every flash.
