#!/usr/bin/env python3
"""Extract the captured production method bytes for a private CMake fixture.

The adapter supplies observations; these definitions are not a rewritten model.
Generated C++ belongs to the enclosing run_tests.py build tree.
"""
import argparse
import hashlib
from pathlib import Path


def method(text, name):
    pos = text.index("MenuState::" + name + "(")
    start = text.rfind("\n", 0, pos) + 1
    end = text.index("{", pos) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def generate(reference):
    path = reference / "frontend_matchmaking.cpp.reference"
    text = path.read_text()
    result = ["// Captured production SHA256 " + hashlib.sha256(path.read_bytes()).hexdigest()]
    for name in ("online_matchmaking_roster_order", "online_matchmaking_rank",
                 "online_matchmaking_player_appearance", "online_matchmaking_speaker"):
        result.append(method(text, name))
    start = text.index("    const auto *playlist=selected(matchmaking_,matchmaking_.playlist_id);",
                       text.index("void MenuState::populate_online_matchmaking"))
    end = text.index('    std::string matched="Matched Player";', start)
    result.append("std::pair<std::vector<MenuEntry>,std::vector<MatchmakingProgressRow>> MenuState::populate_rows() const {\n"
                  + text[start:end] + "\nreturn {players,rows};\n}")
    return "\n".join(result) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(generate(args.reference))


if __name__ == "__main__":
    main()
