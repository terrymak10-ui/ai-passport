**English** · [简体中文](eb-sax.zh_CN.md)

# Eb Alto Saxophone — first increment

Offline reference-note and fingering browser, based on main commit
`3d685a90bdb96dd8526d84edde1e63fec25b460e`. Entry: `main/sax_app.c`.
The hardware BSP and default partitions are unchanged. No radio stack is started.

## Controls and display

- UP selects the previous lower note; DOWN selects the next higher note. Ends clamp.
- OK starts a continuous sine reference; press OK again to stop. Playback uses the selected codec volume (default 80/100). Start quietly and
  keep the speaker away from ears. A double click is treated as one action.
- Hold OK toggles WRITTEN / CONCERT. Written is the startup default.
- Hold UP / DOWN to raise / lower codec volume by 10, clamped to 40–100. Release before each next step. Volume is shown on screen, resets to 80 at startup, and is locked during playback.
- Playback locks note selection, volume and mode changes so display and sound remain consistent. OK remains active to stop. Wait for STOPPING to finish before selecting another note or starting again; queued PCM is drained after a 10 ms release. Stop response time requires device measurement.
- After 60 seconds idle the backlight dims to 5%; a key restores brightness.
- The top-right battery displays `--%` if unavailable.

Fifteen C-major natural notes, written C4 through C6, are included. This is a
limited initial note set, not a chromatic fingering chart. Six circles represent
LH and RH index/middle/ring fingers from left to right. Filled means closed;
outline means open. Oct is the thumb octave key; C is the right little-finger
low-C key. The fingering always follows written pitch even in CONCERT mode.
Other auxiliary keys remain released. This basic conventional fingering table
is authored for this app; alternate fingerings and instrument-specific mechanisms
are outside this increment. A saxophone player must check the chart on hardware.

Internal reference pitch is A4=440 Hz. Sounding MIDI = written MIDI - 9.
Written C5 therefore plays Eb4 (311.12698 Hz), never concert C5. Names use ASCII
accidentals (Eb, F#) and scientific pitch octaves. The interface is English;
Traditional Chinese font integration remains pending.

## Architecture and tests

`main/sax_music.c` is hardware-independent: note data, frequency/name conversion,
and a phase-continuous sine oscillator with a Flash lookup table and 10 ms fades.
No per-sample floating-point trigonometry is used. A dedicated audio task owns
blocking PCM writes. Input callbacks only enqueue, and UI access holds the BSP
LVGL lock. PCM uses a 512-byte worker buffer. There is no recording buffer or
practice-statistics persistence yet. Queues and workers live for application
lifetime; the app has one permanent screen.

`tests/test_sax_music.c` checks transposition examples, pitch frequency, note
bounds/order, key data, waveform peak/RMS, bounded volume steps, phase continuity across different
block sizes, fade endpoints, play/stop state transitions, continuous streaming beyond one second, and A4 zero-crossing count. It is part of
`tools/validate.sh`. Host tests do not establish acoustic accuracy.

Run the complete gate in ESP-IDF 5.5.3:

```sh
./tools/validate.sh
```

## Pending acceptance and later increments

On the device verify boot, every note and key indicator, concert switching,
clamped navigation, sound quality/pitch with an independent tuner, repeated
playback, battery fallback, idle dimming, and no watchdog/reboot. Observe minimum
heap and largest free block during repeated playback. No physical checks have
been performed for this application yet.

Still pending: microphone quality diagnostics, YIN tuner, cents display,
metronome, scales/follow-along, long-tone training, dated practice statistics,
chromatic/alternate fingerings, Traditional Chinese fonts, and automatic sleep.
No ±1-cent, 50-ms, or 1-ms performance claim is made. The default partition has
no OTA or filesystem. Future daily statistics require manual date setup or an
appropriate clock source after complete power loss.

Build outputs must pass the shared merge/layout verifier. Do not flash without
explicit approval for the exact build; a merged image at 0x0 can reset NVS.
Source publication on the feature branch is authorized; merging, releases, and device flashing are not.

Audio follow-up: the initial 45/100 codec setting mapped to approximately -27.5 dB before board gain compensation. The test revision uses the baseline audio demo's 80/100 setting (approximately -10 dB), leaving PCM amplitude and fades unchanged. This raises nominal output by 17.5 dB but is not proof of audible hardware output. Serial logs report reference frequency, format setup and PCM completion; successful writes do not prove speaker output. On the initial device build, UI rendering and a completed playback status were observed, but sound was not confirmed. Retest acoustic output with the new revision.

Second audio follow-up: the user confirmed C6 is audible only very close to the ear with the 80/100 build. PCM sine peak is now 16000 instead of 5000 (nominal +10.1 dB, 16-bit headroom retained); 10 ms fades are unchanged. Volume is adjustable up to the existing public API limit of 100. Do not alter board gain assumptions or codec registers to compensate without measurements. Start playback away from ears, then increase volume one step at a time if needed. Actual loudness, distortion, low-frequency response and usability remain unverified. A successful write/status is not acoustic acceptance.

Continuous playback: OK toggles play/stop with explicit STOPPED/PLAYING/STOPPING states. The audio worker checks a generation-tagged stop notification between 256-sample blocks, keeping stop independent of the start queue. A stop arriving before the worker starts is retained; stale stops cannot cancel a later playback. Phase and attack position persist across blocks, without a one-second loop seam or repeated fade. End playback before changing note, volume or display mode. The backlight does not idle-dim during playback. Continuous audio power, thermal behavior and stop latency need hardware acceptance.
