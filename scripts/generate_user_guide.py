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
TOPICS_PATTERN = re.compile(
    r"static const GuideTopic guide_topics\[\]\s*=\s*\{(?P<body>.*?)^\};",
    re.DOTALL | re.MULTILINE,
)
STRING = r'"(?:\\.|[^"\\\n])+"'
PAGES = rf"\{{\s*{STRING}(?:\s*,\s*{STRING}){{{PAGES_PER_TOPIC - 1}}}\s*,?\s*\}}"
ENTRY_PATTERN = re.compile(
    rf"\s*\{{\s*{STRING}\s*,\s*{STRING}\s*,\s*{PAGES}\s*,\s*{PAGES}\s*,?\s*\}}\s*(?:,|$)"
)


def verify(source):
    topics_match = TOPICS_PATTERN.search(source)
    if not topics_match:
        raise ValueError("guide_topics table was not found")

    body = topics_match.group("body").strip()
    position = 0
    topic_count = 0
    while position < len(body):
        entry = ENTRY_PATTERN.match(body, position)
        if entry is None:
            raise ValueError(
                f"Topic {topic_count + 1} must have two nonempty titles and "
                f"{PAGES_PER_TOPIC} nonempty pages per language"
            )
        position = entry.end()
        topic_count += 1

    if topic_count != TOPIC_COUNT:
        raise ValueError(f"Expected {TOPIC_COUNT} guide topics, found {topic_count}")
    return topic_count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify the current guide table (the default)",
    )
    parser.parse_args()

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
