# User Guide text

The current firmware defines its 12 bilingual topics in the `guide_topics` table in `user_guide.c`. Each entry contains an English title, a Russian title, and three pages per language. Keep the topic order unchanged because the Loader uses topic indices.

After editing the table, run this read-only check from the repository root with Python 3:

```sh
python scripts/generate_user_guide.py --check
```

The script checks the table and requires only the Python standard library. Review any changed text on the 128×64 screen to check line breaks and fit.

`guide_text.json` is a reference from the preceding compressed-guide alpha. The compression instructions and generator for that format are available at the `ru-en-2026.11.18-alpha.1` tag.
