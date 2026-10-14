#!/usr/bin/env python3
"""Create a declared reference weapon through original B9960/B9B70.

This is an original-engine API fixture, not a normal-play provenance claim.
The original executable and player pose are unchanged. Only a temporary caller
stack and placement arguments are supplied; the original constructors create
the object. Normal controller pickup then selects its authored holding state.
A private VM snapshot is retained before the experiment and restored on error.
"""
import argparse
import json
from pathlib import Path
import subprocess

from capture_xbox_tu15_grenade_reference import ROOT, TEXT_SHA, qmp


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--unit', type=lambda x: int(x, 0), required=True)
    parser.add_argument('--weapon', type=lambda x: int(x, 0), required=True)
    parser.add_argument('--forward-offset', type=float, default=.15)
    parser.add_argument('--height-offset', type=float, default=.05)
    args = parser.parse_args()
    session, out = args.session.resolve(), args.output.resolve()
    assert ROOT in session.parents and ROOT in out.parents and not out.exists()
    assert 0 < args.unit < 0xffffffff and 0 < args.weapon < 0xffffffff
    assert 0 < args.forward_offset < 1 and 0 < args.height_offset < 1
    assert out.name.replace('-', '').replace('_', '').isalnum()
    out.mkdir(parents=True)
    launch = json.loads((session/'launch.json').read_text())
    socket = Path(launch['qmp'])
    snapshot = 'pose-api-before-'+out.name
    saved = qmp(socket, [{'execute': 'stop'}, {'execute': 'human-monitor-command',
                    'arguments': {'command-line': 'savevm '+snapshot}}])
    assert not saved[2]['return'].strip(), saved
    script = out/'create.gdb'
    script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 15
target remote 127.0.0.1:{launch['gdb_port']}
hbreak *0x13abd3
continue
python
import gdb,struct,hashlib,json,pathlib,re,sys
sys.path.insert(0,{str(ROOT/'tools')!r})
from capture_xbox_tu15_grenade_reference import qmp
out=pathlib.Path({str(out)!r});memory=gdb.selected_inferior()
def read(p,n):return bytes(memory.read_memory(p,n))
def word(p):return struct.unpack('<I',read(p,4))[0]
def reg(n):return int(gdb.parse_and_eval('$'+n))&0xffffffff
def packet():
 value=gdb.execute('maintenance packet g',to_string=True)
 match=re.search(r'received: "([0-9a-fA-F]+)"',value);assert match,value
 return match.group(1)
def restore(value):
 result=gdb.execute('maintenance packet G'+value,to_string=True)
 assert 'received: "OK"'in result,result
text=read(0x12000,3611948);assert hashlib.sha256(text).hexdigest()=={TEXT_SHA!r}
unit={args.unit};weapon={args.weapon};headers=word(word(0x4ecbd0)+0x44)
header=read(headers+(unit&65535)*12,12);assert struct.unpack_from('<H',header)[0]==unit>>16
body=struct.unpack_from('<I',header,8)[0];assert read(body+0xaa,1)==b'\\0'
tags=word(0x4f0414);tag=tags+(weapon&65535)*16
assert word(tag)==0x77656170 and word(tag+4)==weapon and word(tag+8)
clock=word(0x51df54);assert read(clock,1)==b'\\1'
tick=word(clock+8);pc=reg('eip');sp=reg('esp');assert pc==0x13abd3
saved_registers=packet();scratch=sp-0x1000;old=read(scratch,0x1000)
caller=scratch+0x200;placement=scratch+0x400
size,offset=struct.unpack('<hh',read(body+0x114,4));assert size>0 and size%52==0
player_pose=read(body+offset,size)
gdb.execute('delete 1')
def invoke(entry,arguments,registers=None):
 restore(saved_registers)
 memory.write_memory(caller,struct.pack('<'+'I'*(len(arguments)+1),pc,*arguments))
 gdb.execute('set $esp='+hex(caller));gdb.execute('set $eip='+hex(entry))
 for name,value in (registers or {{}}).items():gdb.execute('set $'+name+'='+hex(value))
 stop=gdb.Breakpoint('*'+hex(pc),gdb.BP_HARDWARE_BREAKPOINT,internal=True)
 gdb.execute('continue');stop.delete();assert reg('eip')==pc
 return reg('eax')
try:
 invoke(0xb9960,[weapon,0xffffffff,0],{{'esi':placement}})
 position=struct.unpack('<3f',read(body+0x64,12));forward=struct.unpack('<3f',read(body+0x70,12))
 point=[position[i]+forward[i]*{args.forward_offset!r} for i in range(3)];point[2]+={args.height_offset!r}
 memory.write_memory(placement+0x1c,struct.pack('<3f',*point))
 memory.write_memory(placement+0x28,struct.pack('<3f',*forward))
 arguments=read(placement,0xc4);(out/'placement.bin').write_bytes(arguments)
 created=invoke(0xb9b70,[placement]);assert created!=0xffffffff
 created_header=read(headers+(created&65535)*12,12)
 assert struct.unpack_from('<H',created_header)[0]==created>>16
 created_body=struct.unpack_from('<I',created_header,8)[0]
 assert word(created_body)==weapon and read(created_body+0xaa,1)==b'\\2'
 assert read(body+offset,size)==player_pose,'Original constructor unexpectedly changed player pose'
 assert word(clock+8)==tick,'Original constructor unexpectedly advanced the game tick'
 assert hashlib.sha256(read(0x12000,3611948)).hexdigest()=={TEXT_SHA!r}
 result=dict(reference='OG Xbox TU1.5',provenance='Declared original-engine object-creation API fixture',
  constructors=[0xb9960,0xb9b70],unit=unit,definition=weapon,created=created,position=point,
  original_tick=tick,original_player_pose_preserved=True,original_text_preserved=True,
  guest_code_patches=[],guest_pose_writes=[],guest_api_inputs=['Temporary caller stack','Original placement constructor','Declared creation position and forward'],
  snapshot={snapshot!r},pixel_acceptance=False)
 (out/'receipt.json').write_text(json.dumps(result,indent=2)+'\\n')
finally:
 memory.write_memory(scratch,old);restore(saved_registers)
 qmp(pathlib.Path({str(socket)!r}),[{{'execute':'stop'}}])
end
detach
quit
''')
    try:
        with (out/'create.log').open('w') as log:
            subprocess.run(['gdb', '-q', '-nx', '-batch', '-x', str(script)],
                           stdout=log, stderr=subprocess.STDOUT, timeout=45, check=True)
    except Exception:
        recovery = qmp(socket, [{'execute': 'stop'}, {'execute': 'human-monitor-command',
                                 'arguments': {'command-line': 'loadvm '+snapshot}},
                                {'execute': 'stop'}])
        (out/'recovery.json').write_text(json.dumps(recovery, indent=2)+'\n')
        raise
    finally:
        qmp(socket, [{'execute': 'stop'}])
    print((out/'receipt.json').read_text())


if __name__ == '__main__':
    main()
