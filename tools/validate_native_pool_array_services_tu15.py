#!/usr/bin/env python3
"""Qualify the shared native pool service composition on the full array oracle."""
import inspect
import validate_native_array_growth_tu15 as parent

def driver(text):
    text='#include "halo2/native_tu15_pool_array_services.hpp"\n'+text
    marker='      const auto host_sw='
    assert text.count(marker)==1
    setup='''      NativeTu15PoolArrayServiceActivity shared_activity;
      auto shared=native_tu15_make_pool_array_services(memory,observe,&shared_activity);
      growth.allocate=[&](const auto&r,auto&fp){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"bytes",r.bytes},{"memory_class",r.memory_class}});return shared.allocate(r,fp);};
      growth.release=[&](const auto&r,auto&fp){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"data",r.data},{"bytes",r.bytes},{"memory_class",r.memory_class}});shared.release(r,fp);};
'''
    text=text.replace(marker,setup+marker)
    before='check(calls==c.at("allocator_calls"),name+" actual allocator request order differs");'
    assert text.count(before)==1
    text=text.replace(before,'''J expected_calls=J::array();std::uint64_t expected_pool_allocations{},expected_pool_releases{};
      const auto direct_pool=c.at("entry")==0x233250u||c.at("entry")==0x233420u;
      for(const auto&call:c.at("allocator_calls")){const auto owner=call.at("owner").get<std::uint32_t>();if(owner==0x233250u)++expected_pool_allocations;if(owner==0x233420u)++expected_pool_releases;if(direct_pool||owner==0x149da0u||owner==0x149e00u)expected_calls.push_back(call);}
      check(calls==expected_calls,name+" actual allocator request order differs");
      if(!failed&&!required&&!direct_pool)check(shared_activity.pool_allocations==expected_pool_allocations&&shared_activity.pool_releases==expected_pool_releases,name+" native pool service completion counts differ");''')
    return text

def main():
    source=inspect.getsource(parent.main)
    marker="        units='src/physics/native_tu15_array_growth.cpp"
    assert source.count(marker)==1
    source=source.replace(marker,"        path=project/'tests/native_tu15_array_growth_tests.cpp';path.write_text(driver(path.read_text()))\n"+marker)
    marker='    finally:save()'
    assert source.count(marker)==1
    source=source.replace(marker,"    finally:\n        report['owned_scratch']=str(scratch);report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());report['source_inputs_unchanged']=all(sha(ROOT/name)==value for name,value in report['source_sha256'].items());report['passed']=report['passed']and report['source_inputs_unchanged'];save()")
    environment=dict(parent.__dict__,__file__=__file__,driver=driver,
        FILES=(*parent.FILES,'include/halo2/native_tu15_pool_array_services.hpp',
               'tools/validate_native_pool_array_services_tu15.py'))
    exec(source,environment)
    return environment['main']()

if __name__=='__main__':main()
