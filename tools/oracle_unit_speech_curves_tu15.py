#!/usr/bin/env python3
"""Execute unchanged110520 over original-loader-extracted opening facial curves.

Curve bytes, previous controls, elapsed seconds, blend and head-suppression are
explicit sampler inputs.380DD0 initializes the actual normalization operand.
There are no patched instructions or intercepted callees. This isolates curve
sampling/quantization; it does not supply sound admission, voices or final poses.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
sys.path[:0]=[str(Path.cwd()/'tools'),str(Path.cwd()/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from functions import _Code,_trace
from research_mp_sandbox_update_functions import NEW_PIN,NEW_DEFAULT

def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def real(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--curves',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--xbe',type=Path,default=NEW_DEFAULT);a=p.parse_args()
    if a.output.exists():raise ValueError('Preserve original evidence')
    x=Xbe(a.xbe)
    if hashlib.sha256(x.data).hexdigest()!=NEW_PIN:raise ValueError('Expected English TU1.5')
    if struct.unpack('<I',x.read(0x46d964,4))[0]!=0x380dd0:raise ValueError('Original normalization registration differs')
    code=_Code(x,'.text');constants={}
    for entry in (0x110520,0x380dd0):
        for pc in _trace(code,entry,{0x110520,0x380dd0}).code:
            i=code.at(pc)
            for op in i.operands:
                if op.type==3 and not op.mem.base and not op.mem.index and op.mem.disp!=0x5564d8:
                    before=constants.get(op.mem.disp,b'')
                    constants[op.mem.disp]=x.read(op.mem.disp,max(len(before),op.size))
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,n in ((0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)):g.mem_map(at,n)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    descriptor,curve,record,stack,stop=0x2100000,0x2101000,0x2104000,0x21fff00,0x3000000
    def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*words))
    put(stack,stop);g.reg_write(R.UC_X86_REG_ESP,stack);g.emu_start(0x380dd0,stop,count=100)
    normalization=struct.unpack('<I',g.mem_read(0x5564d8,4))[0]
    rows=[];curves=[]
    for line in (a.curves/'curves.tsv').read_text().splitlines():
        sound,size,file,name=line.split(' ',3);data=(a.curves/file).read_bytes()
        if len(data)!=int(size):raise ValueError('Extracted original curve size differs')
        channels,kind=struct.unpack_from('<hH',data)
        if not 0<channels<=24 or kind!=0x96c:raise ValueError('Unexpected original curve format')
        factor=struct.unpack_from('<f',data,12)[0];times={bits(-1),bits(0),bits(.001)}
        key_times=[]
        for n in range(channels):
            count,first=struct.unpack_from('<hh',data,16+n*4)
            for k in range(count):
                at=16+channels*4+(first+k)*4
                key_times.append(real(bits(struct.unpack_from('<h',data,at)[0]*factor)))
        if not key_times:raise ValueError('Actual curve contains no keys')
        maximum=max(key_times);times.update(bits(v)for v in (maximum/3,maximum/2,maximum,maximum+.1))
        # Boundary-adjacent and between-key samples exercise original smooth
        # interpolation rather than only reproducing authored key values.
        ordered=sorted(set(key_times))
        for i in (len(ordered)//4,len(ordered)//2,3*len(ordered)//4):
            v=ordered[i];times.update((bits(v),bits(v-1/48000),bits(v+1/48000)))
            if i+1<len(ordered):times.add(bits((v+ordered[i+1])/2))
        digest=hashlib.sha256(data).hexdigest();curves.append(dict(sound=sound,sha256=digest,data=data.hex()))
        g.mem_write(curve,data);put(descriptor,len(data),curve)
        for index,time in enumerate(sorted(times)):
            for blend,head in ((1.,0),(.25,0),(.5,1)):
                before=bytearray((i*17+3)&255 for i in range(48))
                for n in range(12):
                    before[0x14+n]=n*2;before[0x20+n]=(n*19+index*7)&255
                g.mem_write(record,bytes(before));put(stack,stop,time,bits(blend),head)
                for reg in (R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):g.reg_write(reg,0)
                for n in range(8):g.reg_write(getattr(R,'UC_X86_REG_XMM'+str(n)),0)
                g.reg_write(R.UC_X86_REG_EAX,descriptor);g.reg_write(R.UC_X86_REG_EDX,record)
                g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EFLAGS,2)
                g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
                g.emu_start(0x110520,stop,count=100000)
                if g.reg_read(R.UC_X86_REG_EIP)!=stop or g.reg_read(R.UC_X86_REG_ESP)!=stack+16:
                    raise ValueError('Original curve sampler did not complete its actual ABI')
                rows.append(dict(sound=sound,time=time,blend=bits(blend),head=head,
                    before=before.hex(),after=bytes(g.mem_read(record,48)).hex()))
    if len(curves)!=38:raise ValueError('Incomplete original opening curve coverage')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(dict(scope=__doc__,xbe_sha256=NEW_PIN,normalization=normalization,
        instruction_patches=[],intercepted_callees=[],constants={f'{p:08x}':b.hex()for p,b in constants.items()},
        curves=curves,cases=rows),indent=2)+'\n')
    print('PASS',len(curves),'actual original facial curves;',len(rows),'unchanged110520 cases; normalization',hex(normalization))
if __name__=='__main__':main()
