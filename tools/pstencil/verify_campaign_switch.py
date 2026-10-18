#!/usr/bin/env python3
"""Exercise the shipped campaign selector through X11 keyboard/mouse events.

Run under a private Xvfb display. This operates the actual launcher and F1 UI;
it neither changes renderer settings directly nor advances simulation clocks.
"""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import subprocess
import time

from PIL import ImageGrab


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--tick', type=int, default=1400)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(exist_ok=False, parents=True)
    root = Path(__file__).resolve().parents[2]
    trace = out / 'frames.jsonl'
    env = os.environ.copy()
    env.update(HALO2_PSTENCIL_CAMPAIGN_TRACE=str(trace),
               VK_LAYER_PATH=str(root / '.tools/validation/usr/share/vulkan/explicit_layer.d'),
               LD_LIBRARY_PATH=str(root / '.tools/validation/usr/lib'),
               SDL_VIDEODRIVER='x11')
    command = [str(root / 'Halo2-pStencil-Campaign'), '--validation']
    (out / 'command.json').write_text(json.dumps(command, indent=2) + '\n')
    x = C.CDLL('libX11.so.6')
    xt = C.CDLL('libXtst.so.6')
    x.XOpenDisplay.argtypes = [C.c_char_p]
    x.XOpenDisplay.restype = C.c_void_p
    x.XKeysymToKeycode.argtypes = [C.c_void_p, C.c_ulong]
    x.XKeysymToKeycode.restype = C.c_uint
    x.XFlush.argtypes = [C.c_void_p]
    x.XCloseDisplay.argtypes = [C.c_void_p]
    xt.XTestFakeKeyEvent.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_ulong]
    xt.XTestFakeButtonEvent.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_ulong]
    xt.XTestFakeMotionEvent.argtypes = [C.c_void_p, C.c_int, C.c_int, C.c_int, C.c_ulong]
    display = x.XOpenDisplay(None)
    if not display:
        raise RuntimeError('A private X11 display is required')
    transitions = []
    start = time.monotonic()
    with (out / 'native.log').open('w') as log:
        game = subprocess.Popen(command, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT)

        def latest():
            if trace.exists():
                lines = trace.read_text().splitlines()
                for line in reversed(lines):
                    try:
                        return json.loads(line)
                    except json.JSONDecodeError:
                        pass
            return None

        def wait_for(predicate, seconds=20):
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                if game.poll() is not None:
                    raise RuntimeError(f'Campaign exited before UI verification: {game.returncode}')
                row = latest()
                if row and predicate(row):
                    return row
                time.sleep(.08)
            raise RuntimeError('Timed out waiting for actual campaign frame state')

        def frames(count=4):
            before = latest()['frame']
            return wait_for(lambda row: row['frame'] >= before + count)

        def photo(name):
            ImageGrab.grab(xdisplay=os.environ['DISPLAY']).save(out / f'{name}.png')

        def key(keysym):
            code = x.XKeysymToKeycode(display, keysym)
            xt.XTestFakeKeyEvent(display, code, 1, 0)
            x.XFlush(display)
            frames(2)
            xt.XTestFakeKeyEvent(display, code, 0, 0)
            x.XFlush(display)
            frames()

        def click(px, py):
            xt.XTestFakeMotionEvent(display, -1, px, py, 0)
            x.XFlush(display)
            frames(2)
            xt.XTestFakeButtonEvent(display, 1, 1, 0)
            x.XFlush(display)
            frames(2)
            xt.XTestFakeButtonEvent(display, 1, 0, 0)
            x.XFlush(display)
            frames()

        try:
            wait_for(lambda row: row['tick'] >= args.tick and row['pstencil'], 180)
            photo('pStencil-before')
            for index, enabled in enumerate((False, True, False, True)):
                key(0xffbe)  # F1, handled by the actual campaign event loop.
                photo(f'{index}-settings-before')
                click(180, 90)  # Default ImGui Graphics Mode combo.
                photo(f'{index}-combo')
                # The two real menu rows, using the default ImGui font/layout.
                click(60, 109 if not enabled else 126)
                row = wait_for(lambda row: row['pstencil'] == enabled)
                transitions.append(dict(requested='pStencil' if enabled else 'Original', **row))
                photo(f'{index}-selected')
                key(0xffbe)
                photo(f'{index}-scene')
            frames(10)
            # Stop this finite interaction probe; full playback is separate.
            game.terminate()
            game.wait(timeout=20)
            result = dict(status='passed', transitions=transitions,
                          seconds=time.monotonic() - start,
                          audio_driver_forced=False,
                          scope='Real campaign F1 and mouse selection; no pixel parity or full recording claim')
        except BaseException as error:
            photo('failure')
            result = dict(status='failed', error=str(error), transitions=transitions,
                          seconds=time.monotonic() - start)
            raise
        finally:
            if game.poll() is None:
                game.terminate()
                game.wait(timeout=20)
            x.XCloseDisplay(display)
            (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
