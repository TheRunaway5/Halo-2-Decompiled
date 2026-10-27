#!/usr/bin/env python3
"""Run a finite native LAN host and two rendered clients on loopback."""
import argparse
import os
from pathlib import Path
import re
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'build-multiplayer/halo2_native')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    # Reserve an available test port briefly. A racing bind fails explicitly;
    # this tool never stops another host or changes the user's game settings.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
        probe.bind(('127.0.0.1', 0))
        port = probe.getsockname()[1]
    base = [str(args.runtime.resolve()), '--multiplayer', '--map',
            'assets/disc/maps/lockout.map', '--mute']
    env = os.environ.copy()
    env.setdefault('DISPLAY', ':0')
    processes, logs = [], []

    def start(name, options):
        log = (args.output / (name + '.log')).open('w')
        logs.append(log)
        process = subprocess.Popen(base + options, cwd=ROOT, env=env,
                                   stdout=log, stderr=subprocess.STDOUT)
        processes.append(process)
        return process

    try:
        # The independently exchanged key must exist before clients make any
        # raw query or hello. Read it from this owned host's trusted console.
        host = start('lan-host', ['--dedicated', '--host', str(port), '--simulate', '6000'])
        deadline = time.monotonic() + 180
        host_log = args.output / 'lan-host.log'
        while 'LAN join token: ' not in host_log.read_text():
            if host.poll() is not None or time.monotonic() >= deadline:
                raise RuntimeError('Dedicated host did not become ready; inspect lan-host.log')
            time.sleep(.05)
        address = next(line.removeprefix('LAN join token: ').replace('HOST_ADDRESS', '127.0.0.1')
                       for line in host_log.read_text().splitlines() if line.startswith('LAN join token: '))
        clients = []
        for name, count in [('lan-one', 1), ('lan-split', 2)]:
            clients.append(start(name, ['--join', address,
                                        '--local-players', str(count), '--hidden',
                                        '--frames', '36', '--mp-replay',
                                        '--mp-capture-players', '3', '--validation', '--screenshot',
                                        str((args.output / (name + '.ppm')).resolve())]))
        for client in clients:
            if client.wait(timeout=180):
                raise RuntimeError('Rendered LAN client failed; inspect its log')
        if host.wait(timeout=210):
            raise RuntimeError('Dedicated host failed; inspect lan-host.log')
        seen = []
        combined = ''
        for name, count in [('lan-one', 1), ('lan-split', 2)]:
            text = (args.output / (name + '.log')).read_text()
            combined += text
            match = re.search(r'LAN joined slots=([0-9,]+)', text)
            if not match or 'connected=1' not in text:
                raise RuntimeError('LAN client never joined')
            slots = [int(i) for i in match[1].split(',') if i]
            if len(slots) != count:
                raise RuntimeError('Incorrect number of local LAN participants')
            seen.extend(slots)
        if len(set(seen)) != 3 or combined.count('LAN capture participants=3') != 2:
            raise RuntimeError('Three distinct participants were not replicated')
        if 'players=0' not in (args.output / 'lan-host.log').read_text():
            raise RuntimeError('Client leave did not release host participants')
        print('Separate dedicated host, one-player client and two-player client passed; three distinct slots replicated.')
        return 0
    finally:
        for process in processes:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        for log in logs:
            log.close()


if __name__ == '__main__':
    raise SystemExit(main())
