#!/usr/bin/env python3
"""Exact native-only production282C10 builder versus unchanged full parents."""
import inspect,json
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 text=inspect.getsource(parent.main);begin=text.index(" parent.NATIVE_BOUNDARIES='''");end=text.index(' text=inspect.getsource(parent.main)',begin);text=text[:begin]+" parent.NATIVE_BOUNDARIES=''\n"+text[end:]
 catch='''catch(const std::runtime_error&e){if(std::string(e.what())!="actual-island-owner-boundary")throw;}'''
 native_catch='''catch(const std::runtime_error&){if(c.at("boundary").is_null())throw;auto owners=activity.update.unbound_owners;for(const auto&[owner,n]:activity.state.unbound_owners)owners[owner]+=n;check(owners.size()==1&&owners.begin()->first==c.at("boundary").at("owner"),name+" native-only named boundary differs");boundary_hit=true;}
   check(activity.attempts==1&&activity.completed+activity.refusals==1,name+" native-only completion/refusal counters differ");'''
 changes=[('#include "halo2/native_tu15_dirty_island_objects.hpp"','#include "halo2/native_tu15_dirty_island_objects_binding.hpp"'),('NativeTu15DirtyIslandObjectServices services;','NativeTu15DirtyIslandObjectsBindingActivity activity;'),('native_tu15_process_dirty_island_objects(m,fp,0,services,observer,&stats);','native_tu15_process_bound_dirty_island_objects(m,fp,0,observer,&activity);'),(catch,native_catch)]
 transform=''.join('assert driver.count('+repr(a)+')==1;driver=driver.replace('+repr(a)+','+repr(b)+');'for a,b in changes)
 at=text.index(' extra=" text=text.replace(');text=text[:at]+" injected=injected.replace(\"(project/'driver.cpp').write_text(driver)\","+repr(transform+"(project/'driver.cpp').write_text(driver)")+")\n"+text[at:]
 environment=dict(parent.__dict__);exec(text,environment);code=environment['main']();path=Path(parent.parent.parent.sys.argv[parent.parent.parent.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());report['same_production_native_only_builder']=True;report['source_hashes'].update({str(p.relative_to(ROOT)):parent.parent.parent.sha(p)for p in(ROOT/'include/halo2/native_tu15_dirty_island_objects_binding.hpp',Path(__file__))});parent.parent.parent.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
