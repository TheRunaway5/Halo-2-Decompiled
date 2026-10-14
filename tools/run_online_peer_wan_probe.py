#!/usr/bin/env python3
"""Native-to-Vultr UDP check; SSH carries authenticated setup and receipts only.

Prerequisite: isolated aiortc1.14.0 venv and echo endpoint under /opt/halo2/tests
on the user's configured halo2-master SSH host. No live accounts are modified.
"""
import asyncio
import json
from pathlib import Path
import sys


async def main():
    native = await asyncio.create_subprocess_exec(str(Path(sys.argv[1]).resolve()),
        stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
    remote = await asyncio.create_subprocess_exec('ssh', '-o', 'BatchMode=yes', 'halo2-master',
        '/opt/halo2/tests/direct-peer-venv/bin/python /opt/halo2/tests/online_peer_wan_echo.py',
        stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
    receipts = {}
    async def forward(source, target, name):
        while line := await source.stdout.readline():
            value = json.loads(line)
            if 'signal' in value:
                assert set(value) == {'signal'} and set(value['signal']) == {'peer', 'type', 'payload', 'mid'}
                assert value['signal']['type'] in ('offer', 'answer', 'candidate')
                target.stdin.write(line)
                await target.stdin.drain()
            else:
                assert value.get('done') is True
                receipts[name] = value
                if name == 'native':
                    remote.stdin.write(b'{"stop":true}\n')
                    await remote.stdin.drain()
    tasks = [asyncio.create_task(forward(native, remote, 'native')),
             asyncio.create_task(forward(remote, native, 'remote'))]
    try:
        async with asyncio.timeout(55):
            await asyncio.gather(*tasks)
            native_code, remote_code = await asyncio.gather(native.wait(), remote.wait())
            native_error, remote_error = await asyncio.gather(native.stderr.read(), remote.stderr.read())
            assert native_code == remote_code == 0, (native_code, remote_code, native_error.decode(), remote_error.decode())
            assert receipts['native']['received'] >= 45 and receipts['remote']['received'] >= 45
            print(json.dumps(dict(result='PASS direct encrypted UDP over WAN; SSH setup only; no TURN', **receipts), indent=2))
    finally:
        for task in tasks:
            task.cancel()
        for process in (native, remote):
            if process.returncode is None:
                process.kill()
                await process.wait()


if __name__ == '__main__':
    asyncio.run(main())
