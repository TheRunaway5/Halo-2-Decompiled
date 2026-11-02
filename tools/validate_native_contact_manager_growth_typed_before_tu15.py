#!/usr/bin/env python3
"""Actual manager cleanup with genuine deferred-queue native growth/release.

No capacity/allocation outcome is supplied. Source hardware writes determine
the post-run activity assertions; the native API consumes only actual BEFORE.
"""
import inspect
import validate_native_contact_manager_base_lock_typed_before_tu15 as parent

def main():
 source=inspect.getsource(parent.main)
 source=source.replace('assert word(manager+0xa4)==0 and word(collector+4,2)&0x8000','assert word(manager+0xa4)==0\n    lock_selected=bool(word(collector+4,2)&0x8000)')
 start=source.index('    registry=word(0x48c93c)');end=source.index('    provenance=dict',start)
 registry=source[start:end]
 source=source[:start]+'    registry=mask=keys=None;visited=[]\n    if lock_selected:\n'+''.join('    '+line+'\n'for line in registry.rstrip().splitlines())+source[end:]
 source=source.replace('actual_registry_probe=visited,actual_registry_key_present=False,','actual_registry_probe=visited,actual_registry_key_present=False if lock_selected else None,base_lock_selected=lock_selected,')
 source=source.replace('receiver=manager,arguments=[flag],user_words=[],before_fp_input=', 'receiver=manager,arguments=[flag],user_words=[],base_lock_selected=lock_selected,before_fp_input=')
 needle='    source=inspect.getsource(child.main)';assert source.count(needle)==1
 source=source.replace(needle,needle+'''\n    source=source.replace("'tools/validate_native_agent_cleanup_children_tu15.py'", "'tools/validate_native_agent_cleanup_children_tu15.py','tools/validate_native_contact_manager_growth_typed_before_tu15.py'")
    needle="save(fixture/'original.json',"
    assert source.count(needle)==1
    counts="\\n  for case in all_cases:case['source_growths']=sum(w['instruction']==0x2e32b6 for w in case['writes']);case['source_deferred']=sum(w['instruction']==0x316dfa for w in case['writes']);case['source_unregistrations']=sum(w['instruction']==0x315d3e for w in case['writes'])\\n  "
    source=source.replace(needle,counts+needle)
''')
 source=source.replace('collector_stats.base_lock_releases==1u','collector_stats.base_lock_releases==c.at(\\"base_lock_selected\\").get<unsigned>()&&collector_stats.array_growths==c.at(\\"source_growths\\")&&collector_stats.deferred_references==c.at(\\"source_deferred\\")&&collector_stats.unregistrations==c.at(\\"source_unregistrations\\")')
 source=source.replace("target=project/'include/halo2/native_tu15_base_lock.hpp'", "target=project/'native_tu15_activation_contact_manager.cpp'")
 start=source.index("mutants=[('wrong_base_byte'");end=source.index("'''+source[end:]",start)
 source=source[:start]+'''mutants=[('wrong_deferred_ref','word(0x316dc6u,body+6u,word(body+6u,2)+1u,2)','word(0x316dc6u,body+6u,word(body+6u,2)+2u,2)'),('wrong_deferred_count','word(0x316dfau,header+4u,word(header+4u)+1u)','word(0x316dfau,header+4u,word(header+4u)+2u)')]'''+source[end:]
 source=source.replace("scope='Every genuine typed manager BEFORE bank", "scope='Actual deferred-queue growth: native31F660/316DC0 plus native2E3250/2E2E30/149DA0/149E00/pool, sourcehardware activitycounts, selectedbaseLock only ifgenuineflagandabsentregistry; every genuine typed manager BEFORE bank")
 environment=dict(parent.__dict__);exec(source,environment);return environment['main']()

if __name__=='__main__':raise SystemExit(main())
