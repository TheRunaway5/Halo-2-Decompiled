#!/usr/bin/env python3
"""Identify the real worker I/O request reached by an original weapon replay.

Decode ABI arguments from authenticated reconstructed state, then compare the
original full-call exit buffer with owned map bytes. Matching content does not
establish a native handle binding, I/O completion timeline, or parity pass.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import urllib.request
from original_xbox_pose_case import BASE, ROOT, cpu, physical_ram, PageTableMemory, reconstructed_replay_ram, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('case', 'seek-oracle', 'read-oracle', 'output'):
        parser.add_argument('--' + key, type=Path, required=True)
    args = parser.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    output = args.output.absolute()
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=os.environ['HALO2_TEST_SCRATCH'],
        tool_sha256=sha(Path(__file__)), full_weapon_parity=False), indent=2) + '\n')
    url = 'https://raw.githubusercontent.com/XboxDev/nxdk/9e9c07f4b66b540b342e7ac000f02a4a82d7c6e5/lib/xboxkrnl/xboxkrnl.h'
    header = urllib.request.urlopen(url, timeout=30).read()
    (output / 'nxdk-xboxkrnl.h').write_bytes(header)
    text = header.decode()
    signatures = {}
    for name in ('NtSetInformationFile', 'NtReadFile'):
        signatures[name] = re.search(r'XBAPI NTSTATUS NTAPI ' + name + r'\s*\([^;]+\);', text).group(0)
    assert ['FileHandle', 'Event', 'ApcRoutine', 'ApcContext', 'IoStatusBlock', 'Buffer', 'Length', 'ByteOffset'] == [
        row.strip().rstrip(',').split(' OPTIONAL')[0].split()[-1]
        for row in signatures['NtReadFile'].splitlines()[2:-1]]
    with reconstructed_replay_ram(args.case, args.seek_oracle) as (bank, memory, seek_report):
        assert seek_report['exit'] == 0x80016f61
        sp = seek_report['actual_CPU']['gp'][4]
        seek_stack = list(struct.unpack('<6I', memory.read(sp, 24)))
        _, handle, iosb, information, length, information_class = seek_stack
        assert length == 8 and information_class == 14
        offset = struct.unpack('<q', memory.read(information, 8))[0]
        assert offset >= 0
    with reconstructed_replay_ram(args.case, args.read_oracle) as (bank, memory, read_report):
        assert read_report['exit'] == 0x80017467
        sp = read_report['actual_CPU']['gp'][4]
        read_stack = list(struct.unpack('<9I', memory.read(sp, 36)))
        _, read_handle, event, apc, context, read_iosb, buffer, size, byte_offset = read_stack
        assert read_handle == handle and not byte_offset and 0 < size <= 4 * 1024 * 1024
        before = memory.read(buffer, size)
        # These are literal observed candidates, not a guessed queue identity.
        # Keep all matches and do not claim the originating node from content.
        pattern = struct.pack('<4I', handle, buffer, size, offset)
        task_candidates, start = [], 0
        while True:
            position = bank.find(pattern, start)
            if position < 0:
                break
            assert len(task_candidates) < 64
            words = list(struct.unpack('<10I', bank[position:position + 40]))
            task_candidates.append(dict(physical_address=position, observed_words=words,
                                   originating_queue_node_identity_verified=False))
            start = position + 1
    with physical_ram(args.case, 'after') as bank:
        memory = PageTableMemory(bank, cpu(args.case, 'after'))
        original_exit = memory.read(buffer, size)
        iosb_raw = memory.read(read_iosb, 8)
    scope = json.loads((BASE / 'authored-scope.json').read_text())
    candidates = {Path(row['path']): row['sha256'] for row in scope['maps']}
    for name in ('mainmenu', 'shared', 'single_player_shared'):
        path = ROOT / 'assets/disc/maps' / (name + '.map')
        if path.exists():
            candidates.setdefault(path, None)
    checked, matches = [], []
    for path, pin in sorted(candidates.items()):
        if path.stat().st_size < offset + size:
            checked.append(dict(path=str(path), range_available=False))
            continue
        with path.open('rb') as source:
            source.seek(offset)
            actual = source.read(size)
        equal = actual == original_exit
        row = dict(path=str(path), range_available=True, content_equal_original_exit_buffer=equal,
                   requested_bytes_sha256=hashlib.sha256(actual).hexdigest())
        checked.append(row)
        if equal:
            digest = sha(path)
            if pin:
                assert digest == pin
            matches.append(dict(**row, whole_file_sha256=digest, authored_scope_pin=pin))
    result = dict(passed=True, scope=__doc__, original_case_sha256=sha(args.case / 'call.json'),
        seek_oracle_sha256=sha(args.seek_oracle / 'report.json'), read_oracle_sha256=sha(args.read_oracle / 'report.json'),
        primary_ABI_source=dict(url=url, sha256=hashlib.sha256(header).hexdigest(), signatures=signatures),
        original_seek_stack=seek_stack, original_read_stack=read_stack, observed_file_handle=handle,
        seek_information_class=information_class, seek_offset=offset, buffer=buffer, bytes=size,
        original_entry_buffer_sha256=hashlib.sha256(before).hexdigest(),
        original_full_call_exit_buffer_sha256=hashlib.sha256(original_exit).hexdigest(),
        original_full_call_exit_IO_status_raw=iosb_raw.hex(), buffer_changed_bytes=sum(a != b for a, b in zip(before, original_exit)),
        owned_map_content_matches=matches, checked_owned_map_ranges=checked,
        observed_matching_task_candidates=task_candidates,
        native_handle_file_binding_verified=False, I_O_completion_timeline_verified=False,
        original_kernel_or_gameplay_result_supplied=False, persistent_raw_RAM_or_IO_buffer_retained=False,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'], tool_sha256=sha(Path(__file__)),
        helper_sha256=sha(Path(__file__).with_name('original_xbox_pose_case.py')),
        physical_xbox_verified=False, full_weapon_parity=False)
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    for source in (Path(__file__), Path(__file__).with_name('original_xbox_pose_case.py')):
        shutil.copyfile(source, output / source.name)
    print(json.dumps({k: result[k] for k in ('observed_file_handle', 'seek_offset', 'buffer', 'bytes', 'buffer_changed_bytes', 'owned_map_content_matches', 'runner_scratch')}), flush=True)


if __name__ == '__main__':
    main()
