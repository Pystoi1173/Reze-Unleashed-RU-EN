#!/usr/bin/env python3
"""Verify (default) or explicitly regenerate the embedded bilingual guide text."""

import argparse
import json
from pathlib import Path
import re
import struct
import sys


ROOT = Path(__file__).resolve().parents[1]
GUIDE_DIR = ROOT / "applications" / "settings" / "user_guide"
TEXT_PATH = GUIDE_DIR / "guide_text.json"
SOURCE_PATH = GUIDE_DIR / "user_guide.c"
TOPIC_COUNT = 12
STRINGS_PER_TOPIC = 8
WINDOW_SZ2 = 11
LOOKAHEAD_SZ2 = 4
ARRAY_PATTERN = re.compile(
    r"(static const uint8_t guide_text_compressed\[\] = \{)(.*?)(\r?\n\};)",
    re.DOTALL,
)


def unique_match(pattern, source, label):
    if isinstance(pattern, str):
        pattern = re.compile(pattern, re.DOTALL)
    matches = list(pattern.finditer(source))
    if len(matches) != 1:
        raise ValueError(f"Expected exactly one {label} in user_guide.c")
    return matches[0]


def integer_array(source, name):
    match = unique_match(
        rf"static const uint(?:8|16)_t {name}\[[^=]*=\s*\{{(.*?)\}};",
        source,
        name,
    )
    body = match.group(1)
    if re.sub(r"[\d\s,{}]", "", body):
        raise ValueError(f"Unsupported initializer for {name}")
    return [int(value) for value in re.findall(r"\d+", body)]


def load_strings(path):
    topics = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(topics, list) or len(topics) != TOPIC_COUNT:
        raise ValueError(f"guide_text.json must contain exactly {TOPIC_COUNT} topics")
    strings = []
    labels = []
    for index, topic in enumerate(topics):
        keys = {"title_en", "title_ru", "pages_en", "pages_ru"}
        if not isinstance(topic, dict) or set(topic) != keys:
            raise ValueError(f"Topic {index + 1} must have exactly {sorted(keys)}")
        for key in ("title_en", "title_ru", "pages_en", "pages_ru"):
            values = topic[key]
            if key.startswith("pages_"):
                if not isinstance(values, list) or len(values) != 3:
                    raise ValueError(f"Topic {index + 1}: {key} must have three pages")
            else:
                values = [values]
            for page, value in enumerate(values):
                label = f"topic {index + 1} {key}"
                if key.startswith("pages_"):
                    label += f"[{page}]"
                if not isinstance(value, str) or not value or "\0" in value:
                    raise ValueError(f"{label} must be a nonempty string without NUL")
                strings.append(value.encode("utf-8"))
                labels.append(label)
    return strings, labels


def decode_blob(blob, heatshrink2):
    # CompressHeader in lib/toolbox/compress.c is <BBH on the firmware target.
    if len(blob) < 4:
        raise ValueError("Compressed guide header is truncated")
    compressed, reserved, size = struct.unpack("<BBH", blob[:4])
    if compressed != 1 or reserved != 0 or size != len(blob) - 4:
        raise ValueError("Unexpected CompressHeader or compressed payload size")
    return heatshrink2.decompress(
        blob[4:], window_sz2=WINDOW_SZ2, lookahead_sz2=LOOKAHEAD_SZ2
    )


def generate(source, strings, labels, heatshrink2):
    match = unique_match(ARRAY_PATTERN, source, "compressed guide array")
    body = match.group(2)
    if re.sub(r"0x[0-9a-fA-F]{2}|[\s,]", "", body):
        raise ValueError("Unsupported compressed guide initializer")
    existing_blob = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", body))
    existing_data = decode_blob(existing_blob, heatshrink2)
    for name, value in (("window_sz2", WINDOW_SZ2), ("lookahead_sz2", LOOKAHEAD_SZ2)):
        config = unique_match(rf"\.{name}\s*=\s*(\d+)", source, name)
        if int(config.group(1)) != value:
            raise ValueError(f"Firmware {name} must be {value}")
    offsets = integer_array(source, "guide_text_offsets")
    ids = integer_array(source, "guide_topic_string_ids")
    count = TOPIC_COUNT * STRINGS_PER_TOPIC
    if ids != list(range(count)) or len(offsets) != count:
        raise ValueError("Topic IDs/offsets no longer match the fixed 12-topic layout")
    if offsets[0] != 0 or any(a >= b for a, b in zip(offsets, offsets[1:])):
        raise ValueError("Guide offsets must start at zero and increase strictly")
    ends = offsets[1:] + [len(existing_data)]
    for offset, end in zip(offsets, ends):
        if end > len(existing_data) or b"\0" not in existing_data[offset:end]:
            raise ValueError("Existing guide offsets do not identify terminated strings")
        existing_data[offset:end].split(b"\0", 1)[0].decode("utf-8")

    # Offsets and IDs are deliberately not generated. Keep their byte slots intact.
    chunks = []
    for index, (value, label) in enumerate(zip(strings, labels)):
        chunk = value + b"\0"
        if index + 1 < len(offsets):
            capacity = offsets[index + 1] - offsets[index]
            if len(chunk) > capacity:
                raise ValueError(
                    f"{label}: {len(value)} UTF-8 bytes exceeds the fixed slot's "
                    f"{capacity - 1} bytes; changing offsets requires a separate firmware edit"
                )
            chunk = chunk.ljust(capacity, b"\0")
        chunks.append(chunk)
    data = b"".join(chunks)
    if len(data) > 65535:
        raise ValueError("Guide text exceeds the uint16_t offset range")
    payload = heatshrink2.compress(
        data, window_sz2=WINDOW_SZ2, lookahead_sz2=LOOKAHEAD_SZ2
    )
    if len(payload) > 65535:
        raise ValueError("Compressed guide exceeds the CompressHeader size limit")
    blob = struct.pack("<BBH", 1, 0, len(payload)) + payload
    if decode_blob(blob, heatshrink2) != data:
        raise ValueError("Generated guide failed decompression round-trip")
    newline = "\r\n" if "\r\n" in source else "\n"
    lines = [
        "    " + ", ".join(f"0x{value:02x}" for value in blob[start : start + 16]) + ","
        for start in range(0, len(blob), 16)
    ]
    generated = source[: match.start(2)] + newline + newline.join(lines) + source[match.end(2) :]
    for name, value in (
        ("GUIDE_TEXT_SIZE", len(data)),
        ("GUIDE_TOPIC_COUNT", TOPIC_COUNT),
        ("GUIDE_TOPIC_STRINGS", STRINGS_PER_TOPIC),
    ):
        macro = unique_match(
            rf"(?m)^#define {name}[ \t]+(\d+)[ \t]*\r?$", generated, name
        )
        generated = generated[: macro.start(1)] + str(value) + generated[macro.end(1) :]
    return generated, len(data), len(blob)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="verify byte-identical source (default)")
    mode.add_argument("--write", action="store_true", help="update only compressed bytes and size/count macros")
    args = parser.parse_args()
    try:
        import heatshrink2
    except ImportError:
        parser.exit(1, "Missing dependency: install heatshrink2==0.13.0 with this Python's pip.\n")
    try:
        original = SOURCE_PATH.read_bytes()
        source = original.decode("utf-8")
        strings, labels = load_strings(TEXT_PATH)
        generated, text_size, compressed_size = generate(source, strings, labels, heatshrink2)
        output = generated.encode("utf-8")
        if output != original:
            if not args.write:
                parser.exit(1, "Guide text is out of date or not byte-identical; review JSON and use --write explicitly.\n")
            SOURCE_PATH.write_bytes(output)
            action = "Updated"
        else:
            action = "Verified byte-identical"
        print(
            f"{action}: {TOPIC_COUNT} topics, {len(strings)} strings, "
            f"{text_size} decoded bytes, {compressed_size} compressed bytes (including header)."
        )
    except (OSError, ValueError, OverflowError) as error:
        parser.exit(1, f"Guide generation failed: {error}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
