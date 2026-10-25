#!/usr/bin/env python3
"""Trace affine StringID comparisons through original callback control flow.

Only registers established by an authenticated named immediate are tracked.
Every derived equality target must resolve through the exact original/MCC
name join. Ambiguous paths or changed implicit increments are reported, never
turned into a guessed operand or supplied function result.
"""
import collections,csv,hashlib,json,os,re
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-object-function-symbols'
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    names={}
    for old,new,name in re.findall(r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,"([^"]*)"\}',(ROOT/'src/ui/mcc_frontend_string_ids.inc').read_text()):
        names[(len(name)<<24)|int(old,16)]=(int(new,16),name)
    inventory=json.loads((OUT/'inventory.json').read_text())
    starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
    sites={};issues=[];comparisons=[]
    def reg(instruction,value):
        name=instruction.reg_name(value)
        for base,aliases in {'eax':('eax','ax','al','ah'),'ecx':('ecx','cx','cl','ch'),
            'edx':('edx','dx','dl','dh'),'ebx':('ebx','bx','bl','bh'),'esi':('esi','si'),
            'edi':('edi','di'),'ebp':('ebp','bp'),'esp':('esp','sp')}.items():
            if name in aliases:return base
        return None
    for callback in inventory['bodies']:
        entry=callback['entry'];body=_trace(code,entry,starts);visited=set();queue=collections.deque([(entry,{})])
        while queue:
            pc,state=queue.popleft();key=(pc,tuple(sorted(state.items())))
            if key in visited or pc not in body.code:continue
            visited.add(key)
            if len(visited)>50000:raise ValueError('Unbounded affine callback control flow')
            state=dict(state);i=code.at(pc);ops=i.operands;handled=set()
            dst=reg(i,ops[0].reg) if ops and ops[0].type==1 else None
            if i.mnemonic in ('cmp','sub','add') and len(ops)==2 and dst and ops[1].type==2:
                operand=ops[1].imm&0xffffffff
                if dst not in state and operand in names and i.mnemonic!='add':state[dst]=(0,0)
                if dst in state:
                    before_old,before_new=state[dst]
                    target=(before_old-operand if i.mnemonic=='add' else before_old+operand)&0xffffffff
                    converted=(before_new-operand if i.mnemonic=='add' else before_new+operand)&0xffffffff
                    if target in names:
                        actual,name=names[target]
                        new_operand=(before_new-actual if i.mnemonic=='add' else actual-before_new)&0xffffffff
                        row=dict(entry=entry,instruction=pc,mnemonic=i.mnemonic,bytes=i.bytes.hex(),
                            original=operand,mcc=new_operand,original_target=target,mcc_target=actual,name=name,
                            original_anchor=before_old,mcc_anchor=before_new)
                        comparisons.append(row)
                        if new_operand!=operand:
                            previous=sites.get(pc)
                            if previous and (previous['original'],previous['mcc'])!=(operand,new_operand):
                                issues.append(dict(instruction=pc,error='Conflicting affine paths',previous=previous,current=row))
                            sites[pc]=row
                        converted=actual
                    if i.mnemonic!='cmp':state[dst]=(target,converted)
                    handled.add(dst)
            elif i.mnemonic=='mov' and len(ops)==2 and dst:
                src=reg(i,ops[1].reg) if ops[1].type==1 else None
                if src in state:state[dst]=state[src];handled.add(dst)
            elif i.mnemonic in ('inc','dec') and dst in state:
                old,new=state[dst];delta=-1 if i.mnemonic=='inc' else 1
                target=(old+delta)&0xffffffff;converted=(new+delta)&0xffffffff
                if target in names and names[target][0]!=converted:
                    issues.append(dict(instruction=pc,error='Implicit increment crosses a changed named distance',
                        target=target,original_anchor=old,mcc_anchor=new,mcc_target=names[target][0]))
                state[dst]=(target,converted);handled.add(dst)
            # CMP/TEST and control transfers do not clobber the data registers.
            if i.mnemonic not in ('cmp','test'):
                for written in i.regs_access()[1]:
                    target=reg(i,written)
                    if target and target not in handled:state.pop(target,None)
            if i.mnemonic=='call':
                for target in ('eax','ecx','edx'):state.pop(target,None)
            if i.mnemonic.startswith('ret') or i.mnemonic in ('int3','ud2'):continue
            if i.mnemonic.startswith('j'):
                if ops and ops[0].type==2:queue.append((ops[0].imm,dict(state)))
                if i.mnemonic=='jmp':continue
            queue.append((pc+i.size,state))
    direct={x['instruction'] for b in inventory['bodies'] for x in b['literals']}
    folded=[row for pc,row in sorted(sites.items()) if pc not in direct]
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,
        inventory_sha256=hashlib.sha256((OUT/'inventory.json').read_bytes()).hexdigest(),
        changed_operands=[row for _,row in sorted(sites.items())],folded_operands=folded,issues=issues,
        audited_named_comparisons=comparisons,passed=not issues,runtime_changes=False)
    (OUT/'folded-operands.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(passed=not issues,changed_operands=len(sites),folded=folded,issues=issues)),flush=True)
if __name__=='__main__':main()
