#!/usr/bin/env python3
"""Read-only source-owned held-input/inventory/attachment transfer measurement.

The unchanged frozen attachment main must reproduce byte-for-byte before and
during measurement. No source instruction, memory, service result or original
fixture setup is changed. Stored states are bounded original fixture inputs;
nearby placement, heap/residency, scheduling, Havok and Xbox reciprocal limits
remain explicit.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import oracle_mp_weapon_attachment_1_1 as source
from oracle_mp_inventory_1_1 import ROOT, UNIT, UI, POOL, HEADERS, STOP, TIME, SEED, _trace
from oracle_mp_inventory_controls_1_1 import RAW, ACTION, LOGICAL, PLAYER_DATA
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP

BASE = ROOT / 'analysis/multiplayer/integration/inventory-source'
OUT = BASE / 'pickup-transfer-measured'
INSTANCES = []
ENTRIES = (0x126350, 0x219d50, 0x1884e0, 0x14e7e0, 0x150dd0, 0x1ea490,
    0x150f00, 0x151c90, 0xc8140, 0x1533e0, 0x153090, 0xbbd10, 0xceaf0,
    0xcec00, 0x111490, 0x1cfde0, 0xe7f30, 0xe7ef0, 0xe7ff0, 0xea660,
    0xce510, 0xcea70, 0xcfed0, 0xcfe60, 0xfb120, 0xfb360, 0xfb3f0,
    0xfb490, 0xfb560, 0xb96a0, 0x16d2c0, 0x16d4c0, 0xe9d50, 0xe9c50,
    0xe9b40, 0xd1d20, 0xba2a0, 0xba980, 0xbf540, 0xcf8c0, 0xcfd80)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def frozen_write(path, data):
    if path.exists():
        require(path.read_bytes() == data, f'Refusing changed evidence: {path}')
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)


class TransferMeasurement:
    def __init__(self, *args, **kwargs):
        self.transfer_calls = []; self.transfer_pending = []; self.transfer_timeline = []
        self.transfer_states = []; self.transfer_state_ids = {}; self.transfer_order = 0
        self.transfer_roots = []; self.transfer_executed = set()
        INSTANCES.append(self)
        super().__init__(*args, **kwargs)

    def order(self):
        result = self.transfer_order; self.transfer_order += 1
        return result

    def raw(self, pointer, length):
        return bytes(self.g.mem_read(pointer, length)).hex()

    def transfer_state(self):
        count = self.u(POOL + 0x38)
        require(count <= 64, 'Unexpected source pool capacity')
        unit = dict(datum=f'{UI:08x}', body_address=f'{UNIT:08x}',
            body_prefix_hex=self.raw(UNIT, 0x800),
            inventory_hex=self.raw(UNIT + 0x210, 0x30),
            controlled_unit_flags=f'{self.u(UNIT + 0x134):08x}',
            network_entity=f'{self.u(UNIT + 0xd4):08x}',
            player=f'{self.u(UNIT + 0x13c):08x}')
        for name, offset, length in (('manager', 0x12a, 0x90),
                ('extension', 0x33e, 0x124), ('extra', 0x342, 0x58), ('control', 0x346, 0x44)):
            at = self.u(UNIT + offset, 2)
            unit[name] = dict(offset=at, raw_hex=self.raw(UNIT + at, length)) if at not in (0, 0xffff) else None
        length, offset = self.u(UNIT + 0x114, 2), self.u(UNIT + 0x116, 2)
        unit['current_nodes'] = dict(offset=offset, raw_hex=self.raw(UNIT + offset, length)) if 0 < length <= 0x2000 else None
        items = []
        for n in range(2, count):
            header = HEADERS + n * 12; body = self.u(header + 8)
            if not body or self.u(header + 3, 1) != 2:
                continue
            item = dict(datum=f'{self.u(header, 2) << 16 | n:08x}', body_address=f'{body:08x}',
                header_hex=self.raw(header, 12), body_prefix_hex=self.raw(body, 0x340),
                definition=f'{self.u(body):08x}', owner=f'{self.u(body + 0x154):08x}',
                parent=f'{self.u(body + 0x14):08x}', sibling=f'{self.u(body + 0xc):08x}',
                parent_node=self.u(body + 0x18, 1), originating_row=self.u(body + 0xaf, 1),
                loaded=self.u(body + 0x22c, 2), reserve=self.u(body + 0x22a, 2),
                heat_bits=f'{self.u(body + 0x184):08x}')
            length, offset = self.u(body + 0x114, 2), self.u(body + 0x116, 2)
            item['current_nodes'] = dict(offset=offset, raw_hex=self.raw(body + offset, length)) if 0 < length <= 0x2000 else None
            items.append(item)
        value = dict(unit=unit, items=items, object_pool_hex=self.raw(POOL, 0x50),
            headers_hex=self.raw(HEADERS, count * 12), bitmap_hex=self.raw(self.u(POOL + 0x48), 8),
            tick=self.u(TIME + 8), shared_rng=f'{self.u(SEED):08x}')
        if hasattr(self, 'entries') and hasattr(self, 'row'):
            value['netgame_row'] = dict(index=self.row, entry_hex=self.raw(self.entries + self.row * 8, 8))
        if getattr(self, 'controls_ready', False):
            value['input'] = dict(raw_device_hex=self.raw(RAW, 0x48), logical_hex=self.raw(LOGICAL, 0x50),
                generated_action_hex=self.raw(ACTION, 0x5c), player_prefix_hex=self.raw(PLAYER_DATA, 0x40))
        key = hashlib.sha256(encoded(value)).hexdigest()
        if key not in self.transfer_state_ids:
            self.transfer_state_ids[key] = len(self.transfer_states)
            self.transfer_states.append(dict(id=len(self.transfer_states), sha256=key, **value))
        return self.transfer_state_ids[key]

    def finish_transfer(self, address):
        for pending in self.transfer_pending[:]:
            if pending['return_address'] != address:
                continue
            row = pending['row']; row['return_eax'] = f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
            row['after_state'] = self.transfer_state(); row['return_order'] = self.order()
            if 'output' in pending:
                at, length = pending['output']; row['output_hex'] = self.raw(at, length)
            if pending.get('registry'):
                row['registry_after'] = self.registry()
            self.transfer_timeline.append(dict(order=row['return_order'], kind='return', call_id=row['id'], state=row['after_state']))
            self.transfer_pending.remove(pending)

    def run(self, address, argv=(), regs=()):
        root = None
        if hasattr(self, 'source_calls'):
            root = dict(order=self.order(), phase=self.active, entry=f'{address:08x}',
                arguments=[f'{x & 0xffffffff:08x}' for x in argv],
                registers=[dict(register=r, value=f'{v & 0xffffffff:08x}') for r, v in regs], before_state=self.transfer_state())
            self.transfer_roots.append(root)
        super().run(address, argv, regs)
        self.finish_transfer(STOP)
        if root is not None:
            root.update(return_order=self.order(), return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}', after_state=self.transfer_state())

    def fragment(self, name, start, stop, registers=()):
        root = None
        if hasattr(self, 'source_calls'):
            root = dict(order=self.order(), phase=name, fragment=[f'{start:08x}', f'{stop:08x}'],
                exclusive_stop=True, registers=[dict(register=r, value=f'{v & 0xffffffff:08x}') for r, v in registers],
                before_state=self.transfer_state()); self.transfer_roots.append(root)
        result = super().fragment(name, start, stop, registers)
        if root is not None:
            self.finish_transfer(stop); root.update(return_order=self.order(), after_state=self.transfer_state())
        return result

    def hook(self, g, address, size, user):
        if hasattr(self, 'source_calls'):
            self.transfer_executed.add(address); self.finish_transfer(address)
            if address in ENTRIES:
                sp = g.reg_read(UC_X86_REG_ESP)
                row = dict(id=len(self.transfer_calls), order=self.order(), phase=self.active,
                    entry=f'{address:08x}', caller=f'{self.u(sp):08x}', registers=self.registers(),
                    stack=[f'{self.u(sp + 4 + n * 4):08x}' for n in range(7)], before_state=self.transfer_state())
                pending = dict(row=row, return_address=self.u(sp))
                if address == 0x153090:
                    row['candidate_hex'] = self.raw(g.reg_read(UC_X86_REG_ECX), 12)
                    row['primary_button_selector'] = self.u(sp + 4, 1)
                elif address in (0xe7f30, 0xea660):
                    pointer = g.reg_read(UC_X86_REG_EDI) if address == 0xe7f30 else self.u(sp + 8)
                    row['request_hex'] = self.raw(pointer, 32)
                elif address == 0xe7ff0:
                    row['cancel_action_type'] = g.reg_read(UC_X86_REG_EDI)
                elif address == 0xe7ef0:
                    row['action_type'] = g.reg_read(UC_X86_REG_ECX)
                elif address == 0xcec00:
                    pending['output'] = (self.u(sp + 12), 4)
                elif address in (0x150dd0, 0x14e7e0):
                    pointer = g.reg_read(UC_X86_REG_ECX)
                    row['control_input_hex'] = self.raw(pointer, 0x5c); pending['output'] = (pointer, 0x5c)
                elif address in (0x150f00, 0x151c90, 0x1ea490):
                    offset = 12 if address == 0x150f00 else 8 if address == 0x151c90 else 4
                    pointer = self.u(sp + offset)
                    row['control_input_hex'] = self.raw(pointer, 0x5c); pending['output'] = (pointer, 0x5c)
                elif address == 0xc8140:
                    # C8140 also reads three basis vectors and rep-movsd's
                    # +58..+7B. This is the incoming marshalling record, not
                    # BD270's separate 44-byte unit action/control block.
                    row['unit_control_input_hex'] = self.raw(g.reg_read(UC_X86_REG_ECX), 0x7c)
                if address in (0xfb120, 0xfb3f0, 0xfb560, 0xcfed0, 0xb96a0, 0x16d4c0):
                    row['registry_before'] = self.registry()
                    pending['registry'] = True
                self.transfer_calls.append(row); self.transfer_pending.append(pending)
                self.transfer_timeline.append(dict(order=row['order'], kind='entry', call_id=row['id'], state=row['before_state']))
        super().hook(g, address, size, user)

    def mem_write(self, g, access, address, size, value, user):
        if hasattr(self, 'source_calls'):
            target = None
            for low, high, name in ((UNIT + 4, UNIT + 0x1c, 'unit_link_flags'),
                (UNIT + 0x134, UNIT + 0x140, 'unit_control_owner'),
                (UNIT + 0x210, UNIT + 0x240, 'unit_inventory'), (UNIT + 0x704, UNIT + 0x71c, 'unit_action')):
                if address < high and address + size > low:
                    target = dict(kind=name, datum=f'{UI:08x}', offset=f'{address - UNIT:03x}'); break
            if target is None and POOL + 0x34 <= address < POOL + 0x42:
                target = dict(kind='object_allocator', offset=f'{address - POOL:02x}')
            if target is None:
                for n in range(2, self.u(POOL + 0x38)):
                    header = HEADERS + n * 12; body = self.u(header + 8)
                    if header <= address < header + 12:
                        target = dict(kind='item_header', index=n, offset=f'{address - header:02x}'); break
                    if not body or self.u(header + 3, 1) != 2:
                        continue
                    if any(address < body + hi and address + size > body + lo for lo, hi in
                           ((4, 0x1c), (0x28, 0x40), (0x64, 0xa4), (0xaf, 0xb0), (0x12c, 0x158), (0x16c, 0x18c), (0x22a, 0x230))):
                        target = dict(kind='item_field', datum=f'{self.u(header, 2) << 16 | n:08x}', offset=f'{address - body:03x}'); break
            if target is not None:
                require(size <= 8, 'Wide observed store requires a separate exact-value capture')
                self.transfer_timeline.append(dict(order=self.order(), kind='store', phase=self.active,
                    instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}', address=f'{address:08x}', size=size,
                    before_hex=self.raw(address, size), written_hex=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex(), target=target))
        super().mem_write(g, access, address, size, value, user)


def run_original(name, measured):
    fresh = BASE / 'weapon-attachment' / name
    original_class = source.AttachmentTrace; original_write = Path.write_text; argv = sys.argv

    def protected_write(path, data, *args, **kwargs):
        destination = path.resolve(); raw = data.encode(kwargs.get('encoding') or 'utf-8')
        if destination.parent == BASE / 'weapon-attachment' / 'instructions':
            require(destination.exists() and destination.read_bytes() == raw, f'Frozen listing changed: {destination}')
            return len(data)
        require(destination == fresh, f'Unexpected original output: {destination}')
        frozen_write(destination, raw); return len(data)

    if measured:
        source.AttachmentTrace = type('MeasuredAttachmentTrace', (TransferMeasurement, original_class), {})
    try:
        Path.write_text = protected_write; sys.argv = ['attachment', '--output', str(fresh)]
        source.main()
    finally:
        source.AttachmentTrace = original_class; Path.write_text = original_write; sys.argv = argv
    require(fresh.read_bytes() == (BASE / 'weapon-attachment/report.json').read_bytes(), 'Original whole report differs')
    return dict(measured=measured, path=str(fresh.relative_to(ROOT)), sha256=sha(fresh), byte_identical=True,
        frozen_listing_writes_compared_and_suppressed=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=OUT)
    parser.add_argument('--report-name', default='report.json')
    parser.add_argument('--preflight-only', action='store_true')
    args = parser.parse_args(); out = args.output_dir.resolve()
    require(ROOT / 'analysis' in out.parents, 'Evidence writes must remain under ignored analysis')
    require(Path(args.report_name).name == args.report_name and args.report_name.endswith('.json'), 'Report name must be a JSON basename')
    item_manifest = BASE / 'pickup-item-point/manifest.json'
    require(sha(item_manifest) == '9ef827dcd6a4f949243c8d9bbd7288c066c9a06141f79b1c465c1ff7186f0673', 'FBFA0 freeze differs')
    frozen = json.loads(item_manifest.read_text()); pins = dict(frozen['source_pins'])
    pins[str(item_manifest.relative_to(ROOT))] = sha(item_manifest)
    for name, payload in frozen['payloads'].items():
        path = BASE / 'pickup-item-point' / name
        require(sha(path) == payload['sha256'] and path.stat().st_size == payload['bytes'], 'FBFA0 payload differs')
        pins[str(path.relative_to(ROOT))] = payload['sha256']
    for path, key in [('tools/oracle_mp_item_point_1_1.py', 'tool_sha256'),
            ('tools/oracle_mp_item_point_audit_1_1.py', 'audit_tool_sha256'),
            ('docs/MP_ITEM_POINT_RESEARCH.md', 'document_sha256')]:
        pins[path] = frozen[key]
    for dep in json.loads((BASE / 'weapon-attachment/report.json').read_text())['dependencies']:
        pins[dep['path']] = dep['sha256']
    pins['analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json'] = '657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f'
    for path, pin in pins.items():
        require(sha(ROOT / path) == pin, f'Frozen evidence differs: {path}')
    preflight = run_original('transfer-preflight.json', False)
    if args.preflight_only:
        frozen_write(out / 'preflight.json', encoded(preflight)); return
    measured = run_original('transfer-measured-baseline.json', True)
    cases = []
    old = json.loads((BASE / 'weapon-attachment/report.json').read_text())
    for i, instance in enumerate(INSTANCES):
        require(not instance.transfer_pending, 'Unfinished transfer call')
        cases.append(dict(id=i, name=old['cases'][i]['name'], states=instance.transfer_states,
            roots=instance.transfer_roots, calls=instance.transfer_calls, timeline=instance.transfer_timeline,
            samples=old['cases'][i]['samples'], source_pose_fixture=old['cases'][i]['pose_fixture'],
            actual_services=[e for e in old['cases'][i]['events'] if 'service' in e],
            final_state=instance.transfer_state()))
    instructions = []; executed = set().union(*(t.transfer_executed for t in INSTANCES))
    for entry in ENTRIES:
        routine = _trace(INSTANCES[0].code, entry, INSTANCES[0].starts | set(ENTRIES))
        rows = [INSTANCES[0].code.at(a) for a in sorted(routine.code)]
        path = out / 'instructions' / f'{entry:08x}.asm'
        frozen_write(path, ('\n'.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}' for r in rows) + '\n').encode())
        instructions.append(dict(entry=f'{entry:08x}', path=str(path.relative_to(ROOT)), sha256=sha(path),
            rows=len(rows), executed_rows=sum(r.address in executed for r in rows)))
    report = dict(version=1, tool_sha256=sha(Path(__file__)), source_pins=pins,
        preflight=preflight, measured_baseline=measured, cases=cases, instructions=instructions,
        boundary='Read-only untouched original transfer on frozen allocated/equipped fixture. Nearby root, explicit unit/player arenas, heap growth/residency, input packet/duration and bounded root schedule are inherited inputs. Source capability, inventory ownership, registration removal and marker attachment execute. Full movement/world registration/current live pose/Havok and Xbox reciprocal parity remain unresolved. Direct drop is a distinct routine-root fixture with a supplied Havok result, not an ordinary pickup claim.')
    frozen_write(out / args.report_name, encoded(report))
    print(json.dumps(dict(cases=len(cases), calls=sum(len(c['calls']) for c in cases),
        states=sum(len(c['states']) for c in cases), stores=sum(sum(e['kind'] == 'store' for e in c['timeline']) for c in cases),
        report_sha256=sha(out / args.report_name))))


if __name__ == '__main__':
    main()
