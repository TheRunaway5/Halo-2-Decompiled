#!/usr/bin/env python3
"""Actual grenade request input followed by unchanged complete request controls.

An observer captures original live pool/header/body bytes at the real B8820
call. Direct ABI controls then execute unchanged old/u5 bodies against that
literal backing. They do not allocate a projectile or admit a native match.
"""
import argparse
import hashlib
import json
import os
import struct
import subprocess
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP
import oracle_mp_grenade_owned_release_1_1 as release
from oracle_mp_title_update_display import OLD, NEW, PINS, Xbe, _Code, sha, save, encoded

ROOT = release.ROOT
PAIR = ROOT/'analysis/multiplayer/integration/object-creation-request-native/paired'
PAIR_PIN = '49f0ffd3722252559a15ac9e23b0b5ee143617a70c8e01b2dd26bbf99747673c'
STACK, STOP = 0x02008000, 0x03000000


class Capture(release.OwnedReleaseTrace):
    def hook(self, g, address, size, user):
        if self.observe and self.probe == 'genuine_admitted_grenade_ordinary_continuation' and address == 0xb8820:
            assert not hasattr(self, 'request_capture')
            pool = self.u(0x4e78d0)
            capacity, stride, records = (self.u(pool+offset) for offset in (0x20, 0x24, 0x44))
            assert 0 < capacity <= 65535 and stride == 12
            sp = g.reg_read(UC_X86_REG_ESP)
            arguments = [self.u(sp+4+4*i) for i in range(3)]
            descriptor = g.reg_read(UC_X86_REG_ESI)
            regions = [dict(address=f'{pool:08x}', raw_hex=self.raw(pool, 0x4c), owner='Actual original object pool header'),
                       dict(address=f'{records:08x}', raw_hex=self.raw(records, capacity*stride), owner='Actual whole original object record backing'),
                       dict(address=f'{descriptor:08x}', raw_hex=self.raw(descriptor, 0xc4), owner='Actual original request caller backing'),
                       dict(address=f'{arguments[2]:08x}', raw_hex=self.raw(arguments[2], 12), owner='Actual original caller location input')]
            for obj in self.snapshot()['objects']:
                regions.append(dict(address=obj['body_address'], raw_hex=obj['body_hex'], owner='Actual original allocated object body '+obj['datum']))
            self.request_capture = dict(pool=f'{pool:08x}', descriptor=f'{descriptor:08x}',
                definition=f'{arguments[0]:08x}', owner=f'{arguments[1]:08x}', location=f'{arguments[2]:08x}',
                caller=f'{self.u(sp):08x}', cpu=self.cpu(), regions=regions,
                basis_forward=self.raw(self.u(0x46fc48), 12), basis_up=self.raw(self.u(0x46fc50), 12),
                one_bits=self.raw(0x465030, 4),
                runtime_globals={f'{a:08x}':self.raw(a, 4) for a in (0x4e78d0, 0x46fc48, 0x46fc50)})
        super().hook(g, address, size, user)


def capture():
    release.OwnedReleaseTrace = Capture
    process = subprocess.Popen(['wine', str(release.motion.command.CONTEXT_OUT/'generator.exe')],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
        env=dict(os.environ, WINEDEBUG='-all', WINEPREFIX=str(release.motion.command.CONTEXT_OUT/'wine-prefix')))
    try:
        context = json.loads(process.stdout.readline())
        trace, correlation = release.prepare(context)
        continuation = release.continue_updates(trace, 1)
        value = trace.request_capture
        original = next(q for q in trace.calls if q['entry'] == '000b8820' and q['caller'] == value['caller'])
        assert 'return_cpu' in original
        value['original_call'] = original
        value['correlation'] = correlation
        value['continuation_stop'] = continuation['stages'][-1]
        return value
    finally:
        if process.poll() is None:
            try:
                process.stdin.write('x'); process.stdin.flush(); process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait(timeout=5)
        for stream in (process.stdin, process.stdout, process.stderr):
            stream.close()


def execute(captured, side, pair, output):
    path = (OLD, NEW)[side]
    assert sha(path) == PINS[str(path)]
    image = Xbe(path); code = _Code(image, '.text')
    entries = {int(q['old'],16):int(q['old' if side == 0 else 'u5'],16) for q in pair['owners']}
    pcs = {}
    for row in pair['owners']:
        left = (PAIR/'instructions'/f'{row["old"]}-old.asm').read_text().splitlines()
        right = (PAIR/'instructions'/f'{row["old"]}-u5.asm').read_text().splitlines()
        for old, new in zip(left, right, strict=True):
            pcs[int((old if side == 0 else new).split()[0],16)] = int(old.split()[0],16)
    globals_ = {int(q['old_va'],16):int(q['old_va' if side == 0 else 'new_va'],16)
                for row in pair['owners'] for q in row['memory']}
    constants = dict(one_bits=image.read(globals_[0x465030],4).hex(),
        basis_forward=image.read(int.from_bytes(image.read(globals_[0x46fc48],4),'little'),12).hex(),
        basis_up=image.read(int.from_bytes(image.read(globals_[0x46fc50],4),'little'),12).hex())
    assert all(constants[key] == captured[key] for key in constants)
    pool, descriptor, location, definition, unit = (int(captured[key],16) for key in ('pool','descriptor','location','definition','owner'))
    header = bytes.fromhex(captured['regions'][0]['raw_hex'])
    high = int.from_bytes(header[0x38:0x3c],'little')
    objects = [(q['owner'].split()[-1], int(q['address'],16), bytes.fromhex(q['raw_hex']))
               for q in captured['regions'] if q['owner'].startswith('Actual original allocated object body ')]
    weapon = next(int(datum,16) for datum, address, body in objects if body[0xaa] == 2)
    specs = [('ordinary_unit_location', 'initialize', unit, location, 0xffffffff),
             ('ordinary_unit_no_location', 'initialize', unit, 0, 0xffffffff),
             ('ordinary_weapon_location', 'initialize', weapon, location, 0xffffffff),
             ('no_owner_no_location', 'initialize', 0xffffffff, 0, 0xffffffff),
             ('owner_wrong_salt', 'initialize', unit ^ 0x10000, location, 0xffffffff),
             ('owner_at_high_water', 'initialize', (unit & 0xffff0000) | high, location, 0xffffffff)]
    for owner_name, owner in (('unit',unit),('weapon',weapon),('none',0xffffffff),('wrong_salt',unit^0x10000),('high_water',(unit&0xffff0000)|high)):
        for mask in (0xffffffff,0,1,4,0x80000000):
            specs.append((f'lookup_{owner_name}_{mask:08x}', 'lookup', owner, 0, mask))
    cases=[]; executed=set()
    for name, operation, owner, position, mask in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32); g.mem_map(0x10000,0x800000)
        for section in image.sections:
            g.mem_write(section.va,image.section_bytes(section))
        pages=set(range(0x10000,0x810000,0x1000))
        regions=captured['regions']
        for address, length in [(STACK-0x1000,0x2000),(STOP,0x1000)]+[(int(q['address'],16),len(bytes.fromhex(q['raw_hex']))) for q in regions]:
            for page in range(address&~0xfff,(address+length+0xfff)&~0xfff,0x1000):
                if page not in pages:
                    g.mem_map(page,0x1000); pages.add(page)
        for q in regions:
            g.mem_write(int(q['address'],16),bytes.fromhex(q['raw_hex']))
        g.mem_write(globals_[0x4e78d0],struct.pack('<I',pool))
        args=(definition,owner,position) if operation == 'initialize' else (mask,)
        g.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        g.reg_write(UC_X86_REG_ESP,STACK); g.reg_write(UC_X86_REG_ESI,descriptor); g.reg_write(UC_X86_REG_EAX,owner)
        stores=[]; local=set()
        def hook(_g,address,size,user):
            assert address in pcs,hex(address)
            local.add(address)
        def write(_g,kind,address,size,value,user):
            if not any(int(q['address'],16)<=address and address+size<=int(q['address'],16)+len(bytes.fromhex(q['raw_hex'])) for q in regions):
                return
            stores.append(dict(instruction=f'{pcs[g.reg_read(UC_X86_REG_EIP)]:08x}',address=f'{address:08x}',bytes=size,
                before=bytes(g.mem_read(address,size)).hex(),after=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
        g.hook_add(UC_HOOK_CODE,hook); g.hook_add(UC_HOOK_MEM_WRITE,write)
        g.emu_start(entries[0xb8820 if operation=='initialize' else 0xbbcb0],STOP,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==STOP
        after=[dict(address=q['address'],raw_hex=bytes(g.mem_read(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))).hex()) for q in regions]
        if side==0 and name=='ordinary_unit_location':
            packet=captured['original_call']['after_packets'][0]
            assert bytes(g.mem_read(descriptor,0xc4)).hex()==packet['raw_hex']
        cases.append(dict(name=name,operation=operation,definition=f'{definition:08x}',owner=f'{owner:08x}',location=f'{position:08x}',mask=f'{mask:08x}',
            returned_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',return_esp_delta=g.reg_read(UC_X86_REG_ESP)-STACK,stores=stores,after=after))
        executed.update(local)
    listing=output/f'{"old" if side==0 else "u5"}-executed.asm'
    rows=[code.at(pc) for pc in sorted(executed)]
    for row in rows:
        assert bytes(row.bytes)==image.read(row.address,len(row.bytes))
    save(listing,''.join(f'{row.address:08x} {row.bytes.hex()} {row.mnemonic} {row.op_str}\n' for row in rows).encode())
    return dict(image=str(path),image_sha256=sha(path),constants=constants,cases=cases,
        execution_listing_sha256=sha(listing),executed_instruction_rows=len(rows))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--capture',type=Path)
    args=parser.parse_args()
    assert sha(PAIR/'report.json')==PAIR_PIN
    captured=json.loads(args.capture.read_text()) if args.capture else capture()
    save(args.output/'capture.json',encoded(captured))
    pair=json.loads((PAIR/'report.json').read_text())
    images=[execute(captured,side,pair,args.output/'instructions') for side in range(2)]
    report=dict(schema='original-object-creation-request-native-v1',oracle_sha256=sha(__file__),paired_sha256=PAIR_PIN,
        capture_sha256=sha(args.output/'capture.json'),pool=captured['pool'],descriptor=captured['descriptor'],regions=captured['regions'],images=images,
        supplied_fields=['Explicit direct ABI owner/location/type-mask controls; source-created pool/header/body bytes retained unchanged.',
                         'TU1.5 direct ABI controls reuse literal TU1.1 owner backing against authenticated corresponding accesses; no complete TU1.5 owning world is claimed.'],
        instruction_patches=[],supplied_source_returns=[],
        limits=['Only the complete generic request initializer and salted typed body lookup execute here. No projectile allocation, attach, grenade release, collision, audio readiness or native gameplay integration is supplied.'])
    save(args.output/'report.json',encoded(report))
    print(json.dumps(dict(report_sha256=sha(args.output/'report.json'),cases=sum(len(q['cases']) for q in images),stores=sum(len(c['stores']) for q in images for c in q['cases']))),flush=True)


if __name__=='__main__':
    main()
