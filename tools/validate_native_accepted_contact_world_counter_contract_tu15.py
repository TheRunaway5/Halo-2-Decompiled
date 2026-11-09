#!/usr/bin/env python3
"""Exercise exact accepted listener getter, both JSON projections and required gates."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess
ROOT=Path(__file__).resolve().parents[1]
WORLD=ROOT/'analysis/physics/native-accepted-body-contact-world-cohort1482-v4-20261103'
GLUE=ROOT/'analysis/physics/native-accepted-body-contact-world-stage-draft-v3-20261103'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 fields=json.loads((GLUE/'fields.json').read_text());assert len(fields)==46
 whole=json.loads((WORLD/'source-manifest.json').read_text())['source_files_sha256']
 def exact(n):
  q=a.source_root/n;assert sha(q)==whole[n],n;return q.read_text()
 live=exact('src/physics_live_world_tu15.cpp');host=exact('tests/physics_multiplayer_acceptance.cpp');head=exact('include/halo2/native_tu15_physics_frame_residuals.hpp')
 start='    const auto&accepted=world_integration.accepted_body_contact;';stop='    result.native_moving_process_attempts='
 assert live.count(start)==live.count(stop)==1
 getter=live[live.index(start):live.index(stop)]
 prefix='native_accepted_contact_';owner_map=prefix+'refused_owners'
 first='        {"'+prefix+'attempts",'
 endings='        {"native_bsp_association_association_records",'
 positions=[m.start()for m in re.finditer(re.escape(first),host)];assert len(positions)==2
 serializers=[host[i:host.index(endings,i)]for i in positions];assert serializers[0]==serializers[1]
 gi=host.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_ACCEPTED_BODY_CONTACT")')
 gate=host[gi:host.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_BSP_OBJECT_ASSOCIATION"))',gi)]
 envs=list(dict.fromkeys(re.findall(r'getenv\("(HALO2_REQUIRE_NATIVE_ACCEPTED_\w+)"\)',gate)));assert len(envs)==4
 codes=['native_accepted_body_contact_gate','native_accepted_contact_rows_gate','native_accepted_contact_body_update_gate','native_accepted_contact_lookup_gate']
 seeds=[];checks=[]
 for i,(k,v) in enumerate(fields.items(),1):
  assert head.count('std::uint64_t '+prefix+k+'{};')==1
  assert getter.count('result.'+prefix+k+'=')==1
  parts=v.split('+')
  for j,n in enumerate(parts):seeds.append(f'{n}={100*i+j}u;')
  checks.append(f'check(s.{prefix+k}=={sum(100*i+j for j in range(len(parts)))}u,"getter {k}");')
 owners=['listener.unqualified_owners','publication.unqualified_owners','body_update.update.unqualified_owners','native_only.owners','body_update.native_only.owners','metadata.unqualified_owners']
 seeds += [f'accepted.helper.{n}[0x281710u]={i}u;' for i,n in enumerate(owners,1)]
 cases=[
 ('zero_helper_entries','s.native_accepted_contact_helper_entries=0;',0),('unbalanced_parent','s.native_accepted_contact_parent_completed=0;',0),
 ('disabled_or_flag1_only','s.native_accepted_contact_publication_requests=s.native_accepted_contact_publication_calls=s.native_accepted_contact_publication_completed=s.native_accepted_contact_material_pairs=0;',0),
 ('nonzero_role_refusal','s.native_accepted_contact_role_refusals=1;',0),('nonzero_resource_refusal','s.native_accepted_contact_entry_resource_refusals=1;',0),
 ('native_scope_refusal','s.native_accepted_contact_native_scope_refusals=1;',0),('named_unknown','s.native_accepted_contact_refused_owners[0x3200b0u]=1;',0),
 ('row_attempt_only','s.native_accepted_contact_row_initializations=0;',0xd),('impossible_row_count','s.native_accepted_contact_row_initializations=2;',0xd),
 ('body_attempt_only','s.native_accepted_contact_body_update_calls=0;',0xb),('lookup_attempt_only','s.native_accepted_contact_lookup_completions=0;',0x7),
 ('metadata_requested_not_started','s.native_accepted_contact_metadata_update_requests=1;',0),('metadata_incomplete','s.native_accepted_contact_metadata_update_requests=s.native_accepted_contact_metadata_attempts=1;',0),
 ('metadata_refused','s.native_accepted_contact_metadata_refusals=1;',0)]
 positives={'attempts':1,'calls':1,'helper_entries':1,'helper_completed':1,'parent_calls':1,'parent_completed':1,'publication_requests':1,'publication_calls':1,'publication_completed':1,
 'material_pairs':1,'row_append_requests':1,'row_initializations':1,'body_update_requests':1,'body_update_entries':1,'body_update_calls':1,'lookups':2,'lookup_completions':2}
 literals=lambda a:','.join(json.dumps(x)for x in a)
 code='''#include "halo2/native_tu15_world_integration.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;using J=nlohmann::json;
void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
'''
 type_match=re.search(r'struct (\w+)\s*\{[\s\S]*?native_accepted_contact_attempts',head)
 # Locate the exact containing structure without assuming the first struct.
 structures=list(re.finditer(r'struct (\w+)\s*\{',head[:head.index('native_accepted_contact_attempts')]))
 assert structures;stats=structures[-1].group(1)
 code+=f'using Stats={stats};\n'
 code+='Stats project(const auto&world_integration){Stats result{};\n'+getter+'return result;}\n'
 for i,ser in enumerate(serializers):code+=f'J json{i}(const Stats&native_frames){{return J{{\n'+ser+'};}\n'
 code+=f'const char*envs[]={{{literals(envs)}}};const char*keys[]={{{literals(codes)}}};\n'
 code+='unsigned evaluate(const Stats&native_frames,unsigned enable){for(unsigned i=0;i<4;++i){unsetenv(envs[i]);if((enable>>i)&1u)setenv(envs[i],"1",1);} bool passed=true;J report;\n'+gate+'''
 unsigned bits=0;for(unsigned i=0;i<4;++i){if((enable>>i)&1u){check(report.contains(keys[i]),"missing gate");if(report.at(keys[i])==true)bits|=1u<<i;}else check(!report.contains(keys[i]),"unrequested gate");}
 check(passed==(bits==enable),"gate aggregate");return bits;}
'''
 code+='int main(){try{struct Outer {NativeTu15AcceptedBodyContactWorldActivity accepted_body_contact;} world;auto&accepted=world.accepted_body_contact;\n'
 code+='\n'.join(seeds)+'\nconst auto s=project(world);\n'+'\n'.join(checks)
 code+=f'\ncheck(s.{owner_map}.size()==1&&s.{owner_map}.at(0x281710u)==21,"owner map merge");\n'
 code+=f'const auto first=json0(s),second=json1(s);check(first==second&&first.size()==47&&first.at("{owner_map}")==J(s.{owner_map}),"two JSON maps");\n'
 code+='\n'.join(f'check(first.at("{prefix}{k}")==s.{prefix}{k},"JSON {k}");'for k in fields)
 code+='\nStats positive{};'+''.join(f'positive.{prefix}{k}={v}u;'for k,v in positives.items())
 code+='\ncheck(evaluate(positive,15)==15,"positive without optional scaling");check(evaluate({},0)==0,"all opt-in");'
 code+='\nfor(unsigned i=0;i<4;++i)check(evaluate(positive,1u<<i)==(1u<<i),"standalone gate");'
 code+='\n{auto s=positive;s.native_accepted_contact_metadata_update_requests=s.native_accepted_contact_metadata_attempts=s.native_accepted_contact_metadata_completed=1;check(evaluate(s,15)==15,"balanced optional metadata");}'
 code+='\n'+'\n'.join(f'{{auto s=positive;{mutation}check(evaluate(s,15)=={expected}u,"{label}");}}'for label,mutation,expected in cases)
 code+='\nreturn 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\\n";return 1;}}\n'
 test=owner/'exact-counter-contract.cpp';test.write_text(code);(out/'exact-counter-contract.cpp').write_text(code)
 report=dict(passed=False,owned_scratch=str(owner),whole_source_manifest_sha256=sha(WORLD/'source-manifest.json'),exact_getter_two_json_and_gates=True,scalar_fields=len(fields),owner_maps=2,negative_controls=[x[0]for x in cases],compilers=[],retained_large_artifacts=[])
 try:
  for compiler in ['g++','clang++']:
   exe=owner/('counter-'+compiler);cmd=[compiler,'-std=c++20','-O0','-I'+str(a.source_root/'include'),str(test),'-o',str(exe)]
   run=subprocess.run(cmd,capture_output=True,text=True);(out/(compiler+'-build.log')).write_text(run.stdout+run.stderr);assert run.returncode==0,run.stderr[-3500:]
   run=subprocess.run([str(exe)],capture_output=True,text=True);assert run.returncode==0,run.stderr
   report['compilers'].append(dict(compiler=compiler,exit_code=0,command=cmd))
  report['passed']=True
 finally:
  report['allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file())
  (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
