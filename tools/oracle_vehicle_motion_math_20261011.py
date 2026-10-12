#!/usr/bin/env python3
"""Execute original state construction and composed vehicle motion on declared inputs.

Vehicle, model and render-model metadata bytes come from the owned Coagulation,
Headlong and Zanzibar maps. Object, rigid-body, component, node-pose, animation,
control and environment records are declared test inputs. The original state
builder produces the physics state used by subsequent motion calls; that state
is never inserted from a recorded result. These are not live driving captures. No callee output, contact,
successful owner result or patched instruction is supplied. Paths stop at the
actual next unported owner.
"""
import argparse
import itertools
import json
import math
import random
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from research_vehicle_control_20261011 import sha
from inspect_multiplayer import Cache
from oracle_vehicle_update_20261011 import controls as update_controls
from export_mp_physics_contact_lifecycle import segments

BASE = 0x1000000
POOL, HEADERS, BODY, TAGS, DEFINITION, CLOCK = (BASE + n for n in (0, 0x1000, 0x2000, 0x3000, 0x4000, 0x5000))
STACK = BASE + 0xd000
OUTPUT = BASE + 0x6000
STOP = BASE + 0xf000
UNIT = 0xe6f80000
BODY = BASE + 0x100000
PARENT_BODY = BODY + 0x8000
TAGS = BASE + 0x30000
COMPONENT_POOL, COMPONENT, BODY_ARRAY, HAVOK_BODY = (BASE + n for n in (0x7000, 0x8000, 0x8300, 0x8400))
MOTION = BASE + 0x8500
STATE_AREA = BASE + 0x10000
SESSION = BASE + 0x6400
PHYSICS_ENVIRONMENT = BASE + 0x6500
ACTUATOR, ACTUATOR_DATA = BASE + 0x6100, BASE + 0x6200


def capture(g):
    result = dict(gp=[g.reg_read(getattr(R, 'UC_X86_REG_' + name))
        for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')],
        xmm=[f'{g.reg_read(getattr(R, f"UC_X86_REG_XMM{n}")):032x}' for n in range(8)],
        eflags=g.reg_read(R.UC_X86_REG_EFLAGS), fs_base=g.reg_read(R.UC_X86_REG_FS_BASE),
        x87_control_word=g.reg_read(R.UC_X86_REG_FPCW), mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),
        x87_status=g.reg_read(R.UC_X86_REG_FPSW), x87_tag=g.reg_read(R.UC_X86_REG_FPTAG))
    top = (result['x87_status'] >> 11) & 7
    result['x87_values'] = []
    empty = False
    for n in range(8):
        index = (top + n) & 7
        if (result['x87_tag'] >> (index * 2)) & 3 == 3:
            empty = True
            continue
        assert not empty, 'Sparse x87 stack requires a separate owner'
        mantissa, exponent = g.reg_read(getattr(R, f'UC_X86_REG_FP{index}'))
        result['x87_values'].append(mantissa.to_bytes(8, 'little').hex() + exponent.to_bytes(2, 'little').hex())
    return result


def definitions():
    records = []
    for name in ('coagulation', 'headlong', 'zanzibar'):
        path = ROOT / 'assets/disc/maps' / (name + '.map')
        pin = sha(path)
        c = Cache(path)
        for datum, tag in sorted(c.tags.items()):
            if tag['group'] != 'vehi':
                continue
            assert tag['offset'] is not None and tag['size'] >= 0x300
            p = tag['offset']
            raw = bytes(c.data[p:p + tag['size']])
            dependent=[]
            model=c.tags[c.u(p+0x38)]
            render=c.tags[c.u(model['offset']+4)]
            for t in (model,render):
                data=bytes(c.data[t['offset']:t['offset']+t['size']])
                dependent.append(dict(datum=int(t['datum'],16), virtual_address=t['offset']+c.magic,
                    file_offset=t['offset'], raw_hex=data.hex(), sha256=__import__('hashlib').sha256(data).hexdigest()))
            records.append(dict(dependent_tags=dependent, map=str(path.relative_to(ROOT)), map_sha256=pin,
                datum=datum, name=tag['name'], prefix_hex=raw.hex(), virtual_address=p+c.magic,
                file_offset=p, prefix_sha256=__import__('hashlib').sha256(raw).hexdigest()))
        c.data.close()
        c.stream.close()
    return records



def controls(records):
    rows=update_controls(records)
    baseline=dict(rows[0])
    def add(entry, **values):
        row=dict(baseline);row.update(entry=entry, **values);rows.append(row)
    for definition, component, table, flags in itertools.product(range(len(records)), (-1,0),
            (0x41b770,0x41b7f0,0x41b8f0), (0,1)):
        add(0x2095a0, definition=definition, component=component, motion_table=table,
            physics_flags=flags, constructed_motion=True, stack_values={4:STATE_AREA,8:UNIT})
    # The real composed updater produces the state used by its class dispatch.
    # No state-builder return or final physics state is inserted into a callee.
    for definition, table, control in itertools.product(range(len(records)),
            (0x41b770,0x41b7f0,0x41b8f0), ((0.,0.,0.),(0.5,-0.25,1.),(-1.,1.,-0.5))):
        add(0xf1420, definition=definition, component=0, motion_table=table,
            flags_c0=0x40, flags348=0, controls=control, constructed_motion=True, query_values=(0.,)*5, unit_current=0., unit_previous=0.)
    for definition, table, control in itertools.product(range(len(records)),
            (0x41b770,0x41b7f0,0x41b8f0), ((0.,0.,0.),(0.5,-0.25,1.),(-1.,1.,-0.5))):
        kind=struct.unpack_from('<h',bytes.fromhex(records[definition]['prefix_hex']),0x1f0)[0]
        entry={0:0xf3310,1:0xf35c0,3:0xf39a0,4:0xf4650,5:0xf5e40,6:0x2090a0}.get(kind)
        if entry is None:continue
        registers={'ESP':STACK};stack={0:STOP}
        if kind==0:registers['EBX']=UNIT;stack[4]=STATE_AREA
        elif kind in (1,3,4,5):stack.update({4:UNIT,8:STATE_AREA,12:0})
        else:stack.update({4:UNIT,8:STATE_AREA,12:0,16:0,20:0})
        add(0x2095a0, definition=definition,component=0,motion_table=table,constructed_motion=True,
            controls=control, query_values=(0.,)*5, unit_current=0.,unit_previous=0.,
            stack_values={4:STATE_AREA,8:UNIT}, subsequent_call=dict(entry=entry,registers=registers,stack=stack))
    for n in (0.,1.e-8,1.,-1.,2.,1.e8):
        add(0x30bd0, registers={'EAX':OUTPUT}, input_vector=(n,n*0.5,-n))
    for table in (0x41b770,0x41b7f0,0x41b8f0):
        add(0x1d4510, motion_table=table, registers={'EAX':0,'ECX':COMPONENT,'ESI':OUTPUT})
        add(0x1d45a0, motion_table=table, registers={'EAX':0,'ECX':COMPONENT})
    def extended(mantissa, exponent):
        return mantissa.to_bytes(8,'little').hex()+exponent.to_bytes(2,'little').hex()
    one=extended(0x8000000000000000,0x3fff)
    two=extended(0x8000000000000000,0x4000)
    five_half=extended(0xb000000000000000,0x4001)
    negative_five_half=extended(0xb000000000000000,0xc001)
    zero=extended(0,0);negative_zero=extended(0,0x8000)
    infinity=extended(0x8000000000000000,0x7fff)
    quiet=extended(0xc0123456789abcde,0x7fff)
    signaling=extended(0x80123456789abcde,0x7fff)
    denormal=extended(1,0)
    # The original thunk receives denominator in ST(0), numerator in ST(1).
    pairs=((five_half,two),(negative_five_half,two),(two,one),(zero,two),
           (negative_zero,two),(two,infinity),(infinity,two),(two,zero),
           (two,negative_zero),(quiet,two),(two,quiet),(signaling,two),
           (two,signaling),(quiet,signaling),(denormal,two),(two,denormal))
    for pair,word,top,sticky in itertools.product(pairs,(0x37f,0x77f,0xb7f,0xf7f,0x7f,0x27f),range(8),(0,0x20)):
        add(0x37a15a,depth=2,top=top,fp_control=word,x87_values=[pair[1],pair[0]],x87_exceptions=sticky)
    for word,top,sticky in itertools.product((0x37f,0x77f,0xb7f,0xf7f,0x7f,0x27f),range(8),range(64)):
        add(0x327855,top=top,depth=0,fp_control=word,x87_exceptions=sticky)
    for bits in (0,1,2,4,8,16,32,0x80000,0x100,0x200,0x300,0x10000,0x20000,0x40000,0xfffff):
        add(0x32778e,registers={'EBX':bits})
    for word in (0x37f,0x77f,0xb7f,0xf7f,0x7f,0x27f,0x133f,0x123f,0x103f):
        add(0x3276fc,registers={'EBX':word})
        # Round/precision masks preserve all exception masks in these inputs.
        add(0x327866,stack_values={4:0x9001f,8:0x8001f},fp_control=word)
    return rows


def declared_json(value):
    if isinstance(value, float) and not math.isfinite(value):
        return dict(float32_hex=struct.pack('<f', value).hex())
    if isinstance(value, dict):
        return {key: declared_json(item) for key, item in value.items()}
    if isinstance(value, (tuple, list)):
        return [declared_json(item) for item in value]
    return value


def execute(path, paired, latest, records, rows):
    assert sha(path) == (NEW_PIN if latest else PIN)
    image = Xbe(path)
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in image.sections:
        g.mem_write(section.va, image.section_bytes(section))
    g.mem_map(BASE, 0x120000)
    selected = {int(p, 16): q for name in ('address_map', 'instruction_map', 'entries')
                for p, q in paired[name].items()}
    sites = ({q: int(p, 16) for p, q in paired['instruction_map'].items()} if latest
             else {int(p, 16): int(p, 16) for p in paired['instruction_map']})

    code_image=_Code(image,'.text')
    mnemonics={canonical:code_image.at(actual).mnemonic for actual,canonical in sites.items()}

    def w(p, value):
        g.mem_write(p, struct.pack('<I', value & 0xffffffff))

    def address(p):
        return selected[p] if latest else p

    for p, value in ((0x4e78d0, POOL), (0x4eb114, TAGS), (0x518444, CLOCK), (0x526248, COMPONENT_POOL), (0x4edf18, SESSION), (0x526254,PHYSICS_ENVIRONMENT)):
        w(address(p), value)
    w(POOL + 0x44, HEADERS)
    w(HEADERS + 8, BODY)
    w(TAGS + 8, DEFINITION)
    g.mem_write(SESSION,bytes(0x20))
    g.mem_write(SESSION+0xc,bytes((4,)))
    g.mem_write(PHYSICS_ENVIRONMENT,struct.pack('<8f',1.,0.,0.,0.,0.,0.,0.,0.))
    g.mem_map(0x80000000,0x4000000)
    state = {}

    def code(_g, p, _n, _u):
        if p not in sites:
            state['exit_entry'] = p
            g.emu_stop()
        else:
            state['instructions'].append(sites[p])

    def read(_g, _a, p, n, _x, _u):
        state['events'].append(dict(kind='R', instruction=sites[g.reg_read(R.UC_X86_REG_EIP)],
                                    address=p, raw=bytes(g.mem_read(p, n)).hex()))

    def write(_g, _a, p, n, value, _u):
        state['events'].append(dict(kind='W', instruction=sites[g.reg_read(R.UC_X86_REG_EIP)],
            address=p, before=bytes(g.mem_read(p, n)).hex(),
            written=(value & ((1 << (n * 8)) - 1)).to_bytes(n, 'little').hex()))

    g.hook_add(UC_HOOK_CODE, code)
    g.hook_add(UC_HOOK_MEM_READ, read)
    g.hook_add(UC_HOOK_MEM_WRITE, write)
    rng = random.Random(0xf06b0)
    cases = []
    for index, row in enumerate(rows):
        assert not (row.get('parent') and row.get('child')), 'Declared object hierarchy must be acyclic'
        # Real field layouts with explicit, independently seeded input state.
        # No constructor, body creation, solver or activation result is supplied.
        g.mem_write(HEADERS, bytes(0x40))
        w(HEADERS + 8, BODY)
        w(HEADERS + 20, PARENT_BODY)
        g.mem_write(HEADERS + 2, bytes((row.get('object_header_flags', 0), 1)))
        w(COMPONENT_POOL + 0x44, COMPONENT)
        component_bytes = bytearray(0xa0)
        component_bytes[0x18] = 0
        struct.pack_into('<I',component_bytes,0x20,0xffffffff)
        struct.pack_into('<I', component_bytes, 0x70, BODY_ARRAY)
        struct.pack_into('<I', component_bytes, 0x74, 1)
        g.mem_write(COMPONENT, bytes(component_bytes))
        w(BODY_ARRAY + 0x40, HAVOK_BODY)
        g.mem_write(HAVOK_BODY, bytes(0x80))
        g.mem_write(HAVOK_BODY + 0x40, bytes((row.get('motion_flag', 1),)))
        w(HAVOK_BODY + 0x3c, MOTION)
        g.mem_write(MOTION, rng.randbytes(0x120))
        table=row.get('motion_table',0x41b8f0)
        later_table=next(b['later_table'] for b in paired['virtual_bindings'] if b['table']==table)
        w(MOTION,later_table if latest else table)
        g.mem_write(MOTION+0x10,struct.pack('<8f',1.,0.,0.,0.,1.,1.,1.,1.))
        g.mem_write(MOTION+0x40,struct.pack('<8f',*row['velocity'],0.,0.,0.,0.,0.,0.))
        g.mem_write(MOTION+0x70,struct.pack('<4f',1.,1.,1.,0.))
        g.mem_write(MOTION+0xe0,struct.pack('<4f',1.,2.,4.,0.))
        body = bytearray(rng.randbytes(0x6000))
        struct.pack_into('<I', body, 0, 0xab120000)
        struct.pack_into('<2f', body, 0x150, *row['velocity'])
        struct.pack_into('<3f', body, 0x70, *row['forward'], 0.)
        struct.pack_into('<3f', body, 0x7c, 0., 0., 1.)
        struct.pack_into('<h', body, 0x116, row['node_position_offset'])
        struct.pack_into('<hh',body,0x118,0,0x500)
        struct.pack_into('<3f',body,0x64,0.,0.,0.)
        struct.pack_into('<3f', body, 0x1b0, *row['controls'])
        struct.pack_into('<f', body, 0x358, row['turn'])
        body[0x38b] = row['gear'] & 255
        body[0x349] = row['flags349']
        body[0x350] = row.get('transient_state', 0)
        body[0x351] = row.get('transient_ticks', 0)
        struct.pack_into('<I', body, 0x14, UNIT + 1 if row.get('parent') else 0xffffffff)
        struct.pack_into('<I', body, 0xb4, row.get('component', -1) & 0xffffffff)
        struct.pack_into('<I', body, 0xd4, row.get('object_field_d4', 0xffffffff))
        struct.pack_into('<I', body, 0x10, UNIT + 1 if row.get('child') else 0xffffffff)
        struct.pack_into('<I', body, 0xc, 0xffffffff)
        struct.pack_into('<h', body, 0x346, 0x580)
        body[0x580:0x590] = bytes(0x10)
        struct.pack_into('<I', body, 0x584, row.get('update_mask', 0))
        struct.pack_into('<H', body, 0xc0, row.get('flags_c0', 0))
        struct.pack_into('<h', body, 0x12a, 0x480)
        body[0x10a] = row.get('flags10a', 0)
        body[0x348] = row.get('flags348', body[0x348])
        body[0x34d] = row.get('self_right_state', 0)
        body[0x34e] = row.get('self_right_elapsed', 0)
        struct.pack_into('<f', body, 0x84, row.get('up_z', 1.))
        for offset, key in ((0x334, 'unit_current'), (0x338, 'unit_previous')):
            if key in row:
                struct.pack_into('<f', body, offset, row[key])
        if 'query_values' in row:
            for offset, value in zip((0x354, 0x35c, 0x25c, 0x260, 0x358), row['query_values']):
                struct.pack_into('<f', body, offset, value)
        if 'decay_values' in row:
            for offset, value in zip((0x378, 0x354, 0x35c, 0x358, 0x370, 0x374), row['decay_values']):
                struct.pack_into('<f', body, offset, value)
        body[0x480:0x580] = bytes(0x100)
        if not row.get('animation_valid'):
            struct.pack_into('<I', body, 0x480 + 0x68, 0xffffffff)
        for offset, value in row.get('animation_overrides', {}).items():
            width = 2 if offset in (6, 0x46) else 4 if offset in (0, 0x40, 0x68) else 1
            body[0x480 + offset:0x480 + offset + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
        if row.get('constructed_motion'):
            struct.pack_into('<I',body,4,0)
            struct.pack_into('<I',body,0x134,row.get('motion_object_flags',0))
            struct.pack_into('<I',body,0x248,0xffffffff)
            struct.pack_into('<I',body,0x12c,0xffffffff)
            struct.pack_into('<h',body,0x114,128*0x34)
            struct.pack_into('<h',body,0x116,0x1000)
            for n in range(128):struct.pack_into('<13f',body,0x1000+n*0x34,1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.)
        g.mem_write(BODY, bytes(body))
        parent_body = bytearray(body)
        struct.pack_into('<I', parent_body, 0x14, UNIT if row.get('child') else 0xffffffff)
        struct.pack_into('<I', parent_body, 0x10, 0xffffffff)
        g.mem_write(PARENT_BODY, bytes(parent_body))
        definition=records[row['definition']]
        raw_definition=bytes.fromhex(definition['prefix_hex'])
        for tag in definition['dependent_tags']:
            g.mem_write(tag['virtual_address'],bytes.fromhex(tag['raw_hex']))
            w(TAGS+(tag['datum']&0xffff)*16+8,tag['virtual_address'])
        g.mem_write(definition['virtual_address'],raw_definition)
        g.mem_write(DEFINITION,raw_definition)
        if 'physics_flags' in row:w(DEFINITION+0x2ac,row['physics_flags'])
        if 'scalar_rates' in row:
            g.mem_write(DEFINITION + 0x22c, struct.pack('<4f', *row['scalar_rates']))
        if row.get('unit_rates'):
            for offset, value in zip((0x1d4, 0x1dc), row['unit_rates']):
                g.mem_write(DEFINITION + offset, struct.pack('<f', value))
        if 'definition_flags_1ec' in row:
            w(DEFINITION + 0x1ec, row['definition_flags_1ec'])
        g.mem_write(STACK - 0x4000, rng.randbytes(0x5000))
        g.mem_write(OUTPUT, rng.randbytes(0x100))
        g.mem_write(STATE_AREA, rng.randbytes(0x2000))
        if 'input_vector' in row:g.mem_write(OUTPUT,struct.pack('<3f',*row['input_vector']))
        g.mem_write(ACTUATOR, rng.randbytes(0x100))
        g.mem_write(ACTUATOR_DATA, rng.randbytes(0x100))
        w(ACTUATOR + 0x4c, row.get('actuator_count', 0))
        w(ACTUATOR + 0x50, ACTUATOR_DATA)
        g.mem_write(ACTUATOR_DATA + 0x3c, bytes((row.get('actuator_flag', 0),)))
        w(STACK, STOP)
        w(STACK + 4, UNIT)
        w(STACK + 8, OUTPUT)
        if row['entry'] == 0xf02d0:
            w(STACK + 4, OUTPUT)
        if row.get('scalar_control'):
            w(STACK + 4, struct.unpack('<I', struct.pack('<f', row['direction']))[0])
            w(STACK + 8, struct.unpack('<I', struct.pack('<f', row['dt']))[0])
        for offset, value in row.get('stack_values', {}).items():
            w(STACK + int(offset), value)
        g.mem_write(CLOCK, struct.pack('<I', 30 << 16))
        w(CLOCK + 4, struct.unpack('<I', struct.pack('<f', row['dt']))[0])
        w(CLOCK + 8, 123)
        for n, name in enumerate(('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')):
            value = rng.getrandbits(32)
            if name == 'EAX':
                value = row.get('allocation_bytes', UNIT)
            elif name == 'ESP':
                value = STACK
            elif row.get('scalar_control') and name == 'ECX':
                value = BODY + 0x358
            elif row.get('scalar_control') and name == 'EDX':
                value = DEFINITION + 0x22c
            value = row.get('registers', {}).get(name, value)
            g.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
        for n in range(8):
            g.reg_write(getattr(R, f'UC_X86_REG_XMM{n}'), rng.getrandbits(128))
        if row.get('scalar_control'):
            g.reg_write(R.UC_X86_REG_XMM3, struct.unpack('<I', struct.pack('<f', row['target']))[0])
        g.reg_write(R.UC_X86_REG_EFLAGS, 2)
        g.reg_write(R.UC_X86_REG_FPCW, row['fp_control'])
        g.reg_write(R.UC_X86_REG_MXCSR, row['mxcsr'])
        g.reg_write(R.UC_X86_REG_FPSW, (row['top'] << 11)|row.get('x87_exceptions',0))
        tag = 0xffff
        values = ((0x8000000000000000, 0x3fff), (0x8000000000000000, 0xbffe),
                  (0x8000000000000000, 0x4000))
        for n in range(row['depth']):
            physical = (row['top'] + n) & 7
            tag &= ~(3 << (physical * 2))
            value=values[n] if 'x87_values' not in row else (int.from_bytes(bytes.fromhex(row['x87_values'][n])[:8],'little'),int.from_bytes(bytes.fromhex(row['x87_values'][n])[8:],'little'))
            g.reg_write(getattr(R, f'UC_X86_REG_FP{physical}'), value)
        g.reg_write(R.UC_X86_REG_FPTAG, tag)
        state.update(events=[], instructions=[], exit_entry=None)
        initial_abi = capture(g)
        try:
            g.emu_start(address(row['entry']), 0, count=30000)
        except Exception as error:
            raise RuntimeError(dict(case=index, input=row, eip=hex(g.reg_read(R.UC_X86_REG_EIP)))) from error
        if state['exit_entry']==STOP and row.get('subsequent_call') and (g.reg_read(R.UC_X86_REG_EAX)&255):
            call=row['subsequent_call']
            for offset,value in sorted(call['stack'].items(),key=lambda item:str(item[0])):
                p=STACK+int(offset);raw=struct.pack('<I',value)
                state['events'].append(dict(kind='W',instruction=0,address=p,before=bytes(g.mem_read(p,4)).hex(),written=raw.hex()))
                g.mem_write(p,raw)
            for name,value in call['registers'].items():g.reg_write(getattr(R,'UC_X86_REG_'+name),value)
            state['exit_entry']=None
            try:g.emu_start(address(call['entry']),0,count=30000)
            except Exception as error:raise RuntimeError(dict(case=index,input=row,eip=hex(g.reg_read(R.UC_X86_REG_EIP)))) from error
        assert state['exit_entry'] is not None, dict(case=index, input=row,
            eip=hex(g.reg_read(R.UC_X86_REG_EIP)), last_sites=state['instructions'][-16:])
        complete = state['exit_entry'] == STOP
        if row['entry'] in (0xf06b0, 0xf5930, 0x208d80, 0x208e70, 0x326af0, 0xd26c0, 0xf2960, 0xf1270, 0xf11a0, 0xf1d00, 0x208ed0):
            assert complete
        first = {}
        current = {}
        for event in state['events']:
            p = event['address']
            raw = bytes.fromhex(event['raw'] if event['kind'] == 'R' else event['before'])
            for n, value in enumerate(raw):
                if p + n not in current:
                    first[p + n] = value
                    current[p + n] = value
                assert current[p + n] == value
            if event['kind'] == 'W':
                for n, value in enumerate(bytes.fromhex(event['written'])):
                    current[p + n] = value
        # Intel SDM Vol.1 Table 8-1: arithmetic, loads/stores and FCOMIP
        # leave C0/C2/C3 undefined. FSIN/FCOS define C2; FCOM-family
        # comparisons define C0/C2/C3. TOP remains part of every contract.
        status_mask=0x7d3f
        for site in state['instructions']:
            mnemonic=mnemonics[site]
            if mnemonic in ('fxam','fprem'):status_mask=0x7f3f
            elif mnemonic in ('fcom','fcomp','fcompp','fucompp'):status_mask=0x7d3f
            elif mnemonic in ('fsin','fcos'):status_mask=0x3c3f
            elif mnemonic.startswith('f') and mnemonic not in ('fnstsw','fnstcw','fldcw','fnclex'):status_mask=0x383f
        cases.append(dict(x87_defined_status_mask=status_mask,id=index, entry=row['entry'], entry_abi=initial_abi, exit=capture(g),
            exit_entry=state['exit_entry'], return_address=STOP, complete=complete,
            initial_banks=[dict(address=p, hex=b.hex()) for p, b in segments(first)],
            final_banks=[dict(address=p, hex=b.hex()) for p, b in segments(current)],
            instructions=state['instructions'], writes=[e for e in state['events'] if e['kind'] == 'W'],
            estimates=[], timestamps=[], packets=[], declared_input=declared_json(row), subsequent_call=row.get('subsequent_call')))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--correspondence', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    paired = json.loads(args.correspondence.read_text())
    assert sha(ROOT / 'tools/oracle_vehicle_runtime_20261011.py') == 'ac008f33905cff72941f4b789ea173f18e166c07bffa3be8b00430307a40d412'
    assert paired['extension_tool_sha256'] == sha(ROOT / 'tools/research_vehicle_motion_math_20261011.py')
    records = definitions()
    rows = controls(records)
    args.output.mkdir(parents=True, exist_ok=False)
    stats = {}
    for name, latest, path in (('u5', True, NEW_DEFAULT),):
        cases = execute(path, paired, latest, records, rows)
        fixture = dict(schema=1, image='1.5' if latest else '1.1', cases=cases,
            tool_sha256=sha(__file__), correspondence_sha256=sha(args.correspondence),
            definitions=records, instruction_patches=[], native_expected_data_used=False,
            live_vehicle_driving_proof=False)
        fixture_path = args.output / f'{name}.json'
        fixture_path.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':'), allow_nan=False) + '\n')
        seen = set(p for case in cases for p in case['instructions'])
        stats[name] = dict(fixture_sha256=sha(fixture_path), cases=len(cases),
            complete=sum(case['complete'] for case in cases), instructions=sum(len(case['instructions']) for case in cases),
            ordered_writes=sum(len(case['writes']) for case in cases), covered_instructions=len(seen),
            full_state_bytes=sum(len(bytes.fromhex(bank['hex'])) for case in cases for bank in case['final_banks']),
            stop_entries=sorted({case['exit_entry'] for case in cases if not case['complete']}))
    report = dict(schema=1, tool_sha256=sha(__file__), correspondence_sha256=sha(args.correspondence),
        parent_controls_tool_sha256=sha(ROOT / 'tools/oracle_vehicle_runtime_20261011.py'),
        update_controls_tool_sha256=sha(ROOT/'tools/oracle_vehicle_update_20261011.py'),
        declared_environment=dict(session_mode=4, physics_environment_float32=[1.,0.,0.,0.,0.,0.,0.,0.], component_constraint_datum=0xffffffff, constructed_node_poses=128, node_pose_layout='13 float identity transforms', rng_seed=0xf06b0),
        x87_status_authority='https://www.intel.com/content/dam/support/us/en/documents/processors/pentium4/sb/25366521.pdf#page=220',
        reader_sha256=sha(ROOT / 'tools/inspect_multiplayer.py'),
        segments_helper_sha256=sha(ROOT / 'tools/export_mp_physics_contact_lifecycle.py'),
        fixtures=stats, declared_inputs=len(rows), authored_definitions=len(records),
        instruction_patches=[], native_expected_data_used=False, live_vehicle_driving_proof=False,
        boundary=__doc__)
    (args.output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
