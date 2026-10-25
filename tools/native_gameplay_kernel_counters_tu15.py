"""Read named diagnostic atomics from this worker's own live native game child."""
import os
from pathlib import Path
import re
import struct
import subprocess


class NativeGameplayKernelCounters:
    NAMES = {
        'difficulty_parameter': 'halo2::difficulty_parameter_native_detail::parameter_calls',
        'current_difficulty': 'halo2::difficulty_parameter_native_detail::current_difficulty_calls',
        'material_lookup': 'halo2::physics_world_runtime_detail::damage_material_native_calls',
        'model_lookup': 'halo2::physics_world_runtime_detail::damage_model_native_calls',
    }

    def __init__(self, runtime, *, vehicle_choices=False, flying_selector=False):
        self.runtime = Path(runtime).resolve(strict=True)
        names=dict(self.NAMES)
        if vehicle_choices:
            names.update(vehicle_selector='halo2::physics_world_runtime_detail::vehicle_choice_native_calls',
                vehicle_random_draws='halo2::physics_world_runtime_detail::vehicle_choice_native_random_draws')
        if flying_selector:
            names['flying_vehicle_selector']='halo2::physics_world_runtime_detail::vehicle_choice_native_flying_calls'
        with self.runtime.open('rb') as stream:
            elf = stream.read(64)
        if elf[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', elf, 18)[0] != 62:
            raise ValueError('Read-only native counters require the qualified Linux x86-64 ELF')
        self.elf_type = struct.unpack_from('<H', elf, 16)[0]
        if self.elf_type not in (2, 3): raise ValueError('Unsupported native ELF load type')
        if self.elf_type == 3:
            table_at = struct.unpack_from('<Q', elf, 32)[0]
            stride, count = struct.unpack_from('<HH', elf, 54)
            if stride != 56 or count > 128: raise ValueError('Unexpected native ELF program headers')
            with self.runtime.open('rb') as stream:
                stream.seek(table_at); headers = stream.read(stride * count)
            loads = [struct.unpack_from('<IIQQQQQQ', headers, i * stride) for i in range(count)]
            starts = [row for row in loads if row[0] == 1 and row[2] == 0]
            if len(starts) != 1 or starts[0][3] != 0:
                raise ValueError('Native PIE does not have the qualified zero-address first LOAD segment')
        output = subprocess.check_output(['nm', '-C', '--defined-only', str(self.runtime)], text=True)
        found = {}
        for line in output.splitlines():
            match = re.fullmatch(r'([0-9a-fA-F]+)\s+[bBdDuU]\s+(.+)', line)
            if match and match[2] in names.values():
                if match[2] in found: raise ValueError('Duplicate native diagnostic symbol')
                found[match[2]] = int(match[1], 16)
        if set(found) != set(names.values()): raise ValueError('Delivered native diagnostic symbols are incomplete')
        self.offsets = {key: found[name] for key, name in names.items()}
        if any(value % 8 for value in self.offsets.values()): raise ValueError('Native diagnostic word is unaligned')
        self.process = None

    def bind_process(self, process):
        if process.poll() is not None: raise ValueError('Native game exited before diagnostic binding')
        self.process = process

    def read(self):
        process = self.process
        if process is None or process.poll() is not None: raise ValueError('Read only the owned live native game child')
        proc = Path('/proc') / str(process.pid)
        if (proc / 'exe').resolve(strict=True) != self.runtime:
            raise ValueError('Owned child no longer executes the declared native runtime')
        mappings = []
        for line in (proc / 'maps').read_text().splitlines():
            fields = line.split(maxsplit=5)
            start, end = (int(value, 16) for value in fields[0].split('-'))
            mappings.append((start, end, fields[1], int(fields[2], 16), fields[5] if len(fields) == 6 else ''))
        starts = [start for start, _, _, offset, name in mappings if offset == 0 and name == str(self.runtime)]
        if len(starts) != 1: raise ValueError('Native executable load bias is ambiguous')
        # PIE links use a zero-address first LOAD segment. Ordinary ET_EXEC
        # symbols already carry their complete virtual addresses.
        bias = starts[0] if self.elf_type == 3 else 0
        values = {}
        with (proc / 'mem').open('rb', buffering=0) as memory:
            for name, offset in self.offsets.items():
                address = bias + offset
                if not any(start <= address and address + 8 <= end and 'r' in permissions
                    for start, end, permissions, _, _ in mappings):
                    raise ValueError('Native diagnostic word has no readable owned mapping')
                raw = os.pread(memory.fileno(), 8, address)
                if len(raw) != 8: raise ValueError('Native diagnostic read was incomplete')
                values[name] = int.from_bytes(raw, 'little')
        return values
