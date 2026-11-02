#!/usr/bin/env python3
"""Private reversed-process binding on exact1471 initial11v4, no Root edits."""
import argparse,difflib,json,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/physics/native-contact-point-reference-world-binding-stage-v4-20261102'
FIELDS={'native_reversed_contact_process_attempts':'reversed_attempts','native_reversed_contact_process_completions':'reversed_completed','native_reversed_contact_process_refusals':'reversed_refusals','native_reversed_wrapped_contact_process_calls':'reversed_wrapped.calls','native_reversed_compound_contact_process_calls':'reversed_compound.calls','native_reversed_contact_process_child_returns':'reversed_wrapped.child_returns+impl_->native_physics_integrations.contact_process.reversed_compound.child_returns','native_reversed_contact_normal_vectors':'reversed_wrapped.normals_reversed+impl_->native_physics_integrations.contact_process.reversed_compound.normals_reversed'}
FILES=['cmake/PhysicsLiveWorldTu15.cmake','include/halo2/native_tu15_contact_process_binding.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp']
def once(s,before,after,n=1):assert s.count(before)==n,(before,s.count(before));return s.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';shutil.copytree(BASE/'staged',stage);base_manifest=json.loads((BASE/'source-pins.json').read_text());assert all(sha(stage/f)==h for f,h in base_manifest['staged_hashes'].items());before={}
 for f in FILES:
  if not(stage/f).is_file():target=stage/f;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.source_root/f,target)
  before[f]=(stage/f).read_text()
 base_hashes={f:sha(stage/f)for f in FILES}
 for f in('include/halo2/native_tu15_reversed_contact_process.hpp','src/physics/native_tu15_reversed_contact_process.cpp'):target=stage/f;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/f,target)
 f=FILES[0];s=once(before[f],'    src/physics/native_tu15_wrapped_contact_process.cpp','    src/physics/native_tu15_wrapped_contact_process.cpp\n    src/physics/native_tu15_reversed_contact_process.cpp');(stage/f).write_text(s)
 f=FILES[1];s=once(before[f],'#include "halo2/native_tu15_wrapped_contact_process.hpp"','#include "halo2/native_tu15_wrapped_contact_process.hpp"\n#include "halo2/native_tu15_reversed_contact_process.hpp"');s=once(s,'    NativeTu15WrappedContactProcessStats wrapped;','    std::uint64_t reversed_attempts{},reversed_completed{},reversed_refusals{};\n    NativeTu15ReversedContactProcessStats reversed_wrapped,reversed_compound;\n    NativeTu15WrappedContactProcessStats wrapped;');s=once(s,'return owner.identity==0x2f6ca0u||owner.identity==0x2f7a50u;','return owner.identity==0x2f6ca0u||owner.identity==0x2f7a50u||\n            owner.identity==0x2f7480u||owner.identity==0x2f7fb0u;');s=once(s,'        if(stats_)++stats_->attempts;','        const bool reversed=call.owner.identity==0x2f7480u||call.owner.identity==0x2f7fb0u;\n        if(stats_){++stats_->attempts;if(reversed)++stats_->reversed_attempts;}');anchor='            if(call.owner.identity==0x2f6ca0u)\n';branch='''            if(call.owner.identity==0x2f7480u)
                native_tu15_process_reversed_wrapped_contact_agent(memory_,call.receiver,
                    {call.first.identity},call.second,call.configuration,call.output,
                    fp,children,observe_,stats_?&stats_->reversed_wrapped:nullptr);
            else if(call.owner.identity==0x2f7fb0u)
                native_tu15_process_reversed_compound_contact_agent(memory_,call.receiver,
                    {call.first.identity},call.second,call.configuration,call.output,
                    fp,children,observe_,stats_?&stats_->reversed_compound:nullptr);
            else if(call.owner.identity==0x2f6ca0u)
''';s=once(s,anchor,branch);s=once(s,'            if(stats_)++stats_->completed;','            if(stats_){++stats_->completed;if(reversed)++stats_->reversed_completed;}');s=once(s,'        }catch(...){if(stats_)++stats_->refusals;throw;}','        }catch(...){if(stats_){++stats_->refusals;if(reversed)++stats_->reversed_refusals;}throw;}');(stage/f).write_text(s)
 f=FILES[2];s=once(before[f],'  std::uint64_t native_contact_process_calls{}','  std::uint64_t '+','.join(n+'{}'for n in FIELDS)+';\n  std::uint64_t native_contact_process_calls{}');(stage/f).write_text(s)
 f=FILES[3];s=before[f];anchor='    result.native_contact_process_calls=impl_->native_physics_integrations.contact_process.attempts;';fragment=''.join('    result.'+name+'=impl_->native_physics_integrations.contact_process.'+path+';\n'for name,path in FIELDS.items());s=once(s,anchor,fragment+anchor);old='impl_->native_physics_integrations.contact_process.compound.child_calls;';new='impl_->native_physics_integrations.contact_process.compound.child_calls+impl_->native_physics_integrations.contact_process.reversed_wrapped.child_calls+impl_->native_physics_integrations.contact_process.reversed_compound.child_calls;';s=once(s,old,new);(stage/f).write_text(s)
 f=FILES[4];s=before[f];anchor='        {"native_contact_process_calls",native_frames.native_contact_process_calls},';rows=''.join('        {"'+n+'",native_frames.'+n+'},\n'for n in FIELDS);s=once(s,anchor,rows+anchor,2);anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_COMPOUND_CONTACT_PROCESS")) {';gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_REVERSED_CONTACT_PROCESS")||std::getenv("HALO2_REQUIRE_NATIVE_REVERSED_CONTACT_NORMALS")) {
    const bool reversed=native_frames.native_reversed_contact_process_attempts>0&&
      native_frames.native_reversed_contact_process_attempts==native_frames.native_reversed_contact_process_completions&&
      native_frames.native_reversed_contact_process_refusals==0&&native_frames.native_contact_process_refusals==0&&
      native_frames.native_reversed_contact_process_child_returns==native_frames.native_reversed_contact_process_completions&&
      !native_frames.residual_owner_calls.contains(0x2f7480u)&&!native_frames.residual_owner_calls.contains(0x2f7fb0u);
    report["native_reversed_contact_process_gate"]=reversed;passed&=reversed;
    if(std::getenv("HALO2_REQUIRE_NATIVE_REVERSED_CONTACT_NORMALS")) {
      const bool flipped=reversed&&native_frames.native_reversed_contact_normal_vectors>0;
      report["native_reversed_contact_normals_gate"]=flipped;passed&=flipped;
    }
  }
''';s=once(s,anchor,gate+anchor);(stage/f).write_text(s)
 patch=''.join(''.join(difflib.unified_diff(before[f].splitlines(True),(stage/f).read_text().splitlines(True),fromfile='a/'+f,tofile='b/'+f))for f in FILES);(a.output/'delta.patch').write_text(patch);result=dict(qualified=False,production_applied=False,live_qualified=False,base_source_root=str(a.source_root),base_stage=str(BASE),base_hashes=base_hashes,delta_files=FILES,fields=FIELDS,new_cpp='src/physics/native_tu15_reversed_contact_process.cpp',staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},public_positive_contacts_component_qualified=False,scope='Wholepublic39+39 kernels +independentpositive suffix. Native binding routes actual reversed typedchildren, retains304350/unknown child counts and allpointRef4/baseLock/regroup fields. No GP/privatepostimages or supplied childcontacts. Actualpublic generatedcontacts and fullnative childclosure remainopen.',remaining=['Sameproductionbuilder physicalproof','CoherentWorld/Getters/gates','Visiblemultiplayer originalframe and positiveflippedvector stimulus','Noncompound304350 child nativeclosure'])
 save(a.output/'source-pins.json',result);print(dict(prepared=True,files=len(result['staged_hashes']),modified_shared_files=len(FILES)));return 0
if __name__=='__main__':raise SystemExit(main())
