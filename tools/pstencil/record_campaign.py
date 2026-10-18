#!/usr/bin/env python3
"""Record the tested opening window and a game-only audio sink.

The default captures only the child game's window. --private-display selects
a complete private Xvfb surface. The uncut recording includes startup. A 30fps capture
stream does not establish 30 distinct rendered frames per second; the retained
native renderer timings report that separately.
"""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import signal
import subprocess
import time


def window_error_handler(x):
    """A window may disappear between XQueryTree and XGetWindowProperty."""
    class XError(C.Structure):
        _fields_ = [('type', C.c_int), ('display', C.c_void_p),
                    ('resource', C.c_ulong), ('serial', C.c_ulong),
                    ('code', C.c_ubyte), ('request', C.c_ubyte), ('minor', C.c_ubyte)]

    errors = []

    @C.CFUNCTYPE(C.c_int, C.c_void_p, C.POINTER(XError))
    def handler(display, error):
        if error.contents.code != 3:  # BadWindow is the expected enumeration race.
            errors.append((error.contents.code, error.contents.request))
        return 0

    x.XSetErrorHandler.argtypes = [C.c_void_p]
    x.XSetErrorHandler.restype = C.c_void_p
    previous = x.XSetErrorHandler(C.cast(handler, C.c_void_p))
    return handler, previous, errors


class ClientMessage(C.Structure):
    _fields_ = [('type', C.c_int), ('serial', C.c_ulong),
        ('send_event', C.c_int), ('display', C.c_void_p),
        ('window', C.c_ulong), ('message_type', C.c_ulong),
        ('format', C.c_int), ('data', C.c_long * 5)]


class Event(C.Union):
    _fields_ = [('client', ClientMessage), ('padding', C.c_long * 24)]


def wm_request(x, display, window, message, data):
    event = Event()
    event.client.type = 33  # ClientMessage
    event.client.display = display
    event.client.window = window
    event.client.message_type = x.XInternAtom(display, message, 0)
    event.client.format = 32
    for i, value in enumerate(data):
        event.client.data[i] = value
    x.XSendEvent.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_long, C.POINTER(Event)]
    if not x.XSendEvent(display, x.XDefaultRootWindow(display), 0,
                       (1 << 19) | (1 << 20), C.byref(event)):
        raise RuntimeError('Could not send the owned game window request')


def activate_game_window(window):
    """Request activation of only the PID-verified recording window."""
    x = C.CDLL('libX11.so.6')
    x.XOpenDisplay.argtypes = [C.c_char_p]
    x.XOpenDisplay.restype = C.c_void_p
    x.XDefaultRootWindow.argtypes = [C.c_void_p]
    x.XDefaultRootWindow.restype = C.c_ulong
    x.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]
    x.XInternAtom.restype = C.c_ulong
    x.XFlush.argtypes = x.XCloseDisplay.argtypes = [C.c_void_p]
    display = x.XOpenDisplay(None)
    if not display:
        raise RuntimeError('Cannot activate the game recording window')
    handler, previous, errors = window_error_handler(x)
    try:
        wm_request(x, display, window, b'_NET_ACTIVE_WINDOW', [2, 0, 0, 0, 0])
        x.XFlush(display)
        if errors:
            raise RuntimeError(f'X11 activation failed: {errors}')
    finally:
        x.XCloseDisplay(display)
        x.XSetErrorHandler(previous)


def game_window(pid, process):
    """Find only this child's X11 window; never record the user's desktop."""
    x = C.CDLL('libX11.so.6')
    x.XOpenDisplay.argtypes = [C.c_char_p]
    x.XOpenDisplay.restype = C.c_void_p
    x.XDefaultRootWindow.argtypes = [C.c_void_p]
    x.XDefaultRootWindow.restype = C.c_ulong
    x.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]
    x.XInternAtom.restype = C.c_ulong
    x.XQueryTree.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong),
                            C.POINTER(C.c_ulong), C.POINTER(C.POINTER(C.c_ulong)), C.POINTER(C.c_uint)]
    x.XGetWindowProperty.argtypes = [C.c_void_p, C.c_ulong, C.c_ulong, C.c_long,
                                    C.c_long, C.c_int, C.c_ulong, C.POINTER(C.c_ulong),
                                    C.POINTER(C.c_int), C.POINTER(C.c_ulong),
                                    C.POINTER(C.c_ulong), C.POINTER(C.POINTER(C.c_ubyte))]
    x.XFree.argtypes = [C.c_void_p]
    x.XCloseDisplay.argtypes = [C.c_void_p]
    display = x.XOpenDisplay(None)
    if not display:
        raise RuntimeError('Cannot open this X11 display')
    handler, previous, errors = window_error_handler(x)
    atom = x.XInternAtom(display, b'_NET_WM_PID', 0)

    def visit(window):
        actual, count, remaining = C.c_ulong(), C.c_ulong(), C.c_ulong()
        format_ = C.c_int()
        data = C.POINTER(C.c_ubyte)()
        status = x.XGetWindowProperty(display, window, atom, 0, 1, 0, 6,
                                     C.byref(actual), C.byref(format_), C.byref(count),
                                     C.byref(remaining), C.byref(data))
        try:
            if status == 0 and actual.value == 6 and format_.value == 32 and count.value == 1:
                if C.cast(data, C.POINTER(C.c_ulong))[0] == pid:
                    return window
        finally:
            if data:
                x.XFree(data)
        root, parent, children, length = C.c_ulong(), C.c_ulong(), C.POINTER(C.c_ulong)(), C.c_uint()
        if x.XQueryTree(display, window, C.byref(root), C.byref(parent), C.byref(children), C.byref(length)):
            try:
                for index in range(length.value):
                    found = visit(children[index])
                    if found:
                        return found
            finally:
                if children:
                    x.XFree(children)
        return None

    try:
        deadline = time.monotonic() + 180
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError('Campaign exited before creating its recording window')
            found = visit(x.XDefaultRootWindow(display))
            if errors:
                raise RuntimeError(f'X11 window enumeration failed: {errors}')
            if found:
                return found
            time.sleep(.1)
        raise RuntimeError('Cannot find the campaign child window')
    finally:
        x.XCloseDisplay(display)
        x.XSetErrorHandler(previous)


def recording_window_size(window, process, fullscreen=False):
    """Read the owned client size; optionally request its EWMH fullscreen state."""
    x = C.CDLL('libX11.so.6')
    x.XOpenDisplay.argtypes = [C.c_char_p]
    x.XOpenDisplay.restype = C.c_void_p
    x.XDefaultRootWindow.argtypes = [C.c_void_p]
    x.XDefaultRootWindow.restype = C.c_ulong
    x.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]
    x.XInternAtom.restype = C.c_ulong
    x.XGetGeometry.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong),
        C.POINTER(C.c_int), C.POINTER(C.c_int), C.POINTER(C.c_uint),
        C.POINTER(C.c_uint), C.POINTER(C.c_uint), C.POINTER(C.c_uint)]
    x.XCloseDisplay.argtypes = [C.c_void_p]
    x.XFlush.argtypes = [C.c_void_p]
    display = x.XOpenDisplay(None)
    if not display:
        raise RuntimeError('Cannot query this game window')
    handler, previous, errors = window_error_handler(x)
    try:
        if fullscreen:
            wm_request(x, display, window, b'_NET_WM_STATE',
                       [1, x.XInternAtom(display, b'_NET_WM_STATE_FULLSCREEN', 0), 0, 1, 0])
            x.XFlush(display)
        deadline = time.monotonic() + 10
        while process.poll() is None:
            root, left, top = C.c_ulong(), C.c_int(), C.c_int()
            width, height, border, depth = (C.c_uint() for _ in range(4))
            if not x.XGetGeometry(display, window, C.byref(root), C.byref(left),
                    C.byref(top), C.byref(width), C.byref(height), C.byref(border), C.byref(depth)):
                raise RuntimeError('Cannot read the owned game window dimensions')
            if errors:
                raise RuntimeError(f'X11 game window query failed: {errors}')
            if width.value and height.value:
                if not fullscreen or (width.value, height.value) == (1920, 1080):
                    return [width.value, height.value]
            if time.monotonic() >= deadline:
                raise RuntimeError(f'Game fullscreen size is {width.value}x{height.value}; expected 1920x1080')
            time.sleep(.08)
        raise RuntimeError('Campaign exited while preparing its recording window')
    finally:
        x.XCloseDisplay(display)
        x.XSetErrorHandler(previous)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    surface = parser.add_mutually_exclusive_group()
    surface.add_argument('--window', action='store_true', default=True,
                         help='Capture only the child game window (default)')
    surface.add_argument('--private-display', action='store_false', dest='window',
                         help='Capture the complete surface of a caller-provided private Xvfb display')
    parser.add_argument('--fullscreen-window', action='store_true',
                        help='Request 1920x1080 fullscreen on only the owned game window')
    parser.add_argument('--viewport', type=int, nargs=2, default=[1920, 1080],
                        metavar=('WIDTH', 'HEIGHT'), help='Requested native game window dimensions')
    parser.add_argument('--present-mode', choices=['fifo', 'immediate'], default='fifo',
                        help='Recording presentation mode; immediate must be advertised by the surface')
    args = parser.parse_args()
    if any(value < 320 or value > 16384 for value in args.viewport):
        parser.error('Viewport dimensions must be between 320 and 16384')
    if args.fullscreen_window and args.viewport != [1920, 1080]:
        parser.error('The fullscreen capture path requires 1920x1080')
    out = args.output.resolve()
    out.mkdir(exist_ok=False, parents=True)
    root = Path(__file__).resolve().parents[2]
    sink = f'pstencil_campaign_{os.getpid()}'
    module = subprocess.check_output([
        'pactl', 'load-module', 'module-null-sink', f'sink_name={sink}',
        'rate=48000', 'channels=2', 'sink_properties=device.description=pStencilRecording'
    ], text=True).strip()
    recorder = game = native_log = None
    result = dict(status='failed', full_presentation_verified=False)
    start = time.monotonic()
    try:
        env = os.environ.copy()
        env.update(SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='pulseaudio', PULSE_SINK=sink,
                   SDL_AUDIO_DEVICE_APP_NAME='pStencil Opening Recording',
                   SDL_AUDIO_DEVICE_STREAM_NAME='pStencil game audio',
                   HALO2_RENDER_TIMINGS=str(out / 'renderer-timings.tsv'),
                   HALO2_PSTENCIL_CAMPAIGN_TRACE=str(out / 'frames.jsonl'),
                   VK_LAYER_PATH=str(root / '.tools/validation/usr/share/vulkan/explicit_layer.d'),
                   LD_LIBRARY_PATH=str(root / '.tools/validation/usr/lib'))
        env['HALO2_PSTENCIL_CAPTURE_PRESENT_MODE'] = args.present_mode
        result['capture_present_mode'] = args.present_mode
        native = ['stdbuf', '-oL', '-eL', str(root / 'Halo2-pStencil-Campaign'), '--validation',
                  '--viewport', str(args.viewport[0]), str(args.viewport[1])]
        (out / 'native-command.json').write_text(json.dumps(native, indent=2) + '\n')
        native_log = (out / 'native.log').open('w')
        viewport = args.viewport
        if args.window:
            game = subprocess.Popen(native, cwd=root, env=env, stdout=native_log, stderr=subprocess.STDOUT)
            window = game_window(game.pid, game)
            result['x11_game_window'] = window
            viewport = recording_window_size(window, game, args.fullscreen_window)
            if args.fullscreen_window:
                activate_game_window(window)
            result['recording_window_size'] = viewport
        command = ['ffmpeg', '-hide_banner', '-nostdin', '-y',
                   '-thread_queue_size', '512', '-f', 'x11grab', '-draw_mouse', '0',
                   '-framerate', '30', '-video_size', f'{viewport[0]}x{viewport[1]}',
                   *(['-window_id', str(window)] if args.window else []),
                   '-i', os.environ['DISPLAY'],
                   '-thread_queue_size', '512', '-f', 'pulse', '-i', sink + '.monitor',
                   '-map', '0:v:0', '-map', '1:a:0', '-c:v', 'libx264',
                   '-preset', 'veryfast', '-crf', '16', '-pix_fmt', 'yuv444p',
                   '-threads', '2', '-c:a', 'flac', str(out / 'opening-uncut.mkv')]
        (out / 'recorder-command.json').write_text(json.dumps(command, indent=2) + '\n')
        with (out / 'recorder.log').open('w') as log:
            recorder = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 15
            while 'Output #0' not in (out / 'recorder.log').read_text():
                if recorder.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError('The isolated video/audio recorder did not start')
                time.sleep(.1)
            if game is None:
                game = subprocess.Popen(native, cwd=root, env=env, stdout=native_log, stderr=subprocess.STDOUT)
            # SDL may explicitly select the server's default device, which
            # takes precedence over PULSE_SINK. Route only this child's actual
            # streams to the isolated recording sink; retain gain/mute values.
            routing = []
            seen_routes = set()
            deadline = time.monotonic() + 480
            while game.poll() is None:
                if time.monotonic() > deadline:
                    raise RuntimeError('Opening playback exceeded the recording deadline')
                streams = json.loads(subprocess.check_output(
                    ['pactl', '-f', 'json', 'list', 'sink-inputs'], text=True))
                sinks = json.loads(subprocess.check_output(
                    ['pactl', '-f', 'json', 'list', 'sinks'], text=True))
                target = next(s['index'] for s in sinks if s['name'] == sink)
                for stream in streams:
                    if stream.get('properties', {}).get('application.process.id') != str(game.pid):
                        continue
                    identity = (stream['index'], stream['sink'])
                    if identity not in seen_routes:
                        routing.append(dict(seconds=time.monotonic() - start, stream=stream,
                                            target_sink=target))
                        seen_routes.add(identity)
                        (out / 'audio-routing.json').write_text(json.dumps(routing, indent=2) + '\n')
                    if stream['sink'] != target:
                        subprocess.run(['pactl', 'move-sink-input', str(stream['index']), sink], check=True)
                time.sleep(.25)
            status = game.returncode
            native_log.close()
            time.sleep(.5)  # Retain the device's final queued audio after normal exit.
            recorder.send_signal(signal.SIGINT)
            recorder_status = recorder.wait(timeout=30)
        if status or recorder_status not in (0, 255):
            raise RuntimeError(f'Native status={status}; recorder status={recorder_status}')
        result.update(status='captured', native_status=status, recorder_status=recorder_status,
                      seconds=time.monotonic() - start, viewport=viewport,
                      audio='PipeWire PulseAudio game-only null-sink monitor, stereo48000Hz FLAC',
                      video='Uncut X11 capture, 30fps stream, H264 CRF16 4:4:4',
                      scope='Uninterrupted diagnostic capture; material, pacing and audible parity require inspection')
    except BaseException as error:
        result['error'] = str(error)
        raise
    finally:
        if game is not None and game.poll() is None:
            game.terminate()
            game.wait(timeout=20)
        if recorder is not None and recorder.poll() is None:
            recorder.send_signal(signal.SIGINT)
            recorder.wait(timeout=30)
        if native_log is not None:
            native_log.close()
        subprocess.run(['pactl', 'unload-module', module], check=True)
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
