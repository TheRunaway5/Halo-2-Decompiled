#!/usr/bin/env python3
"""Explicit live EOS relay check; all generated device state belongs to CTest scratch."""
import json
import os
from pathlib import Path
import secrets
import subprocess
import sys
import time


def main():
    if os.environ.get('H2_EOS_LIVE_TEST') != '1':
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    signals = scratch / 'signals'
    signals.mkdir()
    common = os.environ | dict(H2_EOS_DIAGNOSTICS='1', H2_EOS_TEST_SIGNALS=str(signals),
        H2_EOS_TEST_SOCKET=secrets.token_hex(16), H2_EOS_TEST_KEY=secrets.token_hex(32))
    processes, logs = [], []
    started = time.monotonic()
    try:
        for slot in (0, 4):
            root = scratch / str(slot)
            root.mkdir()
            env = common | dict(H2_EOS_TEST_SLOT=str(slot), H2_EOS_CACHE_DIR=str(root/'cache'),
                XDG_CONFIG_HOME=str(root/'config'), XDG_DATA_HOME=str(root/'data'), XDG_CACHE_HOME=str(root/'xdg-cache'))
            output = open(root/'process.log', 'w+')
            logs.append(output)
            processes.append(subprocess.Popen([sys.argv[1]], env=env, stdout=output, stderr=subprocess.STDOUT))
        deadline = time.monotonic() + 70
        while any(p.poll() is None for p in processes):
            if any(p.poll() not in (None, 0) for p in processes) or time.monotonic() >= deadline:
                break
            time.sleep(.05)
        codes = [p.poll() for p in processes]
        passed = codes == [0, 0]
        for output in logs:
            output.flush(); output.seek(0)
            # The adapter never prints credentials or product user IDs.
            print(output.read()[:65536])
        report = dict(passed=passed, scope='Two native processes using real EOS; not two consumer NATs or gameplay acceptance',
            packet_bytes=1404, force_relay='H2_EOS_TEST_DIRECT' not in common,
            sessions='H2_EOS_TEST_SESSIONS' in common, exit_codes=codes, elapsed_seconds=round(time.monotonic()-started, 3))
        if len(sys.argv)>2:
            path=Path(sys.argv[2]); path.parent.mkdir(parents=True,exist_ok=True)
            path.write_text(json.dumps(report,indent=2)+'\n')
        return 0 if passed else 1
    finally:
        for process in processes:
            if process.poll() is None: process.terminate()
        for process in processes:
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: process.kill(); process.wait()
        for output in logs: output.close()


if __name__ == '__main__':
    sys.exit(main())
