"""GDB-side read-only capture of one naturally reached TU1.5 first-person call."""
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import gdb
from capture_xbox_tu15_grenade_reference import qmp, TEXT_SHA

config=json.loads(Path(os.environ['HALO2_FP_CASE_CONFIG']).read_text())
out=Path(config['output']);inferior=gdb.selected_inferior();qmp_path=Path(config['qmp'])

def read(address,size):return bytes(inferior.read_memory(address,size))
def word(address):return struct.unpack('<I',read(address,4))[0]
def reg(name):return int(gdb.parse_and_eval('$'+name))&0xffffffff

def stop_at(pc,condition=None):
    point=gdb.Breakpoint('*'+hex(pc),gdb.BP_HARDWARE_BREAKPOINT,internal=True)
    if condition:point.condition=condition
    try:gdb.execute('continue')
    finally:point.delete()
    assert reg('eip')==pc

def cpu():
    monitor=qmp(qmp_path,[dict(execute='human-monitor-command',arguments={'command-line':'info registers'})])[-1]['return']
    fs=re.search(r'^FS\s*=\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)',monitor,re.M);assert fs,monitor
    control={name:int(value,16)for name,value in re.findall(r'\b(CR[0234])=([0-9a-fA-F]+)',monitor)}
    assert set(control)=={'CR0','CR2','CR3','CR4'}
    frame=gdb.selected_frame()
    gp=[reg(n)for n in ('eax','ecx','edx','ebx','esp','ebp','esi','edi')]
    xmm=[list(struct.unpack('<4I',frame.read_register('xmm'+str(i)).bytes))for i in range(8)]
    x87=[frame.read_register('st'+str(i)).bytes.hex()for i in range(8)]
    assert all(len(v)==20 for v in x87)
    return dict(eip=reg('eip'),gp=gp,eflags=reg('eflags'),xmm=xmm,
        x87_control=reg('fctrl'),x87_status=reg('fstat'),x87_tag=reg('ftag'),x87_tag_is_debugger_placeholder=True,x87_register_bytes=x87,
        mxcsr=reg('mxcsr'),fs_selector=reg('fs'),fs_base=int(fs.group(1),16),control_registers=control,
        monitor_registers=monitor)

def dump(label):
    state=cpu();state['tick']=word(word(0x51df54)+8)
    state['text_sha256']=hashlib.sha256(read(0x12000,3611948)).hexdigest();assert state['text_sha256']==TEXT_SHA
    (out/(label+'-cpu.json')).write_text(json.dumps(state,indent=2)+'\n')
    qmp(qmp_path,[dict(execute='pmemsave',arguments=dict(val=0,size=config['ram_bytes'],filename=str(out/(label+'-physical.bin'))))])
    return state

condition='*(unsigned*)($esp+4)==0 && *(unsigned*)($esp+8)=='+hex(config['unit'])
assert hashlib.sha256(read(0x12000,3611948)).hexdigest()==TEXT_SHA
for _ in range(config['warm_views']):
    stop_at(0x168ded,condition);stop_at(word(reg('esp')))
normal_inputs=[];firing_event=None
weapon_input=config.get('weapon_input','none');observed_objects=[]
if config.get('phase')=='weapon':
    headers=word(word(0x4ecbd0)+0x44)
    for name,identity in (('unit',config['unit']),('weapon',config['weapon'])):
        header=read(headers+(identity&65535)*12,12)
        assert struct.unpack_from('<H',header)[0]==identity>>16
        body=struct.unpack_from('<I',header,8)[0];extent=struct.unpack_from('<H',header,6)[0]
        assert body and 0x118<=extent<=65535
        observed_objects.append(dict(name=name,identity=identity,address=body,bytes=extent,definition=word(body)))
    weapon_body=observed_objects[1]['address']
    assert word(weapon_body)==config['definition']and word(weapon_body+0x14)==config['unit']
    assert read(weapon_body+0xaa,1)==b'\x02'
    condition='*(unsigned*)($esp+4)=='+hex(config['weapon'])
    if weapon_input=='charge-start':condition+=' && *(unsigned char*)'+hex(weapon_body+0x170)+'!=0 && *(unsigned char*)'+hex(weapon_body+0x175)+'==0'
    elif weapon_input=='primary-fire':condition+=' && *(unsigned char*)'+hex(weapon_body+0x170)+'!=0'
    elif weapon_input=='full-charge':condition+=' && *(unsigned char*)'+hex(weapon_body+0x20c)+'==2'
    else:assert weapon_input=='none'
    if weapon_input!='none':
        charge_events=[]
        if weapon_input=='full-charge':
            description=gdb.execute('watch -location *(unsigned char*)'+hex(weapon_body+0x20c),to_string=True)
            assert 'Hardware watchpoint'in description,description
            charge_events.append(dict(mode=read(weapon_body+0x20c,1)[0],tick=word(word(0x51df54)+8),initial=True))
        normal_inputs=[dict(binding='right trigger',host_key='e',selection=weapon_input)]
        (out/'NORMAL_FIRE_READY').write_text('Original weapon update hardware breakpoint selection prepared\n')
        if weapon_input=='full-charge':
            while read(weapon_body+0x20c,1)!=b'\x02':
                assert len(charge_events)<16
                gdb.execute('continue')
                charge_events.append(dict(mode=read(weapon_body+0x20c,1)[0],tick=word(word(0x51df54)+8),following_pc=reg('eip'),trigger_hex=read(weapon_body+0x20c,12).hex()))
            gdb.execute('delete')
            (out/'charge-transition-observations.json').write_text(json.dumps(dict(events=charge_events,hardware_watchpoint=description),indent=2)+'\n')
if config.get('normal_fire'):
    weapon=config['weapon'];pool=word(0x4ecbd0);headers=word(pool+0x44)
    header=read(headers+(weapon&65535)*12,12)
    assert struct.unpack_from('<H',header)[0]==weapon>>16
    body=struct.unpack_from('<I',header,8)[0]
    assert word(body)==config['definition'] and word(body+0x14)==config['unit']
    assert read(body+0xaa,1)==b'\x02'
    initial_loaded=struct.unpack('<h',read(body+0x22c,2))[0];assert initial_loaded>0
    description=gdb.execute('watch -location *(unsigned short*)'+hex(body+0x22c),to_string=True)
    assert 'Hardware watchpoint' in description,description
    (out/'NORMAL_FIRE_READY').write_text('Hardware ammunition watchpoint armed; normal controller input may begin\n')
    gdb.execute('continue')
    loaded=struct.unpack('<h',read(body+0x22c,2))[0];assert loaded==initial_loaded-1
    firing_event=dict(weapon=weapon,body=body,definition=word(body),owner=word(body+0x14),
        tick=word(word(0x51df54)+8),following_pc=reg('eip'),initial_loaded=initial_loaded,
        loaded=loaded,reserve=struct.unpack('<h',read(body+0x22a,2))[0],hardware_watchpoint=description)
    gdb.execute('delete')
    normal_inputs=[dict(binding='right trigger',host_key='e',selection='first real ammunition decrement after normal trigger press')]
phase=config.get('phase','models');entry={'models':0x168ded,'update':0x168ca1,'weapon':0x1030f0}[phase]
stop_at(entry,condition if phase in ('models','weapon')else None)
sp=reg('esp');return_address=word(sp)
arguments=list(struct.unpack('<4I',read(sp+4,16)))if phase=='models' else[word(sp+4)]if phase=='weapon' else []
if phase=='models':assert arguments[0]==0 and arguments[1]==config['unit'] and arguments[2]==4
if phase=='weapon'and weapon_input=='full-charge':
    assert read(weapon_body+0x20c,1)==b'\x02'
    proof=dict(source_owner=0x107140,selection_authority='Complete original primary_charge producer: trigger mode2 returns1',
        weapon_body=weapon_body,trigger_address=weapon_body+0x20c,trigger_record_hex=read(weapon_body+0x20c,12).hex(),
        primary_accumulator_byte=read(weapon_body+0x175,1)[0],region_selector_byte=read(weapon_body+0x177,1)[0])
    (out/'charge-state-observation.json').write_text(json.dumps(proof,indent=2)+'\n')
state_bank=word(0x4f6498);assert state_bank
state_bytes=0x8330
(out/'before-first-person-state.bin').write_bytes(read(state_bank,state_bytes))
for obj in observed_objects:
    raw=read(obj['address'],obj['bytes']);(out/('before-'+obj['name']+'-body.bin')).write_bytes(raw)
    obj['before_sha256']=hashlib.sha256(raw).hexdigest()
before=dump('before')
stop_at(return_address)
after=dump('after');count=reg('eax')if phase=='models' else 0
if phase=='models':assert 0<count<=arguments[2]
assert word(0x4f6498)==state_bank
final_state=read(state_bank,state_bytes);(out/'after-first-person-state.bin').write_bytes(final_state)
for obj in observed_objects:
    raw=read(obj['address'],obj['bytes']);(out/('after-'+obj['name']+'-body.bin')).write_bytes(raw)
    obj['after_sha256']=hashlib.sha256(raw).hexdigest()
models=[]
for i in range(count):
    record=arguments[3]+i*0xd0c;model,owner,mode=struct.unpack('<3I',read(record,12))
    metadata=word(word(0x4f0414)+(model&65535)*16+8);nodes=word(metadata+0x48);assert 0<nodes<=64
    raw=read(record+12,nodes*52);name=f'model-{i}-nodes.bin';(out/name).write_bytes(raw)
    models.append(dict(model=model,owner=owner,mode=mode,node_count=nodes,nodes_file=name,nodes_sha256=hashlib.sha256(raw).hexdigest()))
qmp(qmp_path,[dict(execute='stop')])
(out/'call.json').write_text(json.dumps(dict(reference='OG Xbox TU1.5 software in xemu',entry=entry,phase=phase,
    return_address=return_address,entry_stack=sp,arguments=arguments,models=models,ram_bytes=config['ram_bytes'],
    first_person_state=dict(address=state_bank,bytes=state_bytes,after_sha256=hashlib.sha256(final_state).hexdigest()),
    observed_objects=observed_objects,
    tick_before=before['tick'],tick_after=after['tick'],full_live_text_sha256=TEXT_SHA,
    guest_code_patches=[],guest_memory_writes=[],guest_register_writes=[],controller_inputs=normal_inputs,firing_event=firing_event,
    physical_xbox_verified=False,full_weapon_parity=False),indent=2)+'\n')
