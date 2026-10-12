#!/usr/bin/env python3
"""Independently audit source bodies, map bytes, outputs and prior freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,sha,save,encoded,Xbe
PRIOR=ROOT/'analysis/multiplayer/integration/sound-submit-native/final-manifest.json'
PRIOR_PIN='b7d3de47f405ec45ea5d682ab4b7106ab4f65da83fd92a8d834a1d6efbc81ca3'
ERRATUM=ROOT/'analysis/multiplayer/integration/sound-submit-promotion-output-erratum/manifest.json'
ERRATUM_PIN='993cd08aa36edcafd94f4b232caf227e362e09031b53c5dcd3405e19dd13360a'
def prior_pins():
    assert sha(PRIOR)==PRIOR_PIN and sha(ERRATUM)==ERRATUM_PIN
    pins={str(PRIOR.relative_to(ROOT)):PRIOR_PIN,str(ERRATUM.relative_to(ROOT)):ERRATUM_PIN}
    for path,groups in ((PRIOR,('owned_current_files','evidence_files','prior_pins','original_inputs')),(ERRATUM,('observed_files','evidence_files','original_images'))):
        m=json.loads(path.read_text())
        for group in groups:
            for rel,q in m[group].items():
                value=q if isinstance(q,str)else q['sha256'];assert rel not in pins or pins[rel]==value;pins[rel]=value
    return pins

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();prior=prior_pins()
    for rel,pin in prior.items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==pin,rel
    repeats=[]
    for origin,repeat in (('paired','paired-repeat'),('measured','repeated')):
        for f in sorted((d/origin).rglob('*')):
            if not f.is_file():continue
            q=d/repeat/f.relative_to(d/origin);assert f.read_bytes()==q.read_bytes(),str(f)
            repeats.append(dict(file=str(f.relative_to(d)),repeat=str(q.relative_to(d)),sha256=sha(f)))
    for name in ('fixtures.tsv','fixtures.json'):
        assert(d/name).read_bytes()==(d/'export-repeat'/name).read_bytes()
        repeats.append(dict(file=name,repeat='export-repeat/'+name,sha256=sha(d/name)))
    rows=0
    for image,tag in ((OLD,'old'),(NEW,'u5')):
        x=Xbe(image)
        for f in list((d/'paired/instructions').glob(f'*-{tag}.asm'))+list((d/'measured/instructions').glob(f'{tag}-*.asm')):
            for line in f.read_text().splitlines():
                words=line.split();raw=bytes.fromhex(words[1]);assert x.read(int(words[0],16),len(raw))==raw,(f,line);rows+=1
    report=json.loads((d/'measured/report.json').read_text());fixture=json.loads((d/'fixtures.json').read_text())
    assert fixture['calls']==74318 and fixture['ordered_writes']==249740
    assert len(report['maps'])==46
    bank_pairs=0
    for old,new in zip(report['maps'][::2],report['maps'][1::2]):
        assert old['image']=='old'and new['image']=='u5'
        assert old['state']==new['state']and old['regions']==new['regions']and old['counter_arrays']==new['counter_arrays']
        assert old['final_ugh_hex']==new['final_ugh_hex']and old['final_arena_hex']==new['final_arena_hex']
        for a,b in zip(old['cases'],new['cases']):
            clean=lambda q:{k:([{z:v for z,v in s.items()if z!='actual_instruction'}for s in value]if k=='stores'else value)for k,value in q.items()}
            assert clean(a)==clean(b)
        bank_pairs+=1
        assert sha(Path(old['map']['path']))==old['map']['sha256']
    providers=json.loads((d/'paired/providers.json').read_text())
    for word in providers['words']:
        for image,prefix in ((OLD,'old'),(NEW,'new')):
            assert Xbe(image).read(int(word[prefix+'_va'],16),4).hex()==word[prefix+'_hex']
    for directory in ('native','native-repeat-run/native'):
        commands=json.loads((d/directory/'commands.json').read_text())
        assert all(q['exit_code']==0 for q in commands['commands'])and commands['fixture_sha256']==sha(d/'fixtures.tsv')
        for tag in ('normal-build','sanitizer-build','normal','sanitizer'):assert not(d/directory/f'{tag}.stderr').read_bytes()
    for tag in ('normal','sanitizer'):
        assert(d/'native'/f'{tag}.stdout').read_bytes()==(d/'native-repeat-run/native'/f'{tag}.stdout').read_bytes()
    packet=dict(schema='sound-promotion-native-independent-audit-v1',audit_tool_sha256=sha(__file__),prior_pins_verified=prior,independent_byte_equal_files=repeats,original_image_instruction_rows_checked=rows,old_u5_equal_full_bank_pairs=bank_pairs,paired_report_sha256=sha(d/'paired/report.json'),original_report_sha256=sha(d/'measured/report.json'),fixture_sha256=sha(d/'fixtures.tsv'),normal_and_sanitizer_pass=True,no_counter_clock_selector_or_platform_results_supplied=True,native_audio_connected=False)
    save(d/'audit.json',encoded(packet));print(json.dumps(dict(audit_sha256=sha(d/'audit.json'),prior_pins=len(prior),independent_equal_files=len(repeats),original_instruction_rows=rows,bank_pairs=bank_pairs,mismatches=0)))
if __name__=='__main__':main()
