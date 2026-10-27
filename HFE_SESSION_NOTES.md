# eX1 HFE support — session notes (2026-10-27)

## Summary

eX1 now mounts HxC `.hfe` floppy images, and the copy-protected sample
`earthSDF_hfev3.hfe` (The Earth Self Defense Force) boots to its credits screen
in about 60 seconds. The ask was to think through HFEv3 support, build the
smallest version that works ("Tier 1"), and test it on the sample.

What shipped (all uncommitted in the working tree, core files staged):

- `DISK::hfe_to_d88()` in `src/vm/disk.cpp` (+ declaration in `disk.h`):
  decodes the MFM bitstream at mount time into the in-memory D88 the emulator
  already understands. No changes to the floppy controller emulation.
- `.hfe` wired into `DISK::open()`, the drag-and-drop list in `src/sdl/main.cpp`,
  and the `.app` document types in `mkapp.sh`.
- `hfe2d88.py` (not committed): standalone Python 3 port of the converter,
  output verified identical to the C++ path.
- `EX1_AUTOSHOT_FRAMES` environment variable so headless screenshots can run
  longer than the default 300 frames.
- `disk.cpp` and `disk.h` are now git-tracked core edits (the fetch script would
  otherwise overwrite them with upstream); their hunks were appended to
  `core-vs-upstream-20251109.patch`. README and HANDOFF updated.

The interesting part was not the format. The game's loader runs a statistical
test over 32 reads of a weak-bit sector, and HFEv3's weak-bit encoding throws
away the statistics. A run-length rule for weak regions makes the test pass;
details below.

## What the sample image contains

The file holds one revolution per track, not several: each side of each track
is about 100,176 cells with a single SETINDEX opcode at the start. At 250 kbit/s
and 300 rpm that is one 200 ms revolution (about 6,260 bytes, matching the
emulator's 2D track size of 6,250). The HxC hardware loops the track buffer,
which is why it reads as continuous data.

| Property          | Value                                                              |
| ----------------- | ------------------------------------------------------------------ |
| Geometry          | 40 tracks, 2 sides, MFM                                            |
| Header bit rate   | 300 kbit/s (dumped on a 360 rpm drive; irrelevant for emulation)   |
| Plain tracks      | side 1 everywhere; side 0 on T0 and T10–T39: 16 x 256-byte sectors |
| Protected tracks  | side 0, T1–T9                                                      |

Each protected track carries the normal 16 sectors plus three extra 1,024-byte
sectors numbered 245, 246 and 247 (size code N=3), all with a bad data CRC.
Sectors 246 and 247 sit on top of normal sectors: their 1,024-byte data fields
physically contain the ID marks of sectors 1–3 (or of 247 itself on T5–T9).
Sector 245 starts about 2,160 cells before the end of the track, so its data
wraps across the index hole into the start of the track. On T1, T4 and T9 some
cells inside 245 and 246 are marked RAND (weak bits).

The protection check lives in a 768-byte loader at T39 sectors 1–3,
XOR-encrypted behind a small RST 38h decrypt stub. It only ever reads T1 side 0
sector 245.

## HFE format facts learned

The published spec
(https://hxc2001.com/download/floppy_drive_emulator/SDCard_HxC_Floppy_Emulator_HFE_file_format.pdf)
covers the layout but is vague on v3 semantics, so these were verified against
the sample.

- Header: signature `HXCPICFE` (v1) or `HXCHFEV3` (v3) at 0, track count at
  byte 9, side count at 10, bit rate in kbit/s at 12, track table offset (in
  512-byte blocks) at 18.
- Track table: per track, 16-bit offset (512-byte blocks) and 16-bit length.
  Track data is a run of 512-byte blocks; the first 256 bytes of each block
  belong to side 0, the last 256 to side 1.
- Bits are stored least-significant-bit first. One cell per bit; MFM data bits
  are the odd cells of each 16-cell byte, clock bits the even ones.
- v3 opcodes are stored bit-reversed exactly like data, so SETINDEX (0xF1)
  appears in the file as 0x8F. Reverse the byte, then compare.

| Opcode     | Value   | Meaning                                              | Decoder action                                   |
| ---------- | ------- | ---------------------------------------------------- | ------------------------------------------------ |
| NOP        | 0xF0    | padding                                              | skip                                             |
| SETINDEX   | 0xF1    | index pulse here                                     | start the sector scan at this cell               |
| SETBITRATE | 0xF2 nn | cell rate, nn = 36000 / (2 x kbit/s); 60 = 300 kbit/s | skip operand; one track = one revolution anyway  |
| SKIPBITS   | 0xF3 nn | next byte contributes only 8-nn bits                 | drop the first nn bits of the next data byte     |
| RAND       | 0xF4    | 8 random cells                                       | mark 8 weak cells                                |

Pitfall: an opcode can be the last byte of a 256-byte half-block with its
operand in the next half-block of the same side (256 bytes further on).
Skipping the operand with a plain pointer increment lands in the other side's
data and then decodes the real operand as cells. Operand skipping has to be a
state carried across the de-interleave loop. The first version got this wrong
and produced CRC errors on roughly a third of all sectors.

## Design: why Tier 1 and not a flux-level FDC

The emulator's `DISK` class is D88-in-memory. Every non-D88 format it supports
(td0, imd, dsk, nfd, solid images) is converted to D88 at open time by a
`*_to_d88()` function. The MB8877 controller model works at byte granularity:
a position in bytes modulo track size, one DRQ per byte, sector positions
synthesised from gap tables rather than stored. CRC-error sectors (D88 status
0xA0/0xB0), deleted marks, missing data marks, oversized sectors and even
weak bits (a CSCP-specific "unstable mask" block appended to a track, read via
`get_unstable_sector()`) are all already expressible.

So Tier 1 = one more converter, about 250 lines in `disk.cpp`, zero FDC
changes, and every other format untouched:

```
.hfe file --> de-interleave sides, strip v3 opcodes --> cell array (0/1/weak)
          --> find 0x4489 sync runs, decode IDAM/DAM, check CRC-16
          --> D88 sectors (+ unstable-mask block for weak bytes)
          --> existing DISK / MB8877 path
```

Tier 2 (a true bitstream FDC: keep the cell stream, rewrite MB8877's
search/read/write-track paths around a bit-cell position and data separator,
MAME-style) would touch the 1,800-line `mb8877.cpp`, risk every other format,
and is only needed for protections Tier 1 cannot express: non-standard sync
marks, Read Track gap-content tricks, sub-byte timing, real multi-revolution
differences. Nothing in this sample needs it.

## Implementation

Files touched:

| File                              | Change                                                                 |
| --------------------------------- | ---------------------------------------------------------------------- |
| `src/vm/disk.cpp`                 | `hfe_to_d88()` + helpers, `.hfe` branch in `DISK::open()`              |
| `src/vm/disk.h`                   | declaration                                                            |
| `src/sdl/main.cpp`                | `.hfe` in the drop list; `EX1_AUTOSHOT_FRAMES`                         |
| `mkapp.sh`                        | `hfe` document type                                                    |
| `hfe2d88.py`                      | new, standalone converter                                              |
| `core-vs-upstream-20251109.patch` | disk.cpp/disk.h hunks appended (additions only)                        |
| `README.md`, `HANDOFF.md`         | format note, session section, verification recipe                     |

Decoder rules (identical in C++ and Python):

- MFM only. Sync = runs of 0x4489; after the run, the mark byte (0xFE for an ID
  field, 0xFB/0xF8 for data). ID CRC over A1 A1 A1 FE + C H R N; data CRC over
  A1 A1 A1 + mark + data.
- D88 status: 0xA0 ID CRC error, 0xB0 data CRC error, 0xF0 no data field found
  within 64 bytes of the ID, 0x10 deleted data mark. Data size = 128 << (N & 3).
- The scan starts at the SETINDEX cell and runs one full revolution with
  modular cell addressing, so a sector straddling the index decodes correctly.
- After a data field the scan resumes right after the DAM, not after the data,
  so oversized sectors that nest other IDAMs do not swallow them.
- Weak bits: `hfe_mfm_byte()` returns a per-bit mask of which data cells were
  RAND. Masked sectors get a second D88 "track" block after the real sectors
  (CSCP unstable extension: header byte 9 = sector index, data = mask). Other
  emulators ignore the block.
- Weak-run rule (`shortcut:` comment in the code): only RAND runs of at least
  3 opcodes (24 cells, erased/no-flux areas) stay random per read; shorter runs
  are marginal cells and get a fixed random value at mount.
- Read-only: D88 protect flag set, and the save path is `<file>.hfe.D88` (same
  convention as td0), so the HFE itself is never written.

## Debugging story

Three problems, found in order by comparing the converter's D88 against an
independent Python MFM decode and by tracing the FDC.

1. **SETBITRATE operand skip across block boundaries** (format pitfall above).
   Symptom: random CRC errors and missing sectors across the disk. Fix: state
   machine for operands.
2. **Nested sectors.** After the first fix, protected tracks had 13 sectors
   instead of 19: the 1,024-byte sector 246's data field spans the IDAMs of
   sectors 1–3, and the scan skipped past them. Fix: resume scanning after the
   DAM.
3. **The loader's statistical check.** With a structurally correct D88 the
   game still hung at "IPL is loading BASIC CZ8FB02", reading T1 sector 245 in
   an endless loop. An FDC command trace showed: RESTORE, SEEK 1, READ 245
   (ends with CRC error, as required), repeat. Disassembling the loader
   (decrypt stub at D21D copied to 0000, XOR 0x5C over D016–D255, then d033)
   showed it reads sector 245 thirty-two times and keeps two things per read:
   data byte 0x0A, and the 8-bit sum of data bytes 0x20–0x2F. Then:
   - check 1: byte 0x0A must take at least 3 distinct values, and the two most
     common values together must cover fewer than 30 of the 32 reads;
   - check 2: some value of the checksum must appear in at least 8 of 32 reads.

   RAND cells in the dump: a 40-cell run over bytes 10–11 (true noise area)
   and isolated 8–16-cell runs scattered over bytes 34–47 (marginal cells).
   With CSCP's mask semantics (masked bits fully random on every read), check 1
   passes but check 2 can never pass: 10 random bytes in the checksum range.
   A Python simulation of the loader's checks against candidate models:

   | Model                                         | check 1 | check 2 |
   | --------------------------------------------- | ------- | ------- |
   | all weak bytes fully random                   | 100%    | 0%      |
   | per-cell flip probability q = 0.03            | 89%     | 100%    |
   | per-cell flip probability q = 0.08            | 100%    | 0%      |
   | long runs random, short runs fixed (shipped)  | 100%    | 100%    |

   The run-length rule is also the physically plausible reading: a long RAND
   run is an erased area the read channel turns into noise; an isolated RAND
   is one marginal flux transition. It needs no FDC change, so it shipped.

## Verification

- **Converter harness:** `disk.cpp` compiled standalone with
  `-D_ANY2D88 -DLONG_PTR=long` plus a 12-line `main` calling `open()` and
  `save_as_d88()`. The resulting D88 was dumped with Python and compared
  sector-by-sector against an independent Python MFM decode of the HFE.
- **Python parity:** `hfe2d88.py earthSDF_hfev3.hfe out.d88` produces the same
  header, track table, sector headers, masks and good-CRC data as the C++
  path (only the fixed random values chosen for short RAND runs differ by
  RNG). The Python D88 also boots the game.
- **FDC trace:** a copy of `mb8877.cpp` compiled with `-D_FDC_DEBUG_LOG` and
  `this->out_debug_log(` rewritten to `fprintf(stderr, ` (the SDL port has no
  debug-log sink). Confirmed 32 reads of sector 245 followed by a normal load
  of T1–T2 and onward.
- **Screenshots:** `EX1_AUTOSHOT=... EX1_AUTOSHOT_FRAMES=3600 ./ex1turbo
  ../earthSDF_hfev3.hfe` shows the credits screen; the same with the Python
  D88 shows the same screen.
- All three variants (`ex1`, `ex1turbo`, `ex1turboz`) build.

Not verified: FM-encoded HFEs, HFE write support, other protected titles,
behaviour of the generated D88 in other emulators, runtime of the plain-X1 and
turboZ builds with an HFE mounted (compile only).

## Limits, risks and next steps

- The 24-cell weak-run threshold is tuned on one game. Another title whose
  protection expects different statistics may fail. The next step there is a
  per-cell flip probability in the read path (`mb8877.cpp` read-sector and
  `DISK::make_track`), or a multi-revolution flux source (SCP/KryoFlux) whose
  real per-revolution samples are cycled per read.
- Sector timing on oversized tracks is gap-scaled (`invalid_format` path in
  `get_track_tmp`), not real. Timing-based protections could still fail; the
  fix would be to store real byte offsets from the cell positions.
- HFE is treated as one revolution per track; files with several SETINDEX
  opcodes would be scanned from the last one only.
- MFM only; FM marks (0xF57E/0xF56F) are not decoded.
- `hfe2d88.py` and the sample image are kept out of the repo.
