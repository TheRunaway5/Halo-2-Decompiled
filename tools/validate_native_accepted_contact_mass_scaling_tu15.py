#!/usr/bin/env python3
"""Physical TU1.5 accepted-contact parent with declared unequal inverse masses."""
import inspect
from pathlib import Path

import validate_native_accepted_body_contact_actual_tu15 as base
from validate_native_array_growth_tu15 import complete_body

ROOT = Path(__file__).resolve().parents[1]
ACCEPTED = ROOT / 'analysis/physics/native-accepted-body-contact-semantic-stage-v3-20261103/staged'
METADATA = ROOT / 'analysis/physics/native-created-constraint-metadata-stage-20261103/staged'


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def observer():
    text = (ROOT / 'tools/observe_native_accepted_body_contact_actual_tu15.cpp').read_text()
    text = replace(text, 'const std::filesystem::path&directory){',
                   'const std::filesystem::path&directory,const J&control){')
    write = 'request(PTRACE_SETFPREGS,c.pid,nullptr,&f);'
    text = replace(text, write, write + '''
 for(const auto&cell:control.at("cells")){
  const std::array<std::uint32_t,1>word{cell.at("bits").get<std::uint32_t>()};
  c.write(cell.at("address"),std::as_bytes(std::span(word)));
 }''')
    text = replace(text, 'for(char c:std::string(argv[5]))',
                   'for(const auto&control:meta.at("declared_controls"))for(char c:std::string(argv[5]))')
    text = replace(text, '{"mode",c-\'0\'}, {"fault_pc",0u},{"write",false}',
                   '{"mode",c-\'0\'}, {"fault_pc",0u},{"write",false},{"control",control}')
    for child in ('original', 'native'):
        text = replace(text, f'start({child},argv[1],typed,mode,directory);',
                       f'start({child},argv[1],typed,mode,directory,test.at("control"));')
    text = replace(text, 'const auto before_fp=original.fp();',
                   'const auto before_fp=original.fp();unsigned scales{},metadata{};')
    text = replace(text, 'const auto pc=std::uint32_t(regs.rip);',
                   'const auto pc=std::uint32_t(regs.rip);scales+=pc==0x31f460u;metadata+=pc==0x3200b0u;')
    text = replace(text, 'const auto expected_fp=original.fp();',
                   'check(scales==1&&metadata==1,"Declared original contact did not select 31F460 and 3200B0");const auto expected_fp=original.fp();')
    text = replace(text, '   services.publish_body_contact=', '''
   NativeTu15CreatedConstraintMetadataActivity metadata_activity;
   services.update_constraint=[&](auto&current,auto receiver,auto owner,auto,const auto&info,auto&state){
    check(&current==&view&&&state==&fp,"Scaled contact view or FP differs");
    native_tu15_update_created_constraint_metadata(current,receiver,owner,info,observer,&metadata_activity);
   };
   services.publish_body_contact=''')
    text = replace(text, '   if(!failed)check(parent.completed==1', '''
   check(parent.scale_queries==1&&parent.constraint_calls==1&&parent.constraint_updates==1&&
      metadata_activity.calls==1&&metadata_activity.completed==1&&!metadata_activity.refusals,
      "Native unequal-mass path did not complete selected constraint callback");
   if(!failed)check(parent.completed==1''')
    return replace(text, '{"actual_input_fp",mode==0},{"rounding_control",mode}',
                   '{"actual_input_fp",false},{"declared_control",test.at("control")},'
                   '{"source_scale_calls",scales},{"source_metadata_calls",metadata},{"rounding_control",mode}')


def main():
    text = inspect.getsource(base.main)
    text = replace(text, 'for pc in sorted(pcs):', '''for owner in (0x281710,0x184d30,0x184d80,0x31f460,0x3200b0):
   pcs.update(complete_body(code,owner).code)
  for pc in sorted(pcs):''')
    text = replace(text, 'save(scratch/\'metadata.json\',dict(sites=sites))', '''def motion(payload):
   while before.word(payload+0xc):payload=before.word(payload+0xc)
   assert before.word(payload+0x18)==1
   return before.word(before.word(payload+0x20)+0x3c)
  first=motion(before.event_fields['first_payload']);second=motion(before.event_fields['second_payload'])
  assert first!=second and before.word(first+0x2c)==before.word(second+0x2c)
  manager=before.event_fields['manager'];receiver=before.word(manager+0x24)
  assert before.word(manager+0x10)==before.word(0x48d1b0) and before.word(before.word(receiver)+8)==0x3200b0
  controls=[dict(name=name,cells=[dict(address=addr+0x2c,bits=bits)]) for name,addr,bits in (
   ('first_inverse_mass_1',first,0x3f800000),('second_inverse_mass_1',second,0x3f800000))]
  report['declared_controls']=controls
  save(scratch/'metadata.json',dict(sites=sites,declared_controls=controls))''')
    text = replace(text, "'include/halo2/native_tu15_retained_store_observer.hpp'];pins",
                   "'include/halo2/native_tu15_retained_store_observer.hpp','include/halo2/native_tu15_created_constraint_metadata_binding.hpp'];pins")
    text = text.replace('(DRAFT,GENERAL,OBSERVED,ROOT)', '(DRAFT,METADATA,GENERAL,OBSERVED,ROOT)')
    text = replace(text, "driver='#include", "driver='#include \"halo2/native_tu15_created_constraint_metadata_binding.hpp\"\\n#include")
    text = replace(text, "(ROOT/'tools/observe_native_accepted_body_contact_actual_tu15.cpp').read_text()", 'MASS_OBSERVER')
    text = text.replace("'tools/run_test.py');tool_pins", "'tools/run_test.py','tools/validate_native_accepted_contact_mass_scaling_tu15.py');tool_pins")
    text = replace(text, "assert all(sha(paths[n])==h for n,h in pins.items());source=", "assert all(sha(paths[n])==h for n,h in pins.items())and all(sha(ROOT/n)==h for n,h in tool_pins.items());source=")
    env = dict(base.__dict__, DRAFT=ACCEPTED, METADATA=METADATA, MASS_OBSERVER=observer(),
               UNITS=base.UNITS+('native_tu15_island_partition_metadata.cpp',), complete_body=complete_body,
               __doc__=__doc__+' Offline BEFORE controls only; no production or live admission.')
    exec(text, env)
    env['main']()


if __name__ == '__main__':
    main()
