#!/usr/bin/env python3
"""Execute original seconds-to-ticks conversion under caller-owned FP control.

The original inactive-tick caller establishes FPCW/MXCSR. Its input producer
and frame finish are explicit timing-oracle services. The complete original
conversion helper then executes without resetting the caller's FP environment.
Fractional inputs are bounded helper fixtures, not authored variant seconds.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

import capstone
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
from oracle_mp_spawn import ROOT, VERSIONS, Cache, CACHE_PIN, STACK, STOP, f32
from oracle_mp_spawn_timing import TimingOracle, i32


def main():
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    entries={'base':0x1469f0,'tu11':0x148160}
    rows={};instructions={}
    for version,entry in entries.items():
        oracle=TimingOracle(version);oracle.fixture(cache);oracle.tick(False)
        assert oracle.g.reg_read(UC_X86_REG_FPCW)==0x23f
        assert oracle.g.reg_read(UC_X86_REG_MXCSR)==0x1f80
        writes=[]
        def hook(uc,access,address,size,value,unused):
            if address==STACK+4 or (address==STACK-4 and uc.reg_read(UC_X86_REG_EIP)!=entry):
                writes.append(dict(instruction=f'{uc.reg_read(UC_X86_REG_EIP):08x}',
                    bits=f'{value&0xffffffff:08x}'))
        oracle.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,hook)
        cases=[]
        for seconds in (0,.05,.15,.25,.75,-.05,-.15,-.25,-.75,1,5):
            writes.clear();oracle.w(STACK,STOP);oracle.f(STACK+4,seconds)
            before=struct.unpack('<f',oracle.g.mem_read(STACK+4,4))[0]
            oracle.g.reg_write(UC_X86_REG_ESP,STACK)
            oracle.g.emu_start(entry,STOP,count=100000)
            assert oracle.g.reg_read(UC_X86_REG_EIP)==STOP
            result=i32(oracle.g.reg_read(UC_X86_REG_EAX))
            product=f32(before*30)
            assert result==round(product)
            assert len(writes)==2
            assert writes[0]['bits']==struct.pack('<f',product)[::-1].hex()
            assert writes[1]['bits']==f'{result&0xffffffff:08x}'
            cases.append(dict(input_seconds=seconds,input_f32_seconds=before,tick_rate=30,
                original_stores=list(writes),single_precision_product=product,result=result,
                fpcw=f'{oracle.g.reg_read(UC_X86_REG_FPCW):04x}',
                mxcsr=f'{oracle.g.reg_read(UC_X86_REG_MXCSR):08x}'))
        rows[version]=cases
        md=Cs(CS_ARCH_X86,CS_MODE_32)
        ins=[]
        for i in md.disasm(oracle.x.read(entry,0x80),entry):
            ins.append(f'{i.address:08x} {i.mnemonic} {i.op_str}')
            if i.mnemonic=='ret':break
        instructions[version]=ins
    def clean(x):
        if isinstance(x,dict):return {k:clean(v) for k,v in x.items() if k!='instruction'}
        if isinstance(x,list):return [clean(v) for v in x]
        return x
    assert clean(rows['base'])==clean(rows['tu11'])
    report=dict(passed=True,scope=__doc__,case_count=sum(map(len,rows.values())),
        oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        imported_tool_sha256={n:hashlib.sha256((ROOT/f'tools/{n}.py').read_bytes()).hexdigest() for n in
            ('oracle_mp_spawn','oracle_mp_spawn_lifecycle','oracle_mp_spawn_ownership','oracle_mp_spawn_timing')},
        authorities={v:dict(path=a['path'],sha256=a['pin']) for v,a in VERSIONS.items()},
        dependencies=dict(python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__),
        cache=dict(path='assets/disc/maps/lockout.map',sha256=CACHE_PIN),
        entries={v:f'{a:08x}' for v,a in entries.items()},original_instructions=instructions,rows=rows)
    output=ROOT/'analysis/multiplayer/integration/spawn-source/rounding-report.json'
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps(dict(passed=True,case_count=report['case_count'])))


if __name__=='__main__':main()
