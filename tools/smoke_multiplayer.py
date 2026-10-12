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
        clients = []
        for name, count in [('lan-one', 1), ('lan-split', 2)]:
            clients.append(start(name, ['--join', '127.0.0.1:' + str(port),
                                        '--local-players', str(count), '--hidden',
                                        '--frames', '36', '--mp-replay',
                                        '--mp-capture-players', '3', '--validation', '--screenshot',
                                        str((args.output / (name + '.ppm')).resolve())]))
        # Cold driver/asset preparation can take longer than this finite host's
        # entire run. Start its gameplay window when the first renderer is ready;
        # clients' unanswered-host timer begins at their first actual request.
        deadline = time.monotonic() + 180
        while not any('Multiplayer GPU:' in (args.output / (name + '.log')).read_text()
                      for name in ('lan-one','lan-split')):
            if any(client.poll() is not None for client in clients) or time.monotonic() >= deadline:
                raise RuntimeError('LAN renderer did not become ready; inspect client logs')
            time.sleep(.05)
        host = start('lan-host', ['--dedicated', '--host', str(port), '--simulate', '600'])
        deadline = time.monotonic() + 5
        while 'LAN host UDP port=' not in (args.output / 'lan-host.log').read_text():
            if host.poll() is not None or time.monotonic() >= deadline:
                raise RuntimeError('Dedicated host did not become ready; inspect lan-host.log')
            time.sleep(.05)
        for client in clients:
            if client.wait(timeout=20):
                raise RuntimeError('Rendered LAN client failed; inspect its log')
        if host.wait(timeout=25):
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
