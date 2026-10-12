#!/usr/bin/env python3
"""Execute untouched original local-profile conversion and input-member update.

The four controller slots, their signed32-bit committed property, member slot,
identity/name/appearance bytes and empty input-member records are API stimuli.
Whole original converters and profile-update owners return without supplied
services, instruction patches, readiness, allocation or automatic assignment.
This proves the team field's conversion/delivery, not Xbox network encoding or
the complete saved-profile, sign-in, player creation or team assignment owners.
"""
import argparse, csv, hashlib, json, random, struct
from pathlib import Path
from oracle_mp_spawn import Oracle, ROOT, MEM, VERSIONS
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP
from oracle_smg_firing_1_1 import _Code
from oracle_mp_full_scoreboard_1_5 import literal
from research_mp_lobby_later_update import NEW, NEW_PIN, complete_tokens

OUT = ROOT / 'analysis/multiplayer/game-variants/local-profile-team'
FIXTURE = ROOT / 'tests/fixtures/local_profile_team_original.tsv'
VERSIONS['u5'] = dict(path=str(NEW), pin=NEW_PIN)
SOURCE = {
    'base': dict(convert=0x18fd94, update=0x18fe9e, local=0x54e8e0,
        stride=0xc70, members=0x4cd914, functions='analysis/retail/functions.tsv'),
    'tu11': dict(convert=0x1917c6, update=0x1918d0, local=0x556430,
        stride=0xc74, members=0x4d4e34, functions='analysis/map-pack-update/functions.tsv'),
    'u5': dict(convert=0x192baa, update=0x192cb7, local=0x55d770,
        stride=0xc74, members=0x4da138,
        functions='analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'),
}
WIRE, IDENTITY = MEM+0x90000, MEM+0x90200

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
class ProfileOracle(Oracle):
    def __init__(self, version):
        self.visited=set(); self.stores=[];self.slot=0
        super().__init__(version)
        self.code=_Code(self.x,'.text')
        self.starts={int(r['address'],16) for r in csv.DictReader(
            (ROOT/SOURCE[version]['functions']).open(),delimiter='\t')}
        self.g.hook_add(UC_HOOK_MEM_WRITE,self.write)
    def hook(self,g,a,n,user): self.visited.add(a)
    def write(self,g,access,a,n,v,user):
        targets=(WIRE+0x7c,SOURCE[self.version]['members']+self.slot*0xd0+4+0x7c)
        if any(a<=target<a+n for target in targets):
            self.stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                address=f'{a:08x}',size=n,value=v & ((1<<(n*8))-1)))

def values():
    rng=random.Random(0x18fd94)
    return sorted(set(range(-128,128)) | {-2147483648,-65537,-32769,-32768,-257,-256,-129,
        128,129,255,256,257,32767,32768,65535,65536,2147483647} |
        {rng.randrange(-2147483648,2147483648) for _ in range(64)})

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--fixture',type=Path,default=FIXTURE)
    args=ap.parse_args(); args.output.parent.mkdir(parents=True,exist_ok=True)
    rows=[]; cases=[]; oracles={}; listings=[]
    for version,a in SOURCE.items():
        p=ProfileOracle(version);oracles[version]=p
        for slot in range(4):
            p.slot=slot
            for committed in values():
                # Declared source caller storage. No return value/global gate is supplied.
                p.g.mem_write(a['members'],bytes(4*0xd0))
                local=a['local']+slot*a['stride']
                p.g.mem_write(local,bytes(a['stride']))
                p.w(local+4,slot);p.w(local+0x1fc,committed)
                identity=struct.pack('<3I',0x01234567+slot,0x89abcdef,0xfedcba98)
                appearance=bytes((10,0,0,10))+bytes(range(12))
                name=(f'Controller{slot}'+ '\0').encode('utf-16le')
                p.g.mem_write(local+8,identity);p.g.mem_write(local+0x130,appearance)
                p.g.mem_write(local+0xc1c,name)
                p.g.mem_write(WIRE,b'\xcc'*0x90);p.g.mem_write(IDENTITY,b'\xcc'*12)
                before=len(p.stores)
                argv=(slot,WIRE) if version=='u5' else (WIRE,)
                p.call(a['convert'],argv,((UC_X86_REG_EAX,IDENTITY),(UC_X86_REG_ECX,slot)))
                wire=bytes(p.g.mem_read(WIRE,0x90));team=struct.unpack_from('<b',wire,0x7c)[0]
                assert bytes(p.g.mem_read(IDENTITY,12))==identity
                assert wire[:len(name)]==name and wire[0x40:0x50]==appearance
                p.call(a['update'],registers=((UC_X86_REG_EDI,slot),))
                delivered=bytes(p.g.mem_read(a['members']+slot*0xd0+4,0x90))
                assert delivered==wire,(version,slot,committed,delivered.hex(),wire.hex())
                assert p.u(a['members']+slot*0xd0)==slot
                rows.append(f'{version}\t{slot}\t{committed}\t{team}\t{delivered[0x7c]:02x}')
                cases.append(dict(version=version,controller=slot,committed=committed,team=team,
                    wire90_hex=wire.hex(),identity12_hex=identity.hex(),
                    complete_converter_return=True,complete_update_return=True,
                    delivered90_matches=True,observed_stores=p.stores[before:]))
        listingdir=args.output.parent/'instructions';listingdir.mkdir(exist_ok=True)
        entries=(p.starts & p.visited) | {a['convert'],a['update']}
        for entry in sorted(entries):
            _,ins=literal(p.code,entry,p.starts)
            path=listingdir/f'{version}-{entry:08x}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            listings.append(dict(version=version,entry=f'{entry:08x}',instructions=len(ins),
                file=path.name,sha256=sha(path)))
    # Complete retail/TU1.1 converter equivalence permits their changed stride
    # as a recorded operand. u5 changes its ABI and an extra field's producer;
    # all three are independently executed instead of assuming shape equality.
    old,new=oracles['base'],oracles['tu11']
    _,oi=literal(old.code,SOURCE['base']['convert'],old.starts)
    _,ni=literal(new.code,SOURCE['tu11']['convert'],new.starts)
    ot,nt=complete_tokens(oi,old.x),complete_tokens(ni,new.x)
    differences=[dict(index=i,retail=x,tu11=y) for i,(x,y) in enumerate(zip(ot,nt)) if x!=y]
    assert len(ot)==len(nt) and len(differences)==2
    assert differences[0]['retail']=='imul ebx, ebx, 0xc70' and differences[0]['tu11']=='imul ebx, ebx, 0xc74'
    assert differences[1]['retail']=='add ebx, 0x54e8e0' and differences[1]['tu11']=='add ebx, 0x556430'
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text('# Complete original local-profile conversion/update; version controller signed32 signed8 raw-byte\n'+'\n'.join(rows)+'\n')
    report=dict(schema='original-local-profile-team-v1',oracle_sha256=sha(__file__),
        source_images={v:dict(path=VERSIONS[v]['path'],sha256=VERSIONS[v]['pin']) for v in SOURCE},
        contracts=SOURCE,scope=__doc__,signed32_inputs=len(values()),cases=cases,
        original_instruction_counts={v:len(o.visited) for v,o in oracles.items()},
        complete_retail_tu11_pair=dict(instructions=len(ot),differences=differences),
        instruction_listings=listings,fixture_sha256=sha(args.fixture),
        instruction_patches=[],supplied_services=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),signed32_inputs=len(values()),complete_updates=len(cases),
        fixture_sha256=sha(args.fixture),report_sha256=sha(args.output),supplied_services=0)))
if __name__=='__main__':main()
