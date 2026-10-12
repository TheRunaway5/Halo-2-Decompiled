#!/usr/bin/env python3
"""Untouched222A0/211A0/20F30/34A90/34100 fixed color-shadow filter."""
from pathlib import Path
import hashlib,json,random,struct
ROOT=Path(__file__).resolve().parents[1]
authority=ROOT/'tools/audit_title_post_targets.py'
ns={'__file__':str(authority)}
exec(authority.read_text().split('g.mem_write(0x4b4c78')[0],ns)
globals().update(ns)
OUT=ROOT/'analysis/multiplayer-shading/original-xemu/shadow-gpu-audit'
calls=[];sources=[]
stubs[0x3fa3d0]=0;stubs[0x14f60]=0
def observe(guest,pc,size,user):
    if pc==0x14f60:
        sources.append((guest.reg_read(UC_X86_REG_EDI)&0xffff,guest.reg_read(UC_X86_REG_EBX)&0xffff))
    if pc!=0x34a90:return
    sp=guest.reg_read(UC_X86_REG_ESP)
    calls.append(struct.unpack('<11I',guest.mem_read(sp+4,44)))
    guest.reg_write(UC_X86_REG_EIP,struct.unpack('<I',guest.mem_read(sp,4))[0])
    guest.reg_write(UC_X86_REG_ESP,sp+48)
handle=g.hook_add(UC_HOOK_CODE,observe)
g.mem_write(0x4b4c78,struct.pack('<I',0x2200000))
g.mem_write(0x4b5138,struct.pack('<I',0x2200000))
run(0x1d770)
targets={a['target']:(a['width'],a['height'])for a in allocations}
assert targets[10]==targets[11]==(128,128)
g.mem_write(0x407488,struct.pack('<I',0x2350000))
g.mem_write(0x2350000,struct.pack('<II',0x23c0000,0x23f0000))
run(0x222a0)
assert [x[0]for x in calls]==[11,10] and [x[6]for x in calls]==[0x40000000,0x40400000]
assert sources==[(10,i)for i in range(4)]+[(11,i)for i in range(4)],sources
program=bytes(g.mem_read(0x484f68,240))
assert struct.unpack_from('<I',program,53*4)[0]==0x11003
g.hook_del(handle)
# Actual128² resident metadata, consumed by untouched1DD60. GPU allocations,
# sampler/submission endpoints are explicit services; coordinate math is not.
for target in (10,11):
    g.mem_write(0x4b4bd8+target*0x98,struct.pack('<3I',128,128,0x2200000))
g.mem_write(0x485af4,struct.pack('<4I',128,128,128,128))
g.mem_write(0x485b04,struct.pack('<4I',128,128,128,128))
g.mem_write(0x485a80,struct.pack('<I',0x2110000))
g.mem_write(0x211005c,struct.pack('<I',0x2111000))
export=json.loads((OUT/'original20-shadow-filter-export.json').read_text())
fixture=bytearray(struct.pack('<2I',0x46533248,1))
checked=0;randomizer=random.Random(0x222a0);proof=[]
for step,args in enumerate(calls):
    setup.clear();run(0x34a90,args);assert len(setup)==1
    rectangle,parameters=setup[0]
    assert struct.unpack('<4f',rectangle)==(0.,128.,0.,128.)
    g.mem_write(0x2100000,rectangle);g.mem_write(0x2100100,parameters)
    offsets=[]
    for stage in range(4):
        g.mem_write(0x2100200,struct.pack('<2f',0,0))
        run(0x34100,(stage+1,0x2100000,0x2100200,0x2100300,0x2100100))
        offsets.append(bytes(g.mem_read(0x2100300,16)))
        expected=(OUT/export['filters'][step]['vertex_buffers'][stage+1]['file']).read_bytes()[:16]
        assert offsets[-1]==expected
        checked+=4
    fixture.extend(struct.pack('<2I',10 if step==0 else 11,args[0]));fixture.extend(program)
    fixture.extend(b''.join(offsets))
    for sample in range(1024):
        point=[struct.unpack('<f',struct.pack('<f',randomizer.uniform(-256,256)))[0]for _ in range(2)]
        g.mem_write(0x2100200,struct.pack('<2f',point[0]/128,point[1]/128))
        for stage,offset in enumerate(offsets):
            run(0x34100,(stage+1,0x2100000,0x2100200,0x2100300,0x2100100))
            x,y,_,_=struct.unpack('<4f',offset)
            assert bytes(g.mem_read(0x2100300,16))==struct.pack('<4f',point[0]+x,point[1]+y,0,1)
            checked+=4
    proof.append(dict(source_target=10 if step==0 else 11,destination_target=args[0],width=struct.unpack('<f',struct.pack('<I',args[6]))[0],taps=[list(struct.unpack('<4f',o))for o in offsets]))
for step,record in enumerate(export['filters']):
    assert len(record['textures'])==4
    for texture in record['textures']:
        sampler=texture['sampler']
        assert texture['width']==texture['height']==128 and texture['format']=='B8G8R8A8_UNORM'
        assert sampler['addressU']==sampler['addressV']==3 and sampler['addressW']==0
        assert sampler['filter']==dict(filter=0,magnify=2,minify=2,mip=1)
    predecessor=OUT/('original20-shadow-target-7758.bgra8'if step==0 else export['filters'][0]['result']['file'])
    assert predecessor.read_bytes()==(OUT/record['textures'][0]['file']).read_bytes()
assert (OUT/export['filters'][1]['result']['file']).read_bytes()==(OUT/'original20-shadow-receiver-stage0.bin').read_bytes()
name='retail_shadow_filter.bin';(ROOT/'tests/fixtures'/name).write_bytes(fixture)
result=dict(xbe_sha256=EXPECTED,source_entries=['1D770','222A0','211A0','20F30','1DD60','34A90','34100'],fixed_targets={str(t):list(targets[t])for t in(10,11)},program_sha256=hashlib.sha256(program).hexdigest(),fixture=name,fixture_sha256=hashlib.sha256(fixture).hexdigest(),coordinate_words_exact=checked,passes=proof,retained_input_copy_identity=True,retained_final_receiver_input_identity=True,scope='Untouched fixed filter arithmetic with explicit128 resident metadata; D3D allocation/sampler/submission services recorded, actual raw original7959/7973 quad/mip/sampler/target exports corroborate GPU inputs.')
(OUT/'shadow-filter-original-proof.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
