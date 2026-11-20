#!/usr/bin/env python3
"""Verify the embedded bilingual User Guide table."""

import argparse
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE_PATH = ROOT / "applications" / "settings" / "user_guide" / "user_guide.c"
TOPIC_COUNT = 12
PAGES_PER_TOPIC = 3
STRINGS_PER_TOPIC = 2 + PAGES_PER_TOPIC * 2
TOPICS_PATTERN = re.compile(
    r"static const GuideTopic guide_topics\[\] = \{(?P<body>.*?)^\};",
    re.DOTALL | re.MULTILINE,
)
STRING_PATTERN = re.compile(r'"(?:\\.|[^"\\])*"')
ENTRY_PATTERN = re.compile(r"^\s*\{.*\},?$", re.MULTILINE)


def verify(source):
    topics_match = TOPICS_PATTERN.search(source)
    if not topics_match:
        raise ValueError("guide_topics table was not found")

    entries = ENTRY_PATTERN.findall(topics_match.group("body"))
    if len(entries) != TOPIC_COUNT:
        raise ValueError(f"Expected {TOPIC_COUNT} guide topics, found {len(entries)}")

    for index, entry in enumerate(entries, start=1):
        strings = STRING_PATTERN.findall(entry)
        if len(strings) != STRINGS_PER_TOPIC:
            raise ValueError(
                f"Topic {index} must contain {STRINGS_PER_TOPIC} string literals, "
                f"found {len(strings)}"
            )
        if any(value == '""' for value in strings):
            raise ValueError(f"Topic {index} contains an empty string")

    if "COUNT_OF(guide_topics)" not in source:
        raise ValueError("guide_topics must be used for the menu and topic bounds")
    if "for(size_t page = 0; page < 3; page++)" not in source:
        raise ValueError("Guide topics must contain three pages")

    return len(entries)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify the current guide table (the default)",
    )
    args = parser.parse_args()
    del args

    try:
        source = SOURCE_PATH.read_text(encoding="utf-8")
        topic_count = verify(source)
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, f"Guide verification failed: {error}\n")

    print(
        f"Verified current guide table: {topic_count} topics, "
        f"{PAGES_PER_TOPIC} pages per topic, Russian and English strings."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
