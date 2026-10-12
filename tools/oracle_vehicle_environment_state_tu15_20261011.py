#!/usr/bin/env python3
"""Run original TU1.5 environment queries, pose composition and force retirement.

Original code computes every state and return. New direct contact records and
all prior declared inputs are inputs, not contact/solver outcomes. These cases
remain bounded contracts and do not establish live driving or solved contacts.
The diagnostic emulator only discovers memory footprints; hardware is final
FP/status authority.
"""
import argparse,csv,itertools,json,math,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from research_vehicle_contact_state_tu15_20261011 import sha
from oracle_vehicle_motion_math_20261011 import (BASE,POOL,HEADERS,BODY,TAGS,DEFINITION,CLOCK,STACK,
 OUTPUT,STOP,UNIT,PARENT_BODY,COMPONENT_POOL,COMPONENT,BODY_ARRAY,HAVOK_BODY,MOTION,STATE_AREA,
 SESSION,PHYSICS_ENVIRONMENT,ACTUATOR,ACTUATOR_DATA,capture,definitions,controls as parent_controls,declared_json)
from export_mp_physics_contact_lifecycle import segments
CONTACT_STATE,CONTACT_METADATA,CONTACT_ARRAY,CONTACT_INPUT=(BASE+p for p in (0x20000,0x23000,0x24000,0x25000))
from oracle_vehicle_constraint_cleanup_tu15_20261011 import controls as cleanup_controls
RECORD,DESCRIPTORS,ASSOCIATIONS,CONTAINER,META1,META2,ATTR1,ATTR2=(BASE+p for p in (0x40000,0x41000,0x42000,0x42500,0x42600,0x42700,0x43000,0x43100))
FORCE_POOL,FORCES,FORCE_DEFINITION,BLOCK_POOL,BLOCKS,MATRIX,VECTOR2=(BASE+p for p in (0x50000,0x51000,0x52000,0x54000,0x55000,0x56000,0x57000))
ENV_REGISTRY,ENV_META,ENV_CONTEXT,ENV_TREE,ENV_NODES,ENV_PLANES,ENV_WORLD,ENV_LEAVES,ENV_CLUSTERS,ENV_PORTALS,ENV_TAG,ENV_ATTRIBUTES,ENV_LINKS,ENV_FLAGS,ENV_ITER_POOL,ENV_ITER_RECORDS,ENV_BITS,POSE_A,POSE_B,POSE_OUT=(BASE+p for p in (0x60000,0x61000,0x62000,0x63000,0x64000,0x65000,0x66000,0x67000,0x68000,0x69000,0x6a000,0x6b000,0x6c000,0x6d000,0x6e000,0x70000,0x72000,0x74000,0x75000,0x76000))
def controls(records):
 rows=cleanup_controls(records);baseline=dict(rows[0])
 def add(entry,kind,**values):
  row=dict(baseline);row.update(entry=entry,environment_input=kind,**values);rows.append(row)
 for active,point,leaf in itertools.product((-1,0),(-1.,0.,1.,float('nan')),(0,1,-1)):
  add(0x11e600,'plane_query',active=active,point=point,leaf=leaf,registers={'EDX':OUTPUT,'ESI':RECORD})
 for bits,code,target,out,point,rounding in itertools.product((-1,0),(255,0,128),(-1,3),(0,1),(-1.,0.,1.),range(4)):
  add(0x11e850,'portal_query',region=bits,code=code,target=target,out=out,point=point,mxcsr=0x1f80|(rounding<<13),
   registers={'ECX':RECORD,'EDI':OUTPUT},stack_values={4:OUTPUT+0x80 if out else 0})
 for mask,start,high in itertools.product((0,1,2,5,0x80000000,0x100000000),(0,1,31,32,33,-1),(0,3,33)):
  add(0x16ea60,'iterate',occupied=mask,start=start,high=high,registers={'ECX':ENV_ITER_POOL,'EDX':start&0xffffffff})
 for slot,high,clear,stride,cursor in itertools.product((0,1,2),(3,5),(0,8),(24,25,27),(0,4)):
  add(0x16e8a0,'retire',slot=slot,high=high,clear=clear,stride=stride,cursor=cursor,
   registers={'EDX':ENV_ITER_POOL,'EAX':0x10000|slot})
 for color,tag,count,match in itertools.product((0,1,0x1234),(0xffffffff,0),(0,1,3),(-1,0,2)):
  add(0x190420,'lookup',color=color,tag=tag,count=count,match=match,registers={'ECX':tag,'EDI':color})
 for index in (-1,0,1,127):add(0x228220,'activation',index=index,stack_values={4:index&0xffffffff})
 for code in range(24):add(0x18b160,'radius',code=code,registers={'EAX':code})
 for query,category,cluster,match in itertools.product((-1,0,1,13,14,15,16,19,20,21),(0,1),(-1,0,1),(0,1)):
  add(0x18ae80,'category',query=query,category=category,cluster=cluster,match=match,
   registers={'ECX':0,'EDX':query&0xffffffff,'EDI':category})
 for entry in (0x18add0,0x18af20,0x18b0c0):
  for category,cluster,match in itertools.product((0,1),(-1,0,1,4),(0,1)):
   add(entry,'attribute',category=category,cluster=cluster,match=match,registers={'EAX':category,'ECX':0,'EDX':category},
    stack_values={4:ENV_META if entry==0x18add0 else 0,8:cluster&0xffffffff if entry!=0x18af20 else 0,12:cluster&0xffffffff} if entry!=0x18af20 else {4:cluster&0xffffffff})
 for enabled,code,cluster,tag,outputs in itertools.product((0,1),(0,13,14,21),(-1,0,1),(-1,0),(0,1)):
  add(0x18b1d0,'contact_select',enabled=enabled,code=code,cluster=cluster,tag=tag,outputs=outputs,registers={'EBX':tag&0xffffffff},
   stack_values={4:enabled,8:OUTPUT,12:0,16:cluster&0xffffffff,20:0,24:code,28:0,
    **{32+4*n:OUTPUT+0x80+4*n if outputs else 0 for n in range(6)}})
 for entry,alias,rounding in itertools.product((0x2e44d0,0x2e4650),('none','left','right'),range(4)):
  add(entry,'compose',alias=alias,mxcsr=0x1f80|(rounding<<13),registers={'ECX':POSE_OUT if alias=='none' else POSE_A if alias=='left' else POSE_B},stack_values={4:POSE_A,8:POSE_B})
 for entry,handle,attached in itertools.product((0x17a4a0,0x17a380,0x17a230,0x17a420),(0xffffffff,0x10000,0x20001,0x10003,0),(0,1)):
  add(entry,'force_retire',handle=handle,attached=attached,registers={'EAX':handle,'ESI':handle},clear=8)
 for handle,attached in itertools.product((0x10000,0x20001),(0,1)):
  add(0x17a4a0,'force_retire',handle=handle,attached=attached,pending=True,registers={'EAX':handle},clear=8)
 for flags,groups in itertools.product((0,32),(0,1,3)):
  add(0x17a030,'force_tag',flags=flags,groups=groups,stack_values={4:FORCES})
 for active,occupied,value in itertools.product((0,1),(0,1,5),(-1.,0.,0.5,1.,2.)):
  add(0xc6120,'force_links',active=active,occupied=occupied,value=value,xmm_values={1:struct.unpack('<I',struct.pack('<f',value))[0]},stack_values={4:0,8:UNIT})
 for occupied,mode,point,radius,rounding in itertools.product((0,1,5,15),(0,1),(-1.,0.,100.),(0.,1.,10.),range(4)):
  add(0x1755b0,'proximity',occupied=occupied,mode=mode,point=point,radius=radius,mxcsr=0x1f80|(rounding<<13),
   registers={'EAX':mode,'EDI':OUTPUT},xmm_values={0:struct.unpack('<I',struct.pack('<f',radius))[0]})
 for previous,next_,ends in itertools.product((-1,0),(-1,2),(0,1)):
  add(0x177fa0,'unlink',previous=previous,next=next_,ends=ends,registers={'ECX':ENV_LINKS+0x54,'EBX':OUTPUT},stack_values={4:OUTPUT+4})
 for attached in (0,1):add(0x17a2f0,'retire_links',attached=attached,stack_values={4:FORCES})
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
    starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    new_sites={p for r in paired['tu15_new_owners'] for p in _trace(code_image,r['entry'],starts).code}
    parent_research=json.loads((ROOT/'analysis/multiplayer/sandbox-source/vehicle-constraint-cleanup-tu15-20261011/research-verified/report.json').read_text())
    c1_sites=new_sites|{p for r in parent_research['tu15_new_owners'] for p in _trace(code_image,r['entry'],starts).code}
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
        w(TAGS+8,DEFINITION)
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
                g.mem_write(record,bytes(0xa0))
                for offset in (0x28,0x30,0x34,0x38,0x3c,0x94):w(record+offset,0xffffffff)
                g.mem_write(record+0x9c,bytes.fromhex('ffffffff'))
                g.mem_write(record+0x14,struct.pack('<f',0.5))
                g.mem_write(record+8,struct.pack('<h',row['reference_count']))
                g.mem_write(record+0xd,struct.pack('<b',row['record_kind']))
                g.mem_write(record+0x10,struct.pack('<b',row['subtype']))


        if row.get('cleanup_input'):
            g.mem_write(RECORD,bytes(0xa0))
            for offset in (0x28,0x30,0x34,0x38,0x3c,0x94):w(RECORD+offset,0xffffffff)
            g.mem_write(RECORD+0x9c,bytes.fromhex('ffffffff'))
            g.mem_write(RECORD+8,struct.pack('<h',1))
            g.mem_write(RECORD+0x14,struct.pack('<f',0.5))
            w(COMPONENT+8,UNIT);w(COMPONENT+0x74,1)
            w(COMPONENT+0x7c,ASSOCIATIONS);w(COMPONENT+0x80,0)
            w(COMPONENT+0x88,DESCRIPTORS);w(COMPONENT+0x8c,0)
            for p,n in ((DESCRIPTORS,0x200),(ASSOCIATIONS,0x100),(CONTAINER,0x40),(META1,0x40),(META2,0x40),(ATTR1,0x40),(ATTR2,0x40)):
                g.mem_write(p,rng.randbytes(n))
            kind=row['cleanup_input']
            if kind=='lookup':
                g.mem_write(RECORD+0x9c,struct.pack('<hh',row['node'],5))
                g.mem_write(RECORD+0xd,struct.pack('<b',row['record_kind']))
                g.mem_write(COMPONENT+0x18,struct.pack('<b',row['slot']))
                w(COMPONENT+0x74,row['body_count']);w(COMPONENT+0x80,2);w(COMPONENT+0x8c,3)
                for n in range(2):
                    g.mem_write(ASSOCIATIONS+n*12,struct.pack('<hhII',3 if n==1 else 9,5,0,CONTAINER))
                w(CONTAINER+0xc,META1);w(CONTAINER+0x10,META2)
                for meta,attrs in ((META1,ATTR1),(META2,ATTR2)):
                    w(meta+0x30,attrs);w(meta+0x34,0 if row['attributes']==3 else 0xffffffff if row['attributes']==4 else 3)
                    for n in range(3):
                        key=0x2001 if row['attributes']==2 and n==0 else 0x2002 if n==row['attributes'] else 0x1000+n
                        w(attrs+n*8,key);w(attrs+n*8+4,0xffffffff if key==0x2001 else n+1)
                for n,(a,b) in enumerate(((127,5),(-1,2),(0,-128))):
                    w(DESCRIPTORS+n*0x48+8,0 if n!=1 else 7);w(DESCRIPTORS+n*0x48+0x14,0 if n!=2 else 0xffffffff)
                    g.mem_write(DESCRIPTORS+n*0x48+0x45,struct.pack('<bb',a,b))
            elif kind=='threshold':
                g.mem_write(RECORD+0x11,bytes((row['latched'],)))
                w(RECORD+0x18,row['threshold_bits']);w(RECORD+0x94,row['last']);w(RECORD+0x98,0x3f800000)
            elif kind=='cleanup':
                g.mem_write(RECORD+0xc,struct.pack('<b',row['contact_type']))
                g.mem_write(RECORD+0xd,bytes((255,)))
                for offset in (0x1c,0x1e,0x20):g.mem_write(RECORD+offset,struct.pack('<b',row['timer']))
                g.mem_write(RECORD+0x1d,bytes((255,)));g.mem_write(RECORD+0x91,bytes((3,)))
                g.mem_write(0x48b892,bytes((row['enabled'],)))
            elif kind=='basis':g.mem_write(RECORD+0x5c,struct.pack('<3f',*row['basis_vector']))
            elif kind=='pack':g.mem_write(OUTPUT,struct.pack('<3I',*row['vector_bits']))
            elif kind in ('force','force_stop','constraint_stop','cleanup_force'):
                for p,n in ((FORCE_POOL,0x80),(FORCES,0x600),(FORCE_DEFINITION,0x200),(BLOCK_POOL,0x80),(BLOCKS,0x100)):
                    g.mem_write(p,bytes(n))
                w(0x4f720c,FORCE_POOL);w(FORCE_POOL+0x38,3);w(FORCE_POOL+0x24,0x190);w(FORCE_POOL+0x44,FORCES)
                w(TAGS+24,FORCE_DEFINITION);w(FORCE_DEFINITION+0xc,row['groups']);w(FORCE_DEFINITION,0)
                for n in range(3):w(FORCES+n*0x190+0xfc,0xffffffff);w(FORCES+n*0x190+0x100,0xffffffff);w(FORCES+n*0x190+0x48,UNIT)
                w(FORCE_POOL+0x48,ENV_BITS);w(FORCE_POOL+0x3c,3);w(ENV_BITS,7)
                for n in range(3):
                    g.mem_write(FORCES+n*0x190,struct.pack('<H',n+1));w(FORCES+n*0x190+4,1)
                    w(FORCES+n*0x190+0x7c,0);w(FORCES+n*0x190+0x80,2);w(FORCES+n*0x190+8,0xffffffff)
                w(0x4f7208,BLOCK_POOL);w(BLOCK_POOL+0x44,BLOCKS)
                for n in range(3):w(BLOCKS+n*0x3c+4,1 if n==0 else 0xffffffff)
                g.mem_write(MATRIX,struct.pack('<13f',1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,3.,4.,5.))
                g.mem_write(OUTPUT,struct.pack('<3f',1.,-2.,3.));g.mem_write(VECTOR2,struct.pack('<2f',0.25,-0.5))
                if kind=='cleanup_force':
                    w(RECORD+0x28,row['secondary'])
                    g.mem_write(RECORD+0x9c,struct.pack('<h',row['node']))
                    g.mem_write(RECORD+0x5c,struct.pack('<3f',1.,0.,0.));g.mem_write(RECORD+0x68,struct.pack('<3f',2.,-1.,0.5))
                    w(RECORD+0x38,0x10000 if row['mask']&1 else 0xffffffff)
                    w(RECORD+0x3c,0x20001 if row['mask']&2 else 0xffffffff)
                    g.mem_write(MOTION+0x80,struct.pack('<16f',1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,3.,4.,5.,1.))
                    w(COMPONENT+4,0)
                if kind=='constraint_stop':
                    for n,offset in enumerate((0x30,0x34,0x38,0x3c)):w(RECORD+offset,row['handle'] if row['mask']&(1<<n) else 0xffffffff)

            elif kind=='flag':
                w(0x4f9b5c,FORCE_POOL);w(FORCE_POOL+0x44,FORCES)
                g.mem_write(FORCES+row['index']*24+4,bytes((row['initial'],)))
            elif kind in ('allocate','create'):
                g.mem_write(FORCE_POOL,bytes(0x80));g.mem_write(FORCES,rng.randbytes(0x400));g.mem_write(BLOCKS,bytes(0x40))
                start,high,capacity,occupied=row['allocation_state']
                for offset,value in ((0x20,capacity),(0x24,row['stride']),(0x34,start),(0x38,high),(0x3c,2),(0x44,FORCES),(0x48,BLOCKS)):
                    w(FORCE_POOL+offset,value)
                g.mem_write(FORCE_POOL+0x40,struct.pack('<H',row['generation']))
                g.mem_write(BLOCKS,occupied.to_bytes(8,'little'));w(0x4f9b5c,FORCE_POOL)
            elif kind in ('activation','activation_record','color_lookup'):
                g.mem_write(BASE+0x58000,bytes(0x400));g.mem_write(BASE+0x59000,bytes(0x400))
                w(0x4ecc20,BASE+0x58000);w(0x4ecc1c,BASE+0x59000)
                if kind=='activation':
                    g.mem_write(OUTPUT,struct.pack('<9f',1.,0.,0.,0.,1.,0.,0.,0.,1.))
                    g.mem_write(OUTPUT+0x80,struct.pack('<IIf',row['color'],row['tag']&0xffffffff,0.5))
                elif kind=='activation_record':
                    g.mem_write(RECORD+3,bytes((row['record_flags'],)))
                    w(RECORD+0x44,row['attachment']);w(RECORD+0x48,row['attachment']);w(RECORD+0x4c,row['owner'])
            elif kind=='predicate':
                for n,offset in enumerate((0x30,0x34,0x38,0x3c)):w(RECORD+offset,0 if row['mask']&(1<<n) else 0xffffffff)
        # Declared engine environment, tag registry and empty source contexts.
        for p,n in ((ENV_REGISTRY,0x100),(ENV_META,0x100),(ENV_CONTEXT,0x200),(ENV_TREE,0x40),
                    (ENV_NODES,0x40),(ENV_PLANES,0x40),(ENV_WORLD,0x100),(ENV_LEAVES,0x40),
                    (ENV_CLUSTERS,0x200),(ENV_PORTALS,0x40),(ENV_TAG,0x40),(ENV_ATTRIBUTES,0x800),
                    (ENV_LINKS,0x200),(ENV_FLAGS,0x100),(ENV_ITER_POOL,0x80),(ENV_ITER_RECORDS,0x2000),(ENV_BITS,0x40)):
            g.mem_write(p,bytes(n))
        w(0x4f0414,TAGS);w(TAGS+0xe000*16+8,ENV_META)
        w(ENV_REGISTRY+8,ENV_META);w(ENV_REGISTRY+24,FORCE_DEFINITION)
        w(ENV_META+4,ENV_ATTRIBUTES);w(ENV_META+0x58,0xffffffff)
        w(0x4ecc0c,ENV_TREE);w(ENV_TREE+4,ENV_NODES);w(ENV_TREE+0xc,ENV_PLANES)
        g.mem_write(ENV_NODES,b'\x00\x00'+(0x800000).to_bytes(3,'little')+(0x800001).to_bytes(3,'little'))
        g.mem_write(ENV_PLANES,struct.pack('<4f',1.,0.,0.,0.))
        w(0x4ecc18,ENV_WORLD);w(ENV_WORLD+0x30,ENV_LEAVES);w(ENV_WORLD+0x68,ENV_PORTALS);w(ENV_WORLD+0xa0,ENV_CLUSTERS)
        g.mem_write(ENV_LEAVES,struct.pack('<h',0));g.mem_write(ENV_LEAVES+8,struct.pack('<h',1))
        g.mem_write(ENV_CLUSTERS+0x70,b'\xff');g.mem_write(ENV_CLUSTERS+0xb0+0x70,b'\xff')
        g.mem_write(0x474c24,struct.pack('<h',0))
        w(0x4ecc1c,ENV_CONTEXT);w(ENV_CONTEXT+0xc0,1);w(ENV_CONTEXT+0xc4,ENV_TAG);w(ENV_TAG+4,0xe000)
        w(ENV_CONTEXT+0x150,2);w(ENV_CONTEXT+0x154,ENV_CLUSTERS)
        for n in range(2):
            g.mem_write(ENV_CLUSTERS+n*0xb4+8,struct.pack('<h',-1));w(ENV_CLUSTERS+n*0xb4+0xb0,0xffffffff)
        w(0x4f54f0,ENV_FLAGS)
        for n in range(4):w(ENV_FLAGS+0xc+4*n,0xffffffff)
        w(0x4ecbec,ENV_ITER_POOL)
        for offset,value in ((0x24,0x110),(0x38,0),(0x44,ENV_ITER_RECORDS),(0x48,ENV_BITS+0x20)):w(ENV_ITER_POOL+offset,value)
        w(0x51df74,ENV_FLAGS+0x40);w(ENV_FLAGS+0x40+0x24,0x54);w(ENV_FLAGS+0x40+0x44,ENV_LINKS)
        for n in range(3):
            g.mem_write(ENV_LINKS+n*0x54,struct.pack('<H',n+1))
            for offset in (0x38,0x3c,0x40,0x44):w(ENV_LINKS+n*0x54+offset,0xffffffff)
        w(0x4f9b5c,ENV_ITER_POOL);w(ENV_ITER_POOL+0x44,ENV_ITER_RECORDS)
        w(ENV_BITS,7);w(ENV_BITS+0x10,7)
        for offset,value in ((0x24,0x3c),(0x34,0),(0x38,3),(0x3c,3),(0x48,ENV_BITS+0x10)):w(BLOCK_POOL+offset,value)
        # The retained object pool is the same source pool used by the parent.
        w(POOL+0x24,12);w(POOL+0x38,2);g.mem_write(HEADERS,struct.pack('<H',UNIT>>16))
        g.mem_write(BODY+0x11c,struct.pack('<hh',0,0x600))
        if row.get('cleanup_input') in ('activation','activation_record','color_lookup'):w(TAGS+8,ENV_META)
        if row.get('environment_input'):
            kind=row['environment_input']
            w(0x4f0414,ENV_REGISTRY);w(ENV_TAG+4,0)
            g.mem_write(RECORD,bytes(0xa0));g.mem_write(OUTPUT,bytes(0x100))
            if kind=='plane_query':
                g.mem_write(0x474c24,struct.pack('<h',row['active']))
                value=row['leaf']&0xffffff if row['leaf']==-1 else 0x800000|row['leaf']
                g.mem_write(ENV_NODES+2,value.to_bytes(3,'little'))
                g.mem_write(OUTPUT,struct.pack('<3f',row['point'],0.,0.))
            elif kind=='portal_query':
                g.mem_write(RECORD+4,struct.pack('<h',row['region']))
                g.mem_write(ENV_CLUSTERS+0x70,bytes((row['code'],)))
                g.mem_write(ENV_PORTALS+2,struct.pack('<h',row['target']))
                g.mem_write(ENV_PORTALS+4,struct.pack('<4f',1.,0.,0.,0.))
                g.mem_write(OUTPUT,struct.pack('<3f',row['point'],0.,0.))
            elif kind in ('iterate','retire'):
                w(ENV_ITER_POOL+0x38,row['high']);w(ENV_ITER_POOL+0x48,ENV_BITS);w(ENV_ITER_POOL+0x44,ENV_ITER_RECORDS)
                if kind=='iterate':g.mem_write(ENV_BITS,row['occupied'].to_bytes(8,'little'))
                else:
                    w(ENV_ITER_POOL+0x24,row['stride']);w(ENV_ITER_POOL+0x34,row['cursor']);w(ENV_ITER_POOL+0x3c,3)
                    g.mem_write(ENV_ITER_POOL+0x2a,bytes((row['clear'],)))
                    w(ENV_BITS,7)
                    for n in range(3):g.mem_write(ENV_ITER_RECORDS+n*row['stride'],rng.randbytes(row['stride']));g.mem_write(ENV_ITER_RECORDS+n*row['stride'],struct.pack('<H',n+1))
            elif kind=='lookup':
                w(ENV_META,row['count'])
                for n in range(3):w(ENV_ATTRIBUTES+n*0x38,row['color'] if n==row['match'] else 0xf00d0000+n)
            elif kind=='activation':
                w(ENV_META+4,BASE+0x80000);g.mem_write(BASE+0x80000-0x5c,bytes(0x3000))
            elif kind in ('category','attribute','contact_select'):
                w(ENV_META,22);w(ENV_META+4,ENV_ATTRIBUTES)
                category=row.get('category',0);cluster=row['cluster'];match=row.get('match',1)
                for n in range(22):
                    for offset in (8,16):w(ENV_ATTRIBUTES+n*24+offset,1 if match and (n%3==0 or n==14) else 0)
                    for offset in (12,20):w(ENV_ATTRIBUTES+n*24+offset,ENV_ATTRIBUTES+0x400)
                for n in range(2):
                    g.mem_write(ENV_CLUSTERS+n*0xb4+8,struct.pack('<h',1 if n==0 else -1))
                    w(ENV_CLUSTERS+n*0xb4+0xb0,0)
                    for offset in (0x3c,0x54,0x74,0x8c,0x94):w(ENV_CLUSTERS+n*0xb4+offset,0x12340000+offset+n)
                a=ENV_ATTRIBUTES+0x400;w(a+4,0x1234);w(a+12,0x5678)
                g.mem_write(a+0x14,struct.pack('<h',cluster if match and cluster in (0,1) else 7));g.mem_write(a+0x16,b'\x01')
                if kind=='attribute':
                    row_registers=dict(row_registers)
                    if row['entry']==0x18add0:row['stack_values']={4:ENV_ATTRIBUTES,8:cluster&0xffffffff}
                    elif row['entry']==0x18af20:row_registers['EDX']=0
                    else:row['stack_values']={4:cluster&0xffffffff,8:0,12:cluster&0xffffffff}
            elif kind=='compose':
                a=(1.,0.5,0.,0.,0.,1.,0.25,0.,0.,0.,1.,0.,3.,4.,5.,1.)
                b=(0.,1.,0.,0.,-1.,0.,0.,0.,0.,0.,1.,0.,-2.,1.,0.5,1.)
                g.mem_write(POSE_A,struct.pack('<16f',*a));g.mem_write(POSE_B,struct.pack('<16f',*b));g.mem_write(POSE_OUT,struct.pack('<16f',*a))
                if row['entry']==0x2e4650:row['stack_values']={4:POSE_B}
            elif kind in ('force_retire','force_tag','retire_links'):
                g.mem_write(FORCE_POOL,bytes(0x80));g.mem_write(FORCES,bytes(0x600));g.mem_write(FORCE_DEFINITION,bytes(0x200))
                w(0x4f720c,FORCE_POOL);w(0x4f7208,BLOCK_POOL);w(BLOCK_POOL+0x44,BLOCKS)
                w(FORCE_POOL+0x24,0x190);w(FORCE_POOL+0x38,3);w(FORCE_POOL+0x3c,3);w(FORCE_POOL+0x44,FORCES);w(FORCE_POOL+0x48,ENV_BITS)
                for n in range(3):
                    p=FORCES+n*0x190;g.mem_write(p,struct.pack('<H',n+1));w(p+4,1);w(p+8,0xffffffff);w(p+0x48,UNIT)
                    w(p+0x7c,0xffffffff);w(p+0xfc,0xffffffff);w(p+0x100,0xffffffff)
                if row.get('attached'):
                    w(FORCE_DEFINITION+0xc,1);w(FORCES+0x7c,0x10000);w(FORCES+8,0)
                    w(0x4f9b5c,ENV_ITER_POOL);w(ENV_ITER_POOL+0x44,ENV_ITER_RECORDS)
                    g.mem_write(BLOCKS,struct.pack('<H',1));w(BLOCKS+4,0xffffffff)
                    g.mem_write(BODY+0x11c,struct.pack('<hh',24,0x600));g.mem_write(BODY+0x600,b'\x02');w(BODY+0x604,0x10000)
                if row.get('pending'):
                    p=FORCES+(row['handle']&0xffff)*0x190;w(p+0xfc,0);w(p+0x100,0x10000)
                    g.mem_write(ENV_LINKS+0xc,struct.pack('<H',0xffff))
                if kind=='force_tag':
                    w(FORCE_DEFINITION,row['flags']);w(FORCE_DEFINITION+0x14,row['groups']);w(FORCE_DEFINITION+0x18,ENV_ATTRIBUTES)
                    for n in range(3):
                        w(ENV_ATTRIBUTES+n*0x38+0x18,2);w(ENV_ATTRIBUTES+n*0x38+0x1c,ENV_ATTRIBUTES+0x200)
                    for n in range(2):w(ENV_ATTRIBUTES+0x200+n*0x38+0xc,0x7464746c);w(ENV_ATTRIBUTES+0x200+n*0x38+0x10,0)
                if kind=='retire_links' and row['attached']:w(FORCES+0x100,0x10000);w(FORCES+0xfc,0x10000)
            elif kind=='force_links':
                g.mem_write(ENV_ITER_POOL+0x29,bytes((row['active'],)));w(ENV_ITER_POOL+0x38,3);w(ENV_ITER_POOL+0x3c,3);w(ENV_BITS+0x20,row['occupied'])
                for n in range(3):
                    p=ENV_ITER_RECORDS+n*0x110;g.mem_write(p,struct.pack('<H',n+1));w(p+8,0);w(p+12,UNIT)
            elif kind=='proximity':
                g.mem_write(OUTPUT,struct.pack('<3f',row['point'],0.,0.))
                for n in range(4):
                    w(ENV_FLAGS+0xc+4*n,n if row['occupied']&(1<<n) else 0xffffffff)
                    g.mem_write(0x4f655c+n*0x358,struct.pack('<3f',float(n),0.,0.));w(0x4f655c+n*0x358+0x50,0x3f800000)
            elif kind=='unlink':
                w(0x51df74,ENV_ITER_POOL);w(ENV_ITER_POOL+0x44,ENV_LINKS)
                for n in range(3):
                    p=ENV_LINKS+n*0x54;g.mem_write(p,struct.pack('<H',n+1));w(p+0x38,0xffffffff);w(p+0x3c,0xffffffff)
                p=ENV_LINKS+0x54;w(p+0x38,0xffffffff if row['previous']==-1 else 0x10000);w(p+0x3c,0xffffffff if row['next']==-1 else 0x30002)
                w(OUTPUT,0x20001 if row['ends'] else 0xffffffff);w(OUTPUT+4,0x20001 if row['ends'] else 0xffffffff)
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
        for n,value in row.get('xmm_values',{}).items():
            register=getattr(R,f'UC_X86_REG_XMM{n}');g.reg_write(register,(g.reg_read(register)&~0xffffffff)|value)
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
            elif site in c1_sites and mnemonic in ('fld','fistp'):status_mask=0x3a3f
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
 assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_vehicle_environment_state_tu15_20261011.py')
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
  new_environment_case_kinds=dict(__import__('collections').Counter(r['environment_input'] for r in rows if r.get('environment_input'))),
  instructions=sum(len(c['instructions']) for c in cases),ordered_writes=sum(len(c['writes']) for c in cases),
  full_state_bytes=sum(len(bytes.fromhex(b['hex'])) for c in cases for b in c['final_banks']),
  covered_instructions=len(seen),new_owner_covered_instructions=sum(p in seen for r in paired['tu15_new_owners'] for p in (int(line.split('\t')[0],16) for line in (a.correspondence.parent/r['listing_file']).read_text().splitlines())),
  body_coverage={f"{r['entry']:08x}":sum(int(line.split('\t')[0],16) in seen for line in (a.correspondence.parent/r['listing_file']).read_text().splitlines()) for r in paired['tu15_new_owners']},
  stop_entries=sorted({c['exit_entry'] for c in cases if not c['complete']}),
  native_expected_data_used=False,instruction_patches=[],boundary=__doc__)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
