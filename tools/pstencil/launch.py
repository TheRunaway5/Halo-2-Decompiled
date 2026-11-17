#!/usr/bin/env python3
"""Launch pStencil through the current qualified MCC/native-kernel owner."""
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]


def launcher_arguments(arguments):
    if arguments == ['--launcher-info']:
        return arguments
    defaults = ['--launch-game', '--multiplayer']
    for flag, values in (('--map', ['lockout.map']), ('--local-players', ['1']),
                         ('--graphics-mode', ['pStencil']), ('--viewport', ['1280', '720'])):
        if flag not in arguments:
            defaults.extend([flag, *values])
    return [*defaults, *arguments]


def main():
    # The shared launcher authenticates runtime/shaders and resolves the
    # prepared MCC cache. Never fall back to the retained October XBE runtime.
    launcher = ROOT / 'Halo2-Launcher'
    os.chdir(ROOT)
    os.execv(str(launcher), [str(launcher), *launcher_arguments(sys.argv[1:])])


if __name__ == '__main__':
    main()
