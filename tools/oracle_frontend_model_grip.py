#!/usr/bin/env python3
"""Execute unchanged retail2798A0 and grip codec with paused menu inputs.

Copied graph/clip/resource bytes have only host pointers relocated. Captured
locals are supplied destinations, so an idempotence check does not claim that
primary sampling or unrelated unit callbacks have been reproduced. RSQRTPS
executes under Unicorn and is recorded as a hardware approximation boundary.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from inspect_multiplayer import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, STACK
from unicorn.x86_const import (UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_EAX, UC_X86_REG_XMM1, UC_X86_REG_XMM4)


def words(raw):
    return struct.unpack('<' + str(len(raw)//4) + 'I', raw)


class Sampler(Retail):
    def __init__(self):
        super().__init__()
        self.calls = {}
        self.channels = []

    def hook(self, uc, pc, size, user):
        if pc in (0x2798a0, 0x279860, 0x27a100, 0x28c960):
            self.calls[f'{pc:08x}'] = self.calls.get(f'{pc:08x}', 0) + 1
        if pc == 0x1c6f60:
            sp = uc.reg_read(UC_X86_REG_ESP)
            self.channels.append(dict(raw_hex=bytes(uc.mem_read(uc.reg_read(UC_X86_REG_EBX), 32)).hex(),
                                      argv=self.read(sp+4, '<4I')))
            ret = self.r(sp)
            self.ret(0)
            uc.reg_write(UC_X86_REG_ESP, sp+20)
            uc.reg_write(UC_X86_REG_EIP, ret)
            return
        super().hook(uc, pc, size, user)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, default=ROOT/'analysis/frontend/profile-regression/original-model-menu-hidden-grip-control.json')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/frontend/profile-regression/frontend-model-grip-original.json')
    parser.add_argument('--stage', choices=('full', 'initial'), default='full',
        help='Full standalone codec, or BD090 initial complement of authored hlmt+A4.')
    args = parser.parse_args()
    cache = Cache(ROOT/'assets/disc/maps/mainmenu.map')
    snapshot = json.loads(args.snapshot.read_text())
    objects = snapshot.get('objects')
    if objects is None:
        obj = dict(snapshot)
        obj['channels'] = [dict(graph=f'{struct.unpack_from("<I", bytes.fromhex(obj["primary_raw_hex"]))[0]:08x}')]
        objects = [obj]
    names_path = ROOT/'analysis/frontend/profile-regression/model-animation-original.json'
    names_report = json.loads(names_path.read_text())
    assert names_report['xbe_sha256'] == PIN
    records = []
    for obj in objects:
        r = Sampler()
        control = bytes.fromhex(obj['control_hex'])
        header = bytes.fromhex(obj['object_header_hex'])
        graph_datum = int(obj['channels'][0]['graph'], 16)
        packed_id = struct.unpack_from('<I', control, 0x10c)[0]
        assert header[0x212] == 0 and header[0x213] == 255
        assert packed_id == struct.unpack_from('<I', control, 0x110)[0]
        assert packed_id & 0xffff == 0xffff
        assert all(struct.unpack_from('<2I', control, at) == (0xffffffff, 0xffffffff) for at in (0x9c, 0xbc, 0xdc))
        assert struct.unpack_from('<2I', control, 0x104) == (0xffffffff, 0xffffffff)
        graph_tag = cache.tags[graph_datum]
        graph_offset = graph_tag['offset']
        # Original name resolution has a separate frozen1CC2F0 probe. Use
        # its equipped combat names as explicit inputs;1CBAD0's policy
        # searches the complete overlay table then the ordinary actions.
        names = next(row['resolved_combat_names'] for row in names_report['records']
                     if int(row['unit'], 16) == int(obj['definition'], 16))
        r.g.mem_map(0x80000000, 0x4000000)
        graph_va = graph_offset+cache.magic
        r.g.mem_write(graph_va, bytes(cache.data[graph_offset:graph_offset+graph_tag['size']]))
        r.w(r.table+(graph_datum&0xffff)*16+8, graph_va)
        out = r.alloc(bytes(4));found = [r.alloc(bytes(4)) for _ in range(3)]
        looked_up = []
        for entry in (0x1dbc80, 0x1db170):
            r.run(entry, argv=[graph_va, out, *names[:3], *found],
                  registers=[(UC_X86_REG_EAX, 0x0400060b)])
            looked_up.append(r.r(out))
        selected = next((value for value in looked_up if value != 0xffffffff), 0xffffffff)
        assert selected == packed_id, 'Original grip lookup differs from captured control'
        index = packed_id >> 16
        clip_offset = cache.block(graph_offset+0x2c, 0x6c)[index]
        clip_raw = bytes(cache.data[clip_offset:clip_offset+0x6c])
        count = clip_raw[0x13]
        assert count == obj['node_count'] and clip_raw[0x10] == 2
        assert cache.h(clip_offset+0x14) == 1
        assert cache.u(clip_offset+0x64) == 0
        group = cache.block(graph_offset+0xac, 20)[cache.u(clip_offset+0x24)]
        location = cache.u(group+8)
        assert location >> 30 == 0, 'Only exact resident local-cache resources are covered'
        resource_offset = location + cache.u(clip_offset+0x28)
        sizes = bytes(cache.data[clip_offset+0x3c:clip_offset+0x4c])
        sf, af, movement, unknown04, static, unknown08, animated = struct.unpack('<bbhhhii', sizes)
        assert sf == static == movement == unknown04 == unknown08 == 0
        assert af == ((count+31)//32)*12
        resource = bytes(cache.data[resource_offset:resource_offset+animated+af])
        assert resource[0] == 3
        graph = r.alloc(bytes(cache.data[graph_offset:graph_offset+0xbc]))
        clip = r.alloc(clip_raw)
        data = r.alloc(resource)
        descriptor = r.alloc(struct.pack('<IIBbh', data, clip+0x3c, count, clip_raw[0x11], 1))
        baseline = bytes.fromhex(obj['local_bank_hex'])
        assert len(baseline) == count*32
        dest = r.alloc(baseline)
        callback_count, mask_ptr, destination_mask = count, 0, []
        if args.stage == 'initial':
            definition = cache.tags[int(obj['definition'], 16)]['offset']
            model = cache.tags[cache.u(definition+0x38)]['offset']
            deferred = [cache.u(model+0xa4+i*4) for i in range(8)]
            destination_mask = [(~v)&0xffffffff for v in deferred]
            active = [i for i in range(count) if destination_mask[i//32]&(1<<(i%32))]
            assert active
            callback_count = max(active)+1
            mask_ptr = r.alloc(struct.pack('<8I', *destination_mask))
        # Original112070[112C59,112DAE) resolves the source single/dual
        # branch, source slot curve and channel_start with captured bytes.
        # The sample sink is captured before executing2798A0 below. This
        # corridor does not assert preceding aiming/physics producers absent.
        unit = r.alloc(header)
        state = r.alloc(bytes.fromhex(obj['manager_hex']))
        control_at = r.alloc(control)
        pool = r.alloc(bytes(0x50))
        unit_headers = r.alloc(bytes(12))
        r.w(0x4e0300, pool); r.w(pool+0x44, unit_headers); r.w(unit_headers+8, unit)
        for at, value in [(0x18, state), (0x6c, 0), (0xec, mask_ptr), (0xf0, callback_count), (0xf4, dest)]:
            r.w(STACK+at, value)
        r.run(0x112c59, stop=0x112dae, registers=[(UC_X86_REG_EBP, control_at)])
        assert len(r.channels) == 1
        channel = bytes.fromhex(r.channels[0]['raw_hex'])
        assert struct.unpack_from('<2I', channel) == (graph_datum, packed_id)
        assert struct.unpack_from('<H', channel, 0x12)[0] == 0
        assert struct.unpack_from('<2I', channel, 0x18) == (0x3f800000, 0)
        assert r.channels[0]['argv'] == [0x3f800000, mask_ptr, callback_count, dest]
        # Original279D80[279DFC,279E2E): descriptor=ESI,frame=XMM1,
        # weight=XMM4; the remaining ten arguments stay on the stack.
        r.run(0x2798a0, argv=[graph, clip, callback_count, 0, mask_ptr, dest, 0, 0, 0, 0],
            registers=[(UC_X86_REG_ESI, descriptor), (UC_X86_REG_XMM1, 0),
                       (UC_X86_REG_XMM4, 0x3f800000)])
        output = bytes(r.g.mem_read(dest, len(baseline)))
        before, after = words(baseline), words(output)
        masks = resource[animated:animated+af]
        mask_words = struct.unpack('<'+str(af//4)+'I', masks)
        rotated = [i for i in range(count) if mask_words[i//32] & (1 << (i%32))]
        admitted_rotations = [i for i in rotated if i < callback_count and
            (not destination_mask or destination_mask[i//32]&(1<<(i%32)))]
        expected_calls = {'002798a0': 1, '0027a100': 1, '00279860': 1}
        if admitted_rotations: expected_calls['0028c960'] = len(admitted_rotations)
        assert r.calls == expected_calls, 'Required original codec instructions did not execute'
        name = cache.text(cache.u(368)+cache.u(cache.u(364)+(cache.u(clip_offset)&0xffff)*4))
        delta = []
        for node in rotated:
            a = struct.unpack('<4f', baseline[node*32:node*32+16])
            b = struct.unpack('<4f', output[node*32:node*32+16])
            delta.append(dict(node=node, captured_words=[f'{v:08x}' for v in before[node*8:node*8+4]],
                original_unicorn_words=[f'{v:08x}' for v in after[node*8:node*8+4]],
                max_component_error=max(abs(x-y) for x,y in zip(a,b))))
        assert all(before[i] == after[i] for i in range(len(before)) if i//8 not in rotated or i%8 >= 4)
        records.append(dict(object=obj['object'], unit=obj['definition'], graph=f'{graph_datum:08x}',
            clip=index, clip_name=name, authored_type=clip_raw[0x10], frame=0, weight=1,
            inheritance=None, destination_mask=[f'{v:08x}' for v in destination_mask],
            flags=0, node_count=count, callback_node_count=callback_count, stage=args.stage,
            slot_hex=control[0x80:0x84].hex(), resource_sha256=hashlib.sha256(resource).hexdigest(),
            resource_hex=resource.hex(), clip_hex=clip_raw.hex(), rotated_nodes=rotated,
            admitted_rotations=admitted_rotations,
            baseline_hex=baseline.hex(), output_hex=output.hex(), per_node=delta,
            original_local_callback_corridor='112070[112C59,112DAE)', source_channel=r.channels[0],
            original_lookup=dict(equipped_names=names[:3], action='0400060b',
                overlay=f'{looked_up[0]:08x}', ordinary=f'{looked_up[1]:08x}',
                selected=f'{selected:08x}', entries=['001DBC80','001DB170']),
            differing_words=sum(a != b for a,b in zip(before,after)), instructions=r.calls))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(xbe_sha256=PIN,
        cache_sha256=hashlib.sha256(cache.data).hexdigest(), snapshot=str(args.snapshot.resolve()),
        snapshot_sha256=hashlib.sha256(args.snapshot.read_bytes()).hexdigest(), records=records,
        source_names_sha256=hashlib.sha256(names_path.read_bytes()).hexdigest(),
        supersedes='The initial stack-only2798A0 probe was invalid: its register-passed weight was zero and no codec dispatch occurred. This report requires the real279860 and28C960 call counts; the earlier zero-difference full-sampler claim is retracted.',
        boundary='Original2798A0/279860/28C960 execute unchanged on actual authored grip data and captured destination locals. Captured-primary production and other unit callbacks remain separate. Unicorn RSQRTPS approximation is not a physical Xbox lookup claim.'), indent=2)+'\n')
    fixture = args.output.with_suffix('.tsv')
    fixture.write_text('# Exact original grip input/output local banks; all numeric words are hex.\n' +
        '\n'.join(' '.join([r['object'], r['unit'], r['graph'], f"{r['clip']:x}", f"{r['node_count']:x}",
            f"{len(r['destination_mask']):x}", *r['destination_mask'],
            *(f'{v:08x}' for v in words(bytes.fromhex(r['baseline_hex']))),
            *(f'{v:08x}' for v in words(bytes.fromhex(r['output_hex'])))]) for r in records)+'\n')
    print(json.dumps(dict(objects=len(records), affected_rotations=sum(len(r['admitted_rotations']) for r in records),
        differing_words=sum(r['differing_words'] for r in records),
        maximum_component_error=max(n['max_component_error'] for r in records for n in r['per_node']))))


if __name__ == '__main__':
    main()
