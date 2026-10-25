#!/usr/bin/env python3
"""Export exact fixed-address CPU operands for replica lens presentation.

Every byte is authenticated against TU1.5. Mutable bytes are process-cold
destinations or configuration, never a captured original graphics result.
Runtime camera, cache, arena, request and GPU owners remain separate.
"""
import argparse,hashlib,json,os
from pathlib import Path
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_lens_query_tu15 import QueryCode,ENTRIES as QUERY
from generate_lens_draw_tu15 import ENTRIES as DRAW
EXTRA=(0x2db80,0x2e210,0x20990,0x20b20,0x20680,0x20700,0x1fb90,0x20e90,0x169d0,0x126850)
# Existing authenticated tail storage and explicit native caller inputs own
# these complete extents. Do not create overlapping byte-sized substitutes.
OWNED=((0x52dfd8,0x40c4),(0x515ec0,8),(0x515e2c,4),
       (0x4c65c0,0x138),(0x491e20,0x14c),(0x46a0dc,4))
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=QueryCode(image);entries=set(QUERY+DRAW+EXTRA);cells={};sites={}
    for entry in sorted(entries):
        body=_trace(code,entry,entries)
        # Indexed CFG tables are data operands too. Their complete bounded
        # extents come from the same authenticated trace as the native kernel.
        for at,width,count in body.tables:
            assert width in (1,4)
            section=next(s for s in image.sections if s.va<=at and at+width*count<=s.va+s.vsize)
            for n,b in enumerate(image.read(at,width*count)):
                v=(b,bool(section.flags&1));assert at+n not in cells or cells[at+n]==v
                cells[at+n]=v;sites.setdefault(at+n,set()).add(entry)
        for pc in body.code:
            ins=code.at(pc);assert bytes(ins.bytes)==image.read(pc,ins.size)
            if ins.mnemonic=='lea':continue
            for op in ins.operands:
                if op.type!=X86_OP_MEM or op.mem.base or op.mem.index:continue
                at=op.mem.disp&0xffffffff
                if not at:continue
                section=next((s for s in image.sections if s.va<=at and at+op.size<=s.va+s.vsize),None)
                assert section is not None,(hex(pc),hex(at),op.size)
                for n,b in enumerate(image.read(at,op.size)):
                    address=at+n
                    if any(start<=address<start+size for start,size in OWNED):continue
                    v=(b,bool(section.flags&1))
                    assert address not in cells or cells[address]==v
                    cells[address]=v;sites.setdefault(address,set()).add(pc)
    # Full native CPU destinations used by original CRC construction and
    # complete request metadata. They are authenticated zero, not results.
    for at,size in ((0x56e225,1),(0x56e258,1024),(0x4f2950,16)):
        raw=image.read(at,size);assert not any(raw)
        for n,b in enumerate(raw):
            v=(b,True);assert at+n not in cells or cells[at+n]==v;cells[at+n]=v
    # The complete lens rotation owner17F790 calls37DC0A. Its existing native
    # CRT entry takes EDX=48C8CC and indexes this typed descriptor and
    # the sixteen class offsets; these indirect reads are not absolute x86
    # operands. Preserve the authored data and cold policy, never a result.
    fmod=code.at(0x37dc0a)
    assert fmod.mnemonic=='mov' and fmod.op_str=='edx, 0x48c8cc'
    descriptors=[]
    for at,size in ((0x48c8cc,80),(0x46ecb4,16),(0x46ecd0,4),(0x48d4f4,4)):
        raw=image.read(at,size)
        section=next(s for s in image.sections if s.va<=at and at+size<=s.va+s.vsize)
        mutable=bool(section.flags&1)
        if at==0x48c8cc:assert raw[:16]==bytes.fromhex('04666d6f640000000000000000021600')
        if at==0x48d4f4:assert not any(raw)
        for n,b in enumerate(raw):
            v=(b,mutable);assert at+n not in cells or cells[at+n]==v
            cells[at+n]=v;sites.setdefault(at+n,set()).add(0x37dc0a)
        descriptors.append(dict(address=at,bytes=size,mutable=mutable,sha256=hashlib.sha256(raw).hexdigest()))
    rows=[];lines=['// Generated from authenticated TU1.5 CPU operands; no executable I/O at runtime.']
    remaining=sorted(cells);i=0
    while i<len(remaining):
        start=remaining[i];mutable=cells[start][1];end=i+1
        while end<len(remaining) and remaining[end]==remaining[end-1]+1 and cells[remaining[end]][1]==mutable:end+=1
        raw=bytes(cells[k][0] for k in remaining[i:end]);access='ReadWrite' if mutable else 'ReadOnly'
        lines.append(f'operand(0x{start:x}u,std::array<std::uint8_t,{len(raw)}>{{'+','.join(f'0x{b:02x}' for b in raw)+f'}},SourceRuntimeMemory::Access::{access});')
        rows.append(dict(address=start,bytes=len(raw),mutable=mutable,zero=not any(raw),sha256=hashlib.sha256(raw).hexdigest(),instruction_sites=sorted(set().union(*(sites.get(k,set()) for k in remaining[i:end])))))
        i=end
    # Native view destinations retain the actual camera/projection inputs.
    caller_storage=[]
    for at,size in OWNED[3:5]:
        raw=image.read(at,size);assert not any(raw)
        caller_storage.append(dict(address=at,bytes=size,sha256=hashlib.sha256(raw).hexdigest(),process_cold=True))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(lines)+'\n')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(scope=__doc__,xbe_sha256=NEW_PIN,regions=rows,bytes=sum(r['bytes'] for r in rows),caller_camera_storage=caller_storage,indirect_CRT_descriptors=descriptors,generated_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),instruction_or_game_result_replaced=False),indent=2)+'\n')
    print(json.dumps(dict(regions=len(rows),bytes=sum(r['bytes'] for r in rows))),flush=True)
if __name__=='__main__':main()
