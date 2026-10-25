#!/usr/bin/env python3
"""Read original object-function callback literals and their exact MCC names.

No source or runtime changes. Run through tools/run_test.py with the bundled
Python that supplies Capstone. Whole original callback bodies are traced from
the authenticated executable; candidate namespace differences are not fixes.
"""
import csv,hashlib,json,os,re,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    word=lambda p:struct.unpack('<I',image.read(p,4))[0]
    callbacks={};types=[]
    for kind in range(12):
        definition=word(0x474b90+kind*4);rows=[]
        for n in range(16):
            parent=word(definition+0x84+n*4)
            if not parent:break
            target=word(parent+0x4c)
            rows.append(dict(parent=parent,target=target))
            if target:callbacks.setdefault(target,[]).append(kind)
        else:raise ValueError('Unbounded original type callback chain')
        types.append(dict(kind=kind,definition=definition,callbacks=rows))
    names={}
    for old,new,name in re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,"([^"]*)"\}',(ROOT/'src/ui/mcc_frontend_string_ids.inc').read_text()):
        original=(len(name)<<24)|int(old,16)
        assert original not in names
        names[original]=(int(new,16),name)
    starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    bodies=[]
    for entry in sorted(set(callbacks)|{0xbcb70}):
        body=_trace(code,entry,starts);literals=[]
        for pc in sorted(body.code):
            instruction=code.at(pc)
            for operand in instruction.operands:
                if operand.type!=2 or operand.imm not in names:continue
                mcc,name=names[operand.imm]
                literals.append(dict(instruction=pc,mnemonic=instruction.mnemonic,operands=instruction.op_str,
                    original=operand.imm,mcc=mcc,name=name,bytes=instruction.bytes.hex()))
        bodies.append(dict(entry=entry,types=callbacks.get(entry,[]),instructions=len(body.code),literals=literals))
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,types=types,bodies=bodies,
        name_join_sha256=hashlib.sha256((ROOT/'src/ui/mcc_frontend_string_ids.inc').read_bytes()).hexdigest(),
        runtime_changes=False)
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-object-function-symbols';out.mkdir(exist_ok=True)
    (out/'inventory.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(callbacks=len(callbacks),bodies=len(bodies),literals=sum(len(b['literals']) for b in bodies))),flush=True)
if __name__=='__main__':main()
