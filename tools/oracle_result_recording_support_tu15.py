#!/usr/bin/env python3
"""Compare unchanged recipient admission and retained-award writer returns.

Synthetic context, identities, masks and session modes are declared inputs.
No instruction, callee return or machine-lookup result is supplied. Run under
tools/run_test.py; only the small original comparison fixture is retained.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_game_variants import Source,ROOT,STACK
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import unicorn.x86_const as R

REGS=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);assert scratch.is_dir()
    source=Source(dict(path=NEW_DEFAULT,pin=NEW_PIN,globals=0x4ecc1c))
    # Independent cold image reads precede every synthetic caller input.
    # Sizes follow the original context's next owner and19A440 REP STOSD
    # extent, leaving the existing51EEF4 statistic declaration intact.
    cold=[]
    spans=[(p,n,True)for p,n in ((0x554bd4,0xc34),(0x4dc041,2),(0x4dc048,1),
            (0x48a0c4,8),(0x51dfa4,0xf50),(0x522f34,0x8ef4),(0x535f00,1),(0x51d014,8),(0x51d040,8),
            (0x56b990,0x1140),(0x56cad0,0x840),(0x52d698,8),(0x50ed24,4),(0x46e8e8,4))]
    for address,size,writable in spans+[(0x779d0,16,False),(0x779e0,20,False),(0x32e2c9,32,False),
                                      (0x437544,4,False),(0x45f71c,12,False),(0x45f728,8,False)]:
        data=bytes(source.g.mem_read(address,size))
        cold.append(dict(address=address,size=size,writable=writable,sha256=hashlib.sha256(data).hexdigest(),
                         nonzero=[[i,b]for i,b in enumerate(data)if b]))
    assert source.image.read(0x19cadd,7)==bytes.fromhex('68e0a647006a20')
    table=bytes(source.g.mem_read(0x47a6e0,32*16));awards=[]
    for address,size,writable in [(0x47a6e0,len(table),True)]+[
        (p,len(bytes(source.g.mem_read(p,256)).split(b'\0')[0])+1,False)
        for p in sorted({struct.unpack_from('<I',table,n*16)[0]for n in range(32)})]:
        data=bytes(source.g.mem_read(address,size))
        awards.append(dict(address=address,size=size,writable=writable,
                           nonzero=[[i,b]for i,b in enumerate(data)if b]))
    assert source.image.read(0x19cb5d,7)==bytes.fromhex('6850a647006a07')
    combat=[]
    for start,count in ((0x47a650,7),(0x47a6c0,2)):
        raw=bytes(source.g.mem_read(start,count*16))
        for address,size,writable in [(start,len(raw),True)]+[
            (p,len(bytes(source.g.mem_read(p,256)).split(b'\0')[0])+1,False)
            for p in sorted({struct.unpack_from('<I',raw,n*16)[0]for n in range(count)})]:
            data=bytes(source.g.mem_read(address,size))
            combat.append(dict(address=address,size=size,writable=writable,
                               nonzero=[[i,b]for i,b in enumerate(data)if b]))
    base=0x6000000;source.g.mem_map(base,0x10000);cases=[]
    for session in (False,True):
      for mode in (-1,0,2,3,8,9):
       for local in (0,1,15):
        for present in (False,True):
         for matching in (False,True):
          data=bytearray(0x10000)
          def w(at,value):struct.pack_into('<I',data,at,value&0xffffffff)
          w(0xc,base+0x2000 if session else 0);w(0x1c,(1<<local)if present else 0)
          identity=bytes((1,2,3,4,5,local+1));data[0x24+local*6:0x2a+local*6]=identity
          data[0x1000:0x1006]=identity if matching else bytes((9,8,7,6,5,4))
          w(0x2000+0x741c,mode);w(0x2000+0x40,local)
          data[0x2000+0x58+local*0x10c+10:0x2000+0x58+local*0x10c+16]=identity
          source.g.mem_write(base,bytes(data))
          gp=[base+0x1000,base,0x13579bdf,0x2468ace0,STACK,0x11223344,0x55667788,0x99aabbcc]
          source.g.reg_write(R.UC_X86_REG_EFLAGS,0x202)
          source.call(0x878c0,registers=[(reg,value)for reg,value in zip(REGS,gp)if reg!=R.UC_X86_REG_ESP])
          assert bytes(source.g.mem_read(base,len(data)))==bytes(data)
          cases.append(dict(session=session,mode=mode,local=local,present=present,matching=matching,
              gp=[source.g.reg_read(reg)for reg in REGS],eflags=source.g.reg_read(R.UC_X86_REG_EFLAGS)&0x8d5))
    award_cases=[]
    seed=b''.join(struct.pack('<H',(i*73+0x3141)&65535)for i in range(0x4040//2))
    for field in range(32):
     for slot in (0,15):
      for enabled,frozen in ((0,0),(1,0),(1,1)):
       for n,delta in enumerate((-2,1,0x7fffffff)):
        before=(0,1023,32768,65535)[(field+slot+n)%4];offset=(slot*437+45+field)*2
        bank=bytearray(seed);struct.pack_into('<H',bank,offset,before)
        source.g.mem_write(0x51eef4,bytes(bank));source.g.mem_write(0x51dfa0,bytes((enabled,frozen)))
        gp=[field,slot,0x13579bdf,delta&0xffffffff,STACK,0x11223344,0x55667788,0x99aabbcc]
        source.g.reg_write(R.UC_X86_REG_EFLAGS,0x202)
        source.call(0x19baa0,registers=[(reg,value)for reg,value in zip(REGS,gp)if reg!=R.UC_X86_REG_ESP])
        after=bytearray(source.g.mem_read(0x51eef4,len(bank)));value=struct.unpack_from('<H',after,offset)[0]
        after[offset:offset+2]=bank[offset:offset+2];assert after==bank
        award_cases.append(dict(field=field,slot=slot,enabled=enabled,frozen=frozen,delta=delta,before=before,after=value,
            gp=[source.g.reg_read(reg)for reg in REGS],eflags=source.g.reg_read(R.UC_X86_REG_EFLAGS)&0x8d5))
    rank_cases=[]
    for entry,field,stride in ((0x19da10,0x51e3d4,0xa4),(0x19da60,0x51ed75,0x18)):
     for left,right in ((0,1),(15,0),(3,12)):
      for ranks in ((-128,127),(127,-128),(0,0),(-1,-1)):
       for scores in ((-32768,32767),(32767,-32768),(1,1),(1,-1)):
        data=bytearray((i*17+7)&255 for i in range(0xf44))
        for slot,rank,score in zip((left,right),ranks,scores):
            at=field+slot*stride-0x51dfb0;struct.pack_into('<b',data,at,rank)
            struct.pack_into('<h',data,at+(2 if entry==0x19da10 else 1),score)
        source.g.mem_write(0x51dfb0,bytes(data));gp=[0x12345678,0x9abcdef0,0x13579bdf,0x2468ace0,STACK,0x11223344,0x55667788,0x99aabbcc]
        source.g.reg_write(R.UC_X86_REG_EFLAGS,0x202)
        source.call(entry,(left,right,0xa5a5a5a5),registers=[(reg,value)for reg,value in zip(REGS,gp)if reg!=R.UC_X86_REG_ESP])
        assert bytes(source.g.mem_read(0x51dfb0,len(data)))==bytes(data)
        rank_cases.append(dict(entry=entry,left=left,right=right,ranks=ranks,scores=scores,
            gp=[source.g.reg_read(reg)for reg in REGS],eflags=source.g.reg_read(R.UC_X86_REG_EFLAGS)&0x8d5))
    result=dict(original_sha256=NEW_PIN,entry='0x878c0',callee='0x877c0',cases=cases,award_cases=award_cases,rank_cases=rank_cases,
                instruction_patches=[],supplied_returns=[],executed_sites=len(source.executed),cold_regions=cold,award_regions=awards,combat_regions=combat)
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(result,indent=2)+'\n')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(
        original_sha256=NEW_PIN,cases=len(cases),fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),scratch=str(scratch),
        instruction_patches=[],supplied_returns=[],cold_regions=len(cold),
        cold_bytes=sum(row['size']for row in cold),award_regions=len(awards),award_cases=len(award_cases),
        award_bytes=sum(row['size']for row in awards),rank_cases=len(rank_cases),combat_regions=len(combat),
        combat_bytes=sum(row['size']for row in combat)),indent=2)+'\n')
    print('Complete original recording comparisons:',len(cases),'recipient,',len(award_cases),'award,',len(rank_cases),'retained ranking cases')
if __name__=='__main__':main()
