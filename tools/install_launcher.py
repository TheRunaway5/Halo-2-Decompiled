#!/usr/bin/env python3
"""Copy the built local launcher and optionally install desktop shortcuts."""
import argparse
from pathlib import Path
import shutil


def exec_path(path):
    # Desktop Entry Exec quoting is not shell quoting.
    text = str(path)
    for character in ('\\', '"', '`', '$'):
        text = text.replace(character, '\\' + character)
    return '"' + text + '"'


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=root / 'build')
    parser.add_argument('--desktop', action='store_true', help='Also add a desktop/app-menu shortcut')
    parser.add_argument('--title-first', action='store_true', help='Install the title-first launcher separately')
    args = parser.parse_args()
    executable = 'Halo2-Title-Launcher' if args.title_first else 'Halo2-Launcher'
    source = args.build.resolve() / executable
    if not source.is_file():
        parser.error('Build first: cmake --build build --target halo2_launcher')
    launcher = root / executable
    shutil.copy2(source, launcher)
    launcher.chmod(0o755)
    name = 'Halo 2 Native (Title)' if args.title_first else 'Halo 2 Native'
    entry = (f'[Desktop Entry]\nType=Application\nName={name}\n'
             'Comment=Launch the original Halo 2 title screen in the native workbench\n'
             f'Exec={exec_path(launcher)}\nIcon={root / "resources/launcher.svg"}\n'
             'Terminal=false\nCategories=Game;\nStartupNotify=false\n')
    shortcut = root / (name + '.desktop')
    shortcut.write_text(entry)
    shortcut.chmod(0o755)
    if args.desktop:
        menu_name = 'halo2-native-title.desktop' if args.title_first else 'halo2-native.desktop'
        destinations = [Path.home() / '.local/share/applications' / menu_name]
        if (Path.home() / 'Desktop').is_dir():
            destinations.append(Path.home() / 'Desktop' / (name + '.desktop'))
        for destination in destinations:
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(shortcut, destination)
            print(destination)
    print(launcher)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
