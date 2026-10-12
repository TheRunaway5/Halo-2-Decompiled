#!/usr/bin/env python3
"""Run the owned TU1.5 vehicle graph through contact-state delivery.

Original code computes every state and return. New direct contact records and
all prior declared inputs are inputs, not contact/solver outcomes. These cases
remain bounded contracts and do not establish live driving or solved contacts.
The diagnostic emulator only discovers memory footprints; hardware is final
FP/status authority.
"""
import argparse,itertools,json,math,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_contact_state_tu15_20261011 import sha
from oracle_vehicle_motion_math_20261011 import (BASE,POOL,HEADERS,BODY,TAGS,DEFINITION,CLOCK,STACK,
 OUTPUT,STOP,UNIT,PARENT_BODY,COMPONENT_POOL,COMPONENT,BODY_ARRAY,HAVOK_BODY,MOTION,STATE_AREA,
 SESSION,PHYSICS_ENVIRONMENT,ACTUATOR,ACTUATOR_DATA,capture,definitions,controls as parent_controls,declared_json)
from export_mp_physics_contact_lifecycle import segments
CONTACT_STATE,CONTACT_METADATA,CONTACT_ARRAY,CONTACT_INPUT=(BASE+p for p in (0x20000,0x23000,0x24000,0x25000))
def controls(records):
 rows=parent_controls(records);baseline=dict(rows[0])
 pairs=((0x3f800000,0),(0x3f800000,0x40000000),(0x3f800000,0xbf800000),
  (0,0),(0xbf800000,0),(0x3a83126f,0),(0x3a831270,1),(1,0x80000001),
  (0x7f800000,0),(0x3f800000,0x7f800000),(0x7fc12345,0),(0x3f800000,0x7fa12345))
 for active,index,pair,rounding,alias in itertools.product((0,1,255),(0,1,7),pairs,range(4),('separate','vector','fields')):
  row=dict(baseline);row.update(entry=0x20b470,contact_input=True,active=active,contact_index=index,
   cap_bits=pair[0],distance_bits=pair[1],input_alias=alias,surface=0xa500+index,default_surface=0x5a00+index,
   mxcsr=0x1f80|(rounding<<13),registers={'EAX':index,'ECX':CONTACT_INPUT,'EDX':CONTACT_STATE})
  rows.append(row)
 for top,depth,mxcsr in itertools.product(range(8),(0,3),(0x9fc0,0x1fbf)):
  row=dict(baseline);row.update(entry=0x20b470,contact_input=True,active=1,contact_index=1,
   cap_bits=1,distance_bits=0x80000001,input_alias='separate',surface=0xffff,default_surface=0,
   mxcsr=mxcsr,top=top,depth=depth,x87_exceptions=0x3f,
   registers={'EAX':1,'ECX':CONTACT_INPUT,'EDX':CONTACT_STATE})
  rows.append(row)
 for skip,list_datum,count,reference_count,filter_kind,record_kind,subtype in itertools.product(
  (0,1,255),(-1,2),(-1,0,1,3),(-1,0,1,2),(-128,0,127),(-128,0,127),(0,1,2)):
  row=dict(baseline);row.update(entry=0x20b3b0,release_input=True,component=0,
   release_list=list_datum,release_count=count,reference_count=reference_count,
   filter_kind=filter_kind,record_kind=record_kind,subtype=subtype,
   registers={'EAX':OUTPUT},stack_values={4:filter_kind&0xffffffff,8:skip})
  rows.append(row)
 return rows

def execute(path, paired, latest, records, rows):
    assert latest and sha(path) == NEW_PIN
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
        if row.get('contact_input'):
            g.mem_write(CONTACT_STATE,rng.randbytes(0x2000))
            g.mem_write(CONTACT_METADATA,rng.randbytes(0x100))
            g.mem_write(CONTACT_ARRAY,rng.randbytes(0x300))
            w(CONTACT_STATE+4,CONTACT_METADATA)
            w(CONTACT_METADATA+0x48,CONTACT_ARRAY)
            w(CONTACT_ARRAY+row['contact_index']*0x4c+0xc,row['cap_bits'])
            slot=CONTACT_STATE+row['contact_index']*0xd8+0xb10
            input_address={'separate':CONTACT_INPUT,'vector':slot+0xa0,'fields':slot+0x70}[row['input_alias']]
            record=bytearray(rng.randbytes(0x40));record[0]=row['active']
            struct.pack_into('<I',record,4,row['distance_bits'])
            struct.pack_into('<H',record,8,row['surface'])
            struct.pack_into('<3I',record,0x18,0x3f800000,0xc0200000,0x3e800000)
            g.mem_write(input_address,bytes(record))
            w(0x474d10,0x44ac40)
            g.mem_write(0x55d728,struct.pack('<H',row['default_surface']))
            row_registers=dict(row['registers']);row_registers['ECX']=input_address
        else:row_registers=row.get('registers',{})
        if row.get('release_input'):
            w(OUTPUT,UNIT)
            w(COMPONENT+0x20,row['release_list'])
            for global_address,pool,entries in ((0x52d690,BASE+0x26000,BASE+0x28000),(0x52d68c,BASE+0x27000,BASE+0x29000)):
                g.mem_write(pool,rng.randbytes(0x80));g.mem_write(entries,rng.randbytes(0x400))
                w(global_address,pool);w(pool+0x44,entries)
            list_record=BASE+0x28000+2*0x40
            g.mem_write(list_record+2,struct.pack('<h',row['release_count']))
            for n in range(3):
                w(list_record+4+n*4,n)
                record=BASE+0x29000+n*0xa0
                g.mem_write(record+8,struct.pack('<h',row['reference_count']))
                g.mem_write(record+0xd,struct.pack('<b',row['record_kind']))
                g.mem_write(record+0x10,struct.pack('<b',row['subtype']))


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
            value = row_registers.get(name, value)
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
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--correspondence',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 paired=json.loads(a.correspondence.read_text())
 assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_vehicle_contact_state_tu15_20261011.py')
 assert sha(ROOT/'tools/oracle_vehicle_motion_math_20261011.py')=='e5ddfa662860e947a820d8ebf749ba5820ad9d3a1400baada942beb22380da36'
 records=definitions();rows=controls(records);a.output.mkdir(parents=True,exist_ok=False)
 cases=execute(NEW_DEFAULT,paired,True,records,rows)
 fixture=dict(schema=1,image='1.5',parity_target='English TU1.5 only',cases=cases,definitions=records,
  tool_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),instruction_patches=[],
  native_expected_data_used=False,live_vehicle_driving_proof=False)
 target=a.output/'u5.json';target.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'),allow_nan=False)+'\n')
 seen={p for c in cases for p in c['instructions']}
 report=dict(schema=1,parity_target='English TU1.5 only',tool_sha256=sha(__file__),
  correspondence_sha256=sha(a.correspondence),fixture_sha256=sha(target),cases=len(cases),
  complete=sum(c['complete'] for c in cases),required_owner_prefixes=sum(not c['complete'] for c in cases),
  new_direct_contact_cases=sum(bool(r.get('contact_input')) for r in rows),
  new_direct_release_cases=sum(bool(r.get('release_input')) for r in rows),
  instructions=sum(len(c['instructions']) for c in cases),ordered_writes=sum(len(c['writes']) for c in cases),
  full_state_bytes=sum(len(bytes.fromhex(b['hex'])) for c in cases for b in c['final_banks']),
  covered_instructions=len(seen),new_owner_covered_instructions=sum(p in seen for r in paired['tu15_new_owners'] for p in (int(line.split('\t')[0],16) for line in (a.correspondence.parent/r['listing_file']).read_text().splitlines())),
  stop_entries=sorted({c['exit_entry'] for c in cases if not c['complete']}),
  native_expected_data_used=False,instruction_patches=[],boundary=__doc__)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
