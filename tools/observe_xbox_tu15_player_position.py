#!/usr/bin/env python3
"""Observe actual object/camera positions for normal controller navigation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
from capture_xbox_tu15_grenade_reference import qmp,TEXT_SHA,sha,write_json

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('session','output'):parser.add_argument('--'+name,type=Path,required=True)
    for name in ('unit','target'):parser.add_argument('--'+name,type=lambda x:int(x,0),required=True)
    args=parser.parse_args();out=args.output.absolute();out.mkdir(exist_ok=False)
    launch=json.loads((args.session/'launch.json').read_text());qp=Path(launch['qmp']);qmp(qp,[dict(execute='stop')]);raw=[]
    def read(address,size,retain=True):
        fd=os.memfd_create('original-position',os.MFD_CLOEXEC)
        try:
            qmp(qp,[dict(execute='memsave',arguments=dict(val=address,size=size,filename=f'/proc/{os.getpid()}/fd/{fd}'))])
            data=os.pread(fd,size,0);assert len(data)==size
        finally:os.close(fd)
        if retain:raw.append(dict(address=address,hex=data.hex()))
        return data
    def word(address):return struct.unpack('<I',read(address,4))[0]
    assert hashlib.sha256(read(0x12000,3611948,False)).hexdigest()==TEXT_SHA
    headers=word(word(0x4ecbd0)+0x44);objects=[]
    for identity in (args.unit,args.target):
        header=read(headers+(identity&65535)*12,12);assert struct.unpack_from('<H',header)[0]==identity>>16
        body=struct.unpack_from('<I',header,8)[0];data=read(body,0x118)
        objects.append(dict(identity=identity,body=body,definition=struct.unpack_from('<I',data)[0],type=data[0xaa],
            parent=struct.unpack_from('<I',data,0x14)[0],position=list(struct.unpack_from('<3f',data,0x64)),
            forward=list(struct.unpack_from('<3f',data,0x70)),health=struct.unpack_from('<f',data,0xec)[0],shield=struct.unpack_from('<f',data,0xf0)[0]))
    root_forward=list(struct.unpack('<3f',read(0x4c65cc,12)))
    # The original171D90 observer layout, already used by the independent
    # vehicle camera comparison. The FP draw root is a different coordinate frame.
    observer=read(0x4f64a4,0xf0);camera=list(struct.unpack_from('<3f',observer,0xd8));clock=word(0x51df54)
    write_json(out/'report.json',dict(objects=objects,camera_forward=camera,first_person_root_forward=root_forward,
        observer_position=list(struct.unpack_from('<3f',observer,0xb8)),tick=word(clock+8),raw_observations=raw,
        full_live_text_sha256=TEXT_SHA,tool_sha256=sha(Path(__file__)),guest_writes=[],physical_xbox_verified=False,full_weapon_parity=False))
    shutil.copyfile(Path(__file__),out/'observer-source.py')
    print(json.dumps(dict(objects=objects,camera_forward=camera)),flush=True)

if __name__=='__main__':main()
