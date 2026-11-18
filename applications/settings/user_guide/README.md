# Editable user guide text

`guide_text.json` is the preferred editable source for the 12 guide topics. It
contains the exact 96 UTF-8 strings recovered from `user_guide.c`, including the
original `\n` screen line breaks. Each topic has `title_en`, `title_ru`,
`pages_en` (three pages), and `pages_ru` (three pages). Keep topic order unchanged;
the loader uses these topic IDs. Other UI labels remain in C.

## Dependency and usage

Use Python 3 with `heatshrink2==0.13.0` (the firmware toolchain Python already has
it). Run from the repository root:

```powershell
# If needed, install into the Python environment used below:
python -m pip install heatshrink2==0.13.0

# Read-only verification; these commands are equivalent:
python scripts/generate_user_guide.py
python scripts/generate_user_guide.py --check

# Explicitly opt in after editing and reviewing the JSON:
python scripts/generate_user_guide.py --write
python scripts/generate_user_guide.py --check
```

On Windows, replace `python` with
`.\toolchain\x86_64-windows\python\python.exe` to use the bundled interpreter.
Paths are resolved relative to the script, not the working directory.

Verification reconstructs the compressed array and compares the **entire C file
byte for byte**, preserving its line endings. A mismatch exits with status 1 and
never writes. `--write` changes only `guide_text_compressed` and the
`GUIDE_TEXT_SIZE`, `GUIDE_TOPIC_COUNT`, and `GUIDE_TOPIC_STRINGS` macro values;
it does not build firmware or touch other files. An already matching file is not
rewritten.

## Fixed layout and compression

The existing `guide_text_offsets` and `guide_topic_string_ids` arrays are
intentionally not regenerated. Each non-final string must fit its existing UTF-8
byte slot (including its terminating NUL); shorter edits are padded with NULs to
preserve all later offsets. Oversized edits fail before writing. The final string
can change length. Reordering/adding topics or expanding other slots needs a
separate firmware layout change, outside this generator's scope. Review screen
line lengths manually; byte-slot validation does not check visual layout.

The generator uses heatshrink window bits 11 and lookahead bits 4, matching
`guide_text_init`. The four-byte header is `01 00` followed by the little-endian
16-bit compressed payload length, as defined by `CompressHeader` in
`lib/toolbox/compress.c` (not the streamed asset header). The unchanged original
has 4,648 decoded bytes and 2,365 compressed bytes including its header.
