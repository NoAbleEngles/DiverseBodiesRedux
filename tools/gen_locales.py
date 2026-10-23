"""Regenerates assets/F4SE/Plugins/DiverseBodiesRedux/locales/*.json: en from ui/src/i18n/en.ts
(the key list), every other Fallout 4 language from tools/locales_data.py. Checks that every
table has exactly en.ts' keys and placeholders. Run from the repo root:
    python -I -X utf8 tools/gen_locales.py"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from locales_data import LOCALES  # noqa: E402

EN_TS = 'ui/src/i18n/en.ts'
OUT_DIR = 'assets/F4SE/Plugins/DiverseBodiesRedux/locales'


def write_locale(code, display, strings):
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(f'{OUT_DIR}/{code}.json', 'w', encoding='utf-8', newline='\r\n') as f:
        json.dump({'displayName': display, 'strings': strings}, f, ensure_ascii=False, indent=4)


def unescape(text):
    return re.sub(r'\\u([0-9a-fA-F]{4})', lambda m: chr(int(m.group(1), 16)), text).replace('\\"', '"')


def main():
    source = open(EN_TS, encoding='utf-8').read()
    en = {k: unescape(v) for k, v in re.findall(r'^\s*"([^"]+)":\s*"((?:[^"\\]|\\.)*)",?\s*$', source, re.M)}

    problems = []
    for code, (_, strings) in LOCALES.items():
        missing, extra = set(en) - set(strings), set(strings) - set(en)
        if missing or extra:
            problems.append(f'{code}: missing {sorted(missing)} extra {sorted(extra)}')
        for key, text in strings.items():
            if key in en and set(re.findall(r'\{\w+\}', text)) != set(re.findall(r'\{\w+\}', en[key])):
                problems.append(f'{code}: placeholder mismatch in "{key}"')
    if problems:
        raise SystemExit('locale tables out of sync with en.ts:\n  ' + '\n  '.join(problems))

    write_locale('en', 'English', en)
    for code, (display, strings) in LOCALES.items():
        write_locale(code, display, strings)
    print(f'{len(en)} keys, {len(LOCALES) + 1} locales -> {OUT_DIR}')


if __name__ == '__main__':
    main()
