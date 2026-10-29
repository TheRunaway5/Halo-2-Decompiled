#!/usr/bin/env python3
"""Export exact collector/profile operands and cold constructor destinations."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_input_collection_tu15 import BODY_COUNTS as COLLECTION
from generate_profile_binding_tu15 import BODY_COUNTS as PROFILE
DATA_ONLY = {0x12e9a0:34}  # Existing complete native UI/input-suppression owner.


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries=set(COLLECTION)|set(PROFILE)|set(DATA_ONLY)
    constant={};cold={};indexed=[]
    def append(at,raw,writable):
        target=cold if writable else constant
        for n,value in enumerate(raw):
            assert at+n not in target or target[at+n]==value
            target[at+n]=value
    # Complete original65690's four fixedD0 member destinations, genuine
    # hardware-identity cells and the backend's four handles/presence byte.
    for at,n in ((0x4da128,4*0xd0),(0x4dc42c,6),(0x4d61c8,17)):
        raw=image.read(at,n);assert raw==bytes(n);append(at,raw,True)
    #15EBA0's local-player input consumer15E7FF reaches this operand when
    # complete collection supplies its genuine148-byte input, including+46.
    # Mounting remains confined to the independent physical-input world.
    instruction=code.at(0x15e7ff)
    assert instruction.mnemonic=='movss' and instruction.op_str=='xmm2, dword ptr [0x46a81c]'
    append(0x46a81c,image.read(0x46a81c,4),False)
    for entry in sorted(entries):
        body=_trace(code,entry,entries)
        if entry in DATA_ONLY:assert len(body.code)==DATA_ONLY[entry]
        for at,width,count in body.tables:append(at,image.read(at,width*count),False)
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            for op in i.operands:
                if op.type!=3 or op.mem.base or op.mem.disp<0x10000:continue
                at=op.mem.disp&0xffffffff
                section=next((s for s in image.sections if s.va<=at< s.va+s.vsize),None)
                if section is None:continue
                writable=bool(section.flags&1)
                width=40 if at in (0x44ecd4,0x44ecfc) else 16 if at==0x463338 else op.size
                if op.mem.index:
                    indexed.append(dict(instruction=pc,address=at,operand=i.op_str,
                        declared_bytes=width,scope='Remaining indexed extent must already have a genuine source owner or fail at the actual read'))
                if not writable:append(at,image.read(at,width),False)
                elif not op.mem.index:append(at,image.read(at,width),True)
    rows=[]
    for writable,values in ((False,constant),(True,cold)):
        addresses=sorted(values);n=0
        while n<len(addresses):
            at=addresses[n];raw=bytearray([values[at]]);n+=1
            while n<len(addresses)and addresses[n]==at+len(raw):raw.append(values[addresses[n]]);n+=1
            rows.append(dict(address=at,bytes=bytes(raw),writable=writable))
    lines=['// Generated exact English TU1.5 input/profile data.','// Original SHA256: '+NEW_PIN]
    for n,row in enumerate(rows):
        raw=row['bytes'];lines.append('constexpr std::array<std::byte,'+str(len(raw))+'> input_data_'+str(n)+'{')
        for start in range(0,len(raw),12):lines.append('    '+','.join(f'std::byte{{0x{v:02x}}}' for v in raw[start:start+12])+',')
        lines.append('};')
    lines.append('const InputData input_data[]{')
    for n,row in enumerate(rows):lines.append(f'    {{0x{row["address"]:x}u,input_data_{n},'+('true'if row['writable']else'false')+'},')
    lines.append('};')
    output=('\n'.join(lines)+'\n').encode();args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(output)
    report=dict(original_sha256=NEW_PIN,generated_sha256=hashlib.sha256(output).hexdigest(),
        regions=[dict(address=r['address'],bytes=len(r['bytes']),sha256=hashlib.sha256(r['bytes']).hexdigest(),writable=r['writable'])for r in rows],
        indexed_operands=indexed,data_only_existing_native_entries=DATA_ONLY,
        scope=__doc__,retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(regions=len(rows),bytes=sum(len(r['bytes'])for r in rows))))


if __name__=='__main__':main()
