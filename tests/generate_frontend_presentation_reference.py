#!/usr/bin/env python3
"""Extract unchanged captured native text methods for an owned CMake build."""
import argparse
import hashlib
from pathlib import Path


def definition(text, token):
    pos = text.index(token)
    start = text.rfind("\n", 0, pos) + 1
    end = text.index("{", pos) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    path = args.reference / "frontend_matchmaking.cpp.reference"
    text = path.read_text()
    definitions = [definition(text, token) for token in (
        "const OnlinePlaylist *selected(", "std::optional<UiVariant> matched_variant(",
        "std::string matched_map_name(", "std::string replace_token(",
        "MenuState::online_matchmaking_original_text(", "MenuState::online_matchmaking_text(")]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("// Captured source SHA256 " + hashlib.sha256(path.read_bytes()).hexdigest()
                           + "\n" + "\n".join(definitions) + "\n")


if __name__ == "__main__":
    main()
