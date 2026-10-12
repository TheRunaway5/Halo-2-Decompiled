#!/usr/bin/env python3
"""Replay the original customization child's channel backdrop policy.

The real retained Appearance/default channel headers are frozen input. Child
headers are supplied at the source policy seam: the common v20/v21/v27 entries,
channel3/index4, literal retail wgit flags, and steady animation. 22F916 copies
flag5, and 23029A/1490EC/234F8C then execute unchanged. Final graphics services
are recorded, not rendered. Widget admission is separately executed using a
minimal root/list tree; no actual child runtime capture or GPU parity is claimed.
"""
from pathlib import Path
import hashlib
import json
import struct
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

out = ROOT / 'analysis/frontend/customization'
source = ROOT / 'analysis/frontend/profile-regression/original-model-appearance-channels.json'
state = json.loads(source.read_text())
assert state['mode'] == 2
r = Retail(); g = r.g
cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
g.mem_map(0x82000000, 0x1000000)
for channel in state['channels_in_147AF1_render_order']:
    g.mem_write(int(channel['address'], 16), bytes.fromhex(channel['raw_hex']))
    for screen in channel['screens'].values():
        if screen:
            g.mem_write(int(screen['address'], 16), bytes.fromhex(screen['raw_hex']))
clock = state['ui_clock_ms']
r.w(0x54d5b8, clock)
g.mem_write(0x54d5a8, bytes([2]))
wigl = next(t for t in cache.tags.values() if t['group'] == 'wigl')
_, globals_offset = cache.tag(wigl['datum'], 'wigl')
globals_pointer = r.alloc(cache.data[globals_offset:globals_offset+452])
parent_channel = next(c for c in state['channels_in_147AF1_render_order'] if c['channel'] == '5')
parent_pointer = int(parent_channel['screens']['current']['address'], 16)
child_channel = next(c for c in state['channels_in_147AF1_render_order'] if c['channel'] == '3')
child_channel_pointer = int(child_channel['address'], 16)
child = r.alloc(bytes.fromhex(parent_channel['screens']['current']['raw_hex']))
r.w(child+0x74, 3)
r.w(child+0x78, 4)
# These retail class declarations identify the four actual child vtables.
# Verify their inherited source policy entries before executing each class.
child_vtables = {49: 0x45d328, 50: 0x45d398, 52: 0x45d408, 233: 0x45d328}
for vtable in child_vtables.values():
    assert [r.r(vtable+n*4) for n in (20,21,27)] == [0x23012c,0x230130,0x1473c9]
submissions = []
executed = {}
tracked = (0x22f916, 0x1473c9, 0x23012c, 0x230130, 0x23029a,
           0x1490ec, 0x234f8c, 0x23566a, 0x22e3b4, 0x22f6ab)
def supplied_return(value=0, arguments=0):
    sp = g.reg_read(UC_X86_REG_ESP)
    r.ret(value)
    g.reg_write(UC_X86_REG_ESP, sp+4+arguments)
def hook(_g, address, size, context):
    if address in tracked: executed[address] = executed.get(address, 0)+1
    sp = g.reg_read(UC_X86_REG_ESP)
    if address == 0x148350:
        supplied_return(globals_pointer)
    elif address == 0x147cdb:
        submissions.append(dict(service='tint', packed_argb=f'{r.r(sp+4):08x}'))
        supplied_return(arguments=4)
    elif address == 0x235756:
        submissions.append(dict(service='blur', fade_bits=f'{g.reg_read(UC_X86_REG_XMM0)&0xffffffff:08x}'))
        supplied_return()
    elif address == 0x2353a5:
        submissions.append(dict(service='screen', screen_id=r.r(g.reg_read(UC_X86_REG_EDI)+0x70)))
        supplied_return(arguments=4)
g.hook_add(UC_HOOK_CODE, hook)
def render_channels():
    submissions.clear()
    for c in state['channels_in_147AF1_render_order']:
        r.run(0x234f8c, argv=[0x1234500], registers=[(UC_X86_REG_ECX, int(c['address'],16))])
    return list(submissions)
base = render_channels()
assert base == [dict(service='screen', screen_id=5), dict(service='screen', screen_id=47)]
rows = []
for screen_id in (49, 50, 52, 233):
    tag = next(t for t in cache.tags.values() if t['group']=='wgit' and cache.u16(t['address']-cache.magic+4)==screen_id)
    p = tag['address']-cache.magic
    definition = r.alloc(cache.data[p:p+44])
    r.w(child, child_vtables[screen_id])
    r.w(child+0x70, screen_id)
    r.run(0x22f916, stop=0x22f923, registers=[(UC_X86_REG_ESI, child), (UC_X86_REG_EDI, definition)])
    flag5 = int(g.mem_read(child+0x5f4,1)[0])
    assert flag5 == 1 and cache.u32(p) == 0x25
    r.w(child_channel_pointer+8, child)
    predicate = r.run(0x23029a, registers=[(UC_X86_REG_ESI,child)])&255
    actual = render_channels()
    assert predicate == 0
    assert actual == base+[dict(service='screen',screen_id=screen_id)], actual
    rows.append(dict(screen_id=screen_id, original_vtable=f'{child_vtables[screen_id]:08x}',
                     literal_flags=f'{cache.u32(p):08x}',
                     source_value5f4=flag5, source_23029A=bool(predicate),
                     original_submissions=actual))
    r.w(child_channel_pointer+8,0)
# Check original collector admission, independently of the separate child
# channel: the source parent/list virtual visibility methods do not hide it.
root = r.alloc(bytes.fromhex(parent_channel['screens']['current']['raw_hex']))
list_node = r.alloc(bytes(0x100))
# Actual 2C8B85 constructor writes the edit-list vtable; its v16 is base.
assert r.r(0x45d478+16*4) == 0x22e3b4
r.w(list_node, 0x45d478)
r.w(list_node+4,1)
r.w(list_node+0x10,root)
g.mem_write(list_node+0x6d,bytes([1,1]))
r.w(root+0x14,list_node)
collector = r.alloc(bytes(256*12)); count = r.alloc(bytes(4))
admissions = []
for opened in (False,True):
    r.w(child_channel_pointer+8, child if opened else 0)
    r.w(count,0)
    r.run(0x23566a,argv=[root,collector,count])
    entries = [r.r(collector+i*12) for i in range(r.r(count))]
    assert entries == [list_node,root], entries
    admissions.append(dict(child_channel_occupied=opened, collected_types=[r.r(p+4)for p in entries]))
report = dict(xbe_sha256=PIN, retained_parent_dump=str(source.relative_to(ROOT)),
              retained_parent_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
              original_mode=2, original_base_submissions=base, child_cases=rows,
              list_admission_cases=admissions,
              executed={f'{a:08x}':n for a,n in sorted(executed.items())},
              supplied_seams=['child policy header and common original virtual entries',
                              'authored globals query', 'final tint/blur/widget graphics services',
                              'minimal widget tree for independent collector admission'],
              boundary='Does not establish child runtime widget memory, global world color pass, or framebuffer pixel equality.')
path = out/'appearance-composition-original.json'
path.write_text(json.dumps(report,indent=2)+'\n')
print(f'Original child composition: {len(rows)} channel cases, {len(admissions)} list admission cases passed')
print(f'Fixture SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}')
