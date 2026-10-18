#!/usr/bin/env python3
"""Deliver a normal keyboard-controller hold to one private reference session."""
import argparse
import datetime
import json
from pathlib import Path
import subprocess
import time

from audit_rendering import file_sha
from capture_xbox_tu15_grenade_reference import qmp, x11_window


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--key', required=True, choices=(
        'Return', 'BackSpace', 'Up', 'Down', 'Left', 'Right',
        'a', 'b', 'x', 'y', 'w', 's', 'd', 'f', 'i', 'j', 'k', 'l',
        'e', 'q', '1', '2', '3', '4'))
    parser.add_argument('--hold', type=float, default=.15)
    parser.add_argument('--settle', type=float, default=.5)
    args = parser.parse_args()
    assert 0 < args.hold <= 5 and 0 <= args.settle <= 5
    assert not args.output.exists()
    args.output.mkdir(parents=True)
    launch_path = args.session / 'launch.json'
    launch = json.loads(launch_path.read_text())
    command = launch['command']
    address = command[command.index('-qmp') + 1]
    assert address.startswith('unix:')
    socket = Path(address[5:].split(',')[0])
    # The display was created by this owned launch. Never send keys to :0.
    assert launch['display'] != ':0' and launch['xvfb_pid'] > 0
    window = x11_window(launch['display'])
    before = qmp(socket, [{'execute': 'query-status'}, {'execute': 'cont'}])
    try:
        x11_window(launch['display'], press=True, key=args.key, duration=args.hold)
        time.sleep(args.settle)
    finally:
        after = qmp(socket, [{'execute': 'stop'}, {'execute': 'query-status'}])
    assert after[-1]['return']['status'] == 'paused'
    subprocess.run(['import', '-display', launch['display'], '-window', str(window),
                    str(args.output / 'original.png')], check=True, timeout=10)
    result = dict(timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        provenance='Normal X11 keyboard event to the configured private Xbox gamepad',
        key=args.key, hold_seconds=args.hold, settle_seconds=args.settle,
        display=launch['display'], window=window, before=before, after=after,
        launch_sha256=file_sha(launch_path), screenshot_sha256=file_sha(args.output / 'original.png'),
        guest_code_patches=[], guest_memory_patches=[], guest_register_writes=[],
        physical_xbox_verified=False, full_weapon_parity=False,
        tool_sha256=file_sha(Path(__file__)))
    (args.output / 'input.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(key=args.key, screenshot=str(args.output / 'original.png'))))


if __name__ == '__main__':
    main()
