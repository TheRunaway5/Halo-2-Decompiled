#!/usr/bin/env python3
"""Query a real SDL playback provider, leaving devices paused and empty.

This does not supply Xbox settings, DirectSound returns, APU resource counts,
or playback completion to any original executable instruction.
"""
import argparse
import ctypes
import ctypes.util
import hashlib
import json
import os
from pathlib import Path


class Spec(ctypes.Structure):
    _fields_ = [('freq', ctypes.c_int), ('format', ctypes.c_ushort),
                ('channels', ctypes.c_ubyte), ('silence', ctypes.c_ubyte),
                ('samples', ctypes.c_ushort), ('padding', ctypes.c_ushort),
                ('size', ctypes.c_uint), ('callback', ctypes.c_void_p),
                ('userdata', ctypes.c_void_p)]


class Version(ctypes.Structure):
    _fields_ = [('major', ctypes.c_ubyte), ('minor', ctypes.c_ubyte),
                ('patch', ctypes.c_ubyte)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    library = ctypes.util.find_library('SDL2')
    assert library
    lib = ctypes.CDLL(library)
    signatures = {
        'SDL_InitSubSystem': ([ctypes.c_uint], ctypes.c_int),
        'SDL_QuitSubSystem': ([ctypes.c_uint], None),
        'SDL_GetVersion': ([ctypes.POINTER(Version)], None),
        'SDL_GetError': ([], ctypes.c_char_p),
        'SDL_GetCurrentAudioDriver': ([], ctypes.c_char_p),
        'SDL_GetNumAudioDevices': ([ctypes.c_int], ctypes.c_int),
        'SDL_GetAudioDeviceName': ([ctypes.c_int, ctypes.c_int], ctypes.c_char_p),
        'SDL_GetAudioDeviceSpec': ([ctypes.c_int, ctypes.c_int, ctypes.POINTER(Spec)], ctypes.c_int),
        'SDL_GetDefaultAudioInfo': ([ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(Spec), ctypes.c_int], ctypes.c_int),
        'SDL_OpenAudioDevice': ([ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(Spec), ctypes.POINTER(Spec), ctypes.c_int], ctypes.c_uint),
        'SDL_GetAudioDeviceStatus': ([ctypes.c_uint], ctypes.c_int),
        'SDL_GetQueuedAudioSize': ([ctypes.c_uint], ctypes.c_uint),
        'SDL_CloseAudioDevice': ([ctypes.c_uint], None),
        'SDL_free': ([ctypes.c_void_p], None),
    }
    for name, (argv, result) in signatures.items():
        if hasattr(lib, name):
            function = getattr(lib, name)
            function.argtypes = argv
            function.restype = result
    error = lambda: lib.SDL_GetError().decode()
    preferred = lambda s: {name: getattr(s, name) for name in ('freq', 'format', 'channels')}
    version = Version()
    lib.SDL_GetVersion(ctypes.byref(version))
    report = {
        'tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'library': library,
        'version': [version.major, version.minor, version.patch],
        'environment_audio_driver': os.environ.get('SDL_AUDIODRIVER'),
        'provider_forced': False,
        'original_settings_caps_constructor_or_playback_result_supplied': False,
        'Xbox_settings_or_APU_resource_counts_obtained': False,
        'physical_DAC_format_latency_or_consumption_proven': False,
        'audible_playback_tested': False,
        'native_gameplay_update_applied': False,
    }
    initialized = lib.SDL_InitSubSystem(0x10)
    report['actual_init_return'] = initialized
    try:
        if initialized:
            report['actual_error'] = error()
        else:
            driver = lib.SDL_GetCurrentAudioDriver()
            report['actual_driver'] = None if driver is None else driver.decode()
            if driver in (b'dummy', b'disk'):
                report['unsupported_test_only_provider'] = True
            else:
                count = lib.SDL_GetNumAudioDevices(0)
                report['actual_playback_device_count'] = count
                report['enumerated_devices'] = []
                for index in range(max(0, count)):
                    name = lib.SDL_GetAudioDeviceName(index, 0)
                    row = {'index': index, 'actual_name': None if name is None else name.decode()}
                    if hasattr(lib, 'SDL_GetAudioDeviceSpec'):
                        spec = Spec()
                        result = lib.SDL_GetAudioDeviceSpec(index, 0, ctypes.byref(spec))
                        row['preferred_spec_return'] = result
                        if result == 0:
                            row['provider_preferred_spec'] = preferred(spec)
                        else:
                            row['actual_error'] = error()
                    report['enumerated_devices'].append(row)
                if hasattr(lib, 'SDL_GetDefaultAudioInfo'):
                    name = ctypes.c_void_p()
                    spec = Spec()
                    result = lib.SDL_GetDefaultAudioInfo(ctypes.byref(name), ctypes.byref(spec), 0)
                    row = {'actual_return': result}
                    try:
                        if result == 0:
                            row['actual_name'] = None if name.value is None else ctypes.string_at(name).decode()
                            row['provider_preferred_spec'] = preferred(spec)
                        else:
                            row['actual_error'] = error()
                    finally:
                        if name.value:
                            lib.SDL_free(name)
                    report['actual_default_info'] = row
                report['paused_open_cases'] = []
                for changes in (0, 0xF):
                    want = Spec()
                    want.freq, want.format, want.channels, want.samples = 48000, 0x8120, 2, 512
                    got = Spec()
                    device = lib.SDL_OpenAudioDevice(None, 0, ctypes.byref(want), ctypes.byref(got), changes)
                    row = {'requested': {'freq': 48000, 'format': 0x8120, 'channels': 2, 'samples': 512},
                           'allowed_changes': changes, 'null_callback_queue_mode': True,
                           'actual_device_id': device, 'queued_or_unpaused': False}
                    try:
                        if device:
                            row['obtained_application_spec'] = {name: getattr(got, name) for name in ('freq', 'format', 'channels', 'samples', 'size', 'silence')}
                            row['actual_status'] = lib.SDL_GetAudioDeviceStatus(device)
                            row['actual_queued_bytes'] = lib.SDL_GetQueuedAudioSize(device)
                            assert row['actual_status'] == 2 and row['actual_queued_bytes'] == 0
                        else:
                            row['actual_error'] = error()
                    finally:
                        if device:
                            lib.SDL_CloseAudioDevice(device)
                            row['actual_closed'] = True
                    report['paused_open_cases'].append(row)
    finally:
        if initialized == 0:
            lib.SDL_QuitSubSystem(0x10)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
