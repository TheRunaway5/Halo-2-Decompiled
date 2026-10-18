#!/usr/bin/env python3
"""Add the authenticated382230 body without regenerating retained map owners.

Run generate_map_world_support_tu15.py to a separate output first. This checked
step takes only that complete new body and its three dispatcher registrations;
all existing owner bodies and native x87 helpers remain byte-exact. Repeating
the step on an already extended source is an idempotent verification.
"""
import argparse,hashlib,json,re
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def bodies(text):
    result={}
    for match in re.finditer(r'(?:static )?void run_([0-9a-f]+)\(WorldState &s\) \{',text):
        result[match[1]]=text[match.start():text.index('\n}',match.start())+2]
    return result

def extended(retained,fresh):
    old,new=bodies(retained),bodies(fresh)
    assert set(new)-set(old)<={'382230'} and set(old)<=set(new)
    assert len(set(old)-{'382230'})==188
    body=new['382230']
    assert re.findall(r'services\.instruction\(0x([0-9a-f]+)u\)',body)==['382230','382236','382238','38223e']
    assert 'native_fcos(s.fp.st(0),status)'in body and 'native_single_spill(s.fp.st(0),s.fp.status)'in body
    assert 'asm volatile("fcos\\n\\tfnstsw %1"'in retained
    assert 'std::uint32_t native_single_spill('in retained
    if '382230'in old:
        assert old['382230']==body,'Existing CRT owner differs from fresh authenticated lowering'
        return retained
    prototype='static void run_382230(WorldState &);\n'
    anchor='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){'
    assert retained.count(anchor)==1
    after=retained.replace(anchor,prototype+anchor+'if(target==0x382230u){run_382230(s);return;}',1)
    anchor='} // namespace\nvoid source_run_map_world_support_tu15'
    assert after.count(anchor)==1
    after=after.replace(anchor,body+'\n'+anchor,1)
    anchor='case 0x381770u:run_381770(s);break;'
    assert after.count(anchor)==1
    after=after.replace(anchor,anchor+'\ncase 0x382230u:run_382230(s);break;',1)
    anchor='case 0x381770u:return true;'
    assert after.count(anchor)==1
    after=after.replace(anchor,'case 0x381770u:case 0x382230u:return true;',1)
    updated=bodies(after)
    assert len(updated)==189 and all(updated[k]==value for k,value in old.items())
    return after

def main():
    a=argparse.ArgumentParser(description=__doc__)
    for name in ['source','generated','manifest','output','report']:a.add_argument('--'+name,type=Path,required=True)
    a=a.parse_args();sha=lambda b:hashlib.sha256(b).hexdigest()
    assert sha(NEW_DEFAULT.read_bytes())==NEW_PIN
    x=Xbe(NEW_DEFAULT);assert x.read(0x382230,15).hex()=='dd05a0ab4600d9ffd91d70b15600c3'
    assert x.read(0x46dc64,4).hex()=='30223800'and x.read(0x46aba0,8).hex()=='0000000000804640'
    before=a.source.read_text();fresh=a.generated.read_text();manifest=json.loads(a.manifest.read_text())
    assert manifest['later_original_sha256']==NEW_PIN and manifest['generated_cpp_sha256']==sha(fresh.encode())
    expected={'00382230':('fld','dd05a0ab4600'),'00382236':('fcos','d9ff'),'00382238':('fstp','d91d70b15600'),'0038223e':('ret','c3')}
    for site,(mnemonic,raw)in expected.items():
        fact=manifest['lowered_instructions'][site]
        assert fact['mnemonic']==mnemonic and fact['source_bytes']==raw
    after=extended(before,fresh)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(after)
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps({'reference':'English TU1.5 only','original_sha256':NEW_PIN,'entry':0x382230,'complete_original_bytes':15,'complete_original_instruction_sites':list(expected),'unchanged_retained_bodies':188,'before_sha256':sha(before.encode()),'after_sha256':sha(after.encode()),'fresh_full_generation_sha256':sha(fresh.encode()),'patch_generator_sha256':sha(Path(__file__).read_bytes()),'idempotent':after==before,'new_body_sha256':sha(bodies(after)['382230'].encode()),'native_x87_helpers_unchanged':True},indent=2)+'\n')
    print('PASS authenticated382230 extension;188 retained bodies unchanged')
if __name__=='__main__':main()
