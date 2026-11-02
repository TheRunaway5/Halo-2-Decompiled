#!/usr/bin/env python3
"""Authenticate current native constant class helpers and source-pin reuse."""
import argparse,hashlib,json,os,re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN
 names=('src/physics/native_tu15_contact_agent_factory.cpp','include/halo2/native_tu15_contact_agent_factory.hpp','src/physics/native_tu15_activation_services.cpp','include/halo2/native_tu15_activation_services.hpp');pins={n:sha(ROOT/n)for n in names};text=(ROOT/names[2]).read_text();start=text.index('std::optional<std::uint32_t> native_tu15_activation_type(');end=text.index('std::uint32_t native_tu15_activation_group_filter(',start);body=text[start:end];pairs=[(int(owner,0),int(value,0))for owner,value in re.findall(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+|[0-9]+)u?;',body)];assert len(pairs)==body.count('case ')
 source=(ROOT/names[0]).read_text()
 for value,owner in((23,0xa9520),(24,0xab4b0)):
  assert source.count('std::uint32_t native_tu15_contact_child_class_'+str(value)+'() noexcept{return '+str(value)+';}')==1;pairs.append((owner,value))
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');rows=[]
 for owner,value in pairs:
  instructions=[code.at(pc)for pc in sorted(complete_body(code,owner).code)];assert len(instructions)==2 and instructions[1].mnemonic=='ret';i=instructions[0]
  if i.mnemonic=='mov'and i.op_str.startswith('eax, '):actual=i.operands[1].imm&0xffffffff
  elif i.mnemonic in('xor','sub')and i.op_str=='eax, eax':actual=0
  elif i.mnemonic=='or'and i.op_str in('eax, 0xffffffff','eax, -1'):actual=0xffffffff
  else:raise AssertionError((hex(owner),i.mnemonic,i.op_str))
  assert actual==value,(hex(owner),value,actual);rows.append(dict(owner=owner,result=value,body_sites=2,bytes=b''.join(i.bytes for i in instructions).hex(),no_data_reads=True,no_fp_or_simd=True))
 receipts=('build/test-results/zanzibar-ghost-og-20261101/native-bv-factories-physical-v2/report.json','build/test-results/zanzibar-ghost-og-20261101/original-getter-physical/report.json','build/test-results/vehicle-destruction-list-contact-agent-20261101/v3/report.json','build/test-results/vehicle-destruction-list-contact-agent-20261101/collection-v2/report.json')
 reuse=[]
 for name in receipts:
  r=json.loads((ROOT/name).read_text());source_pins=r.get('source_hashes',r.get('source_sha256',{}));selected={n:v for n,v in source_pins.items()if n.startswith(('src/physics/','include/halo2/'))};drift={n:dict(qualified=v,current=sha(ROOT/n))for n,v in selected.items()if sha(ROOT/n)!=v};reuse.append(dict(report=name,passed=r['passed'],runtime_source_drift=drift,selected_runtime_source_pins=selected))
 report=dict(passed=all(sha(ROOT/n)==v for n,v in pins.items()),original_sha256=NEW_PIN,source_hashes=pins,owners=rows,receipt_reuse=reuse,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],shape_constructor_admission_broadened=False,scope='Exact typed numeric constant getters only. Input-independent source2-site bodies; no physics memory/FP input or retained pointer. Normal source flags/register incidental values are not typed outputs. Actual selected owner/shape tuple and each factory/cache/geometry closure still require independent qualification.')
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
