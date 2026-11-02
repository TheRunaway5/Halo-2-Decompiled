#!/usr/bin/env python3
"""Exact staged World release lambda against whole original232D20/232C80."""
import argparse,inspect,json,sys
from pathlib import Path
import validate_native_main_heap_release_binding_tu15 as parent
from build_native_tu15_constraint_before_reference import sha
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--stage',required=True,type=Path);a,remaining=p.parse_known_args();stage=a.stage.resolve()/'staged';world=stage/'src/physics/native_tu15_world_integration_residuals.cpp';text=world.read_text();begin=text.index('  pair_heap.release_owner=');end=text.index('  pair_heap.default_heap=',begin);callback=text[begin:end]
 needle='native_tu15_release_bound_main_heap_allocation(m,{c.at("allocator"),0x232d20u,c.at("data"),c.at("arguments")[1],c.at("arguments")[2]},fp,0,observer,&activity);'
 replacement='''[&]{NativeTu15WorldIntegrationStats world_activity;auto*stats=&world_activity;
      const struct{unsigned x87_depth;}state{0};NativeTu15ArrayAllocatorServices pair_heap;
      const auto&observe=observer;const auto unqualified_pair_service=[&](const std::string&why){++stats->native_pair_consumer_unqualified_services;throw std::runtime_error(why);};
'''+callback+'''      try{pair_heap.release_owner({c.at("allocator"),0x232d20u,c.at("data"),c.at("arguments")[1],c.at("arguments")[2]},fp);}
      catch(...){activity=world_activity.main_heap_release;throw;}activity=world_activity.main_heap_release;}();'''
 customize='driver='+repr('#include "halo2/native_tu15_world_integration.hpp"\n#include "halo2/native_tu15_array_allocator.hpp"\n#include "halo2/native_tu15_pair_pool_release_input_observer.hpp"\n')+'+driver;assert driver.count('+repr(needle)+')==1;driver=driver.replace('+repr(needle)+','+repr(replacement)+');'
 source=inspect.getsource(parent.main);at="(project/'driver.cpp').write_text(driver);variants={}";assert source.count(at)==1;source=source.replace(at,customize.replace('\\','\\\\')+at)
 anchor="text=inspect.getsource(base.main)";assert source.count(anchor)==1
 overlays=";text=text.replace("+repr("shutil.copytree(ROOT/'include',project/'include')")+","+repr("shutil.copytree(ROOT/'include',project/'include');shutil.copytree(stage/'include',project/'include',dirs_exist_ok=True)")+")"
 source=source.replace(anchor,anchor+overlays)
 source=source.replace("(project/'types.cpp').write_text('')","shutil.copyfile(ROOT/'src/physics/native_tu15_array_allocator.cpp',project/'types.cpp')")
 source=source.replace('environment=dict(base.__dict__,__file__=__file__,native_boundaries=native)','environment=dict(base.__dict__,__file__=__file__,native_boundaries=native,stage=stage)')
 environment=dict(parent.__dict__,stage=stage);sys.argv=[sys.argv[0],*remaining];exec(source,environment);code=environment['main']();out=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';report=json.loads(out.read_text());report.update(exact_world_callback_source=str(world),exact_world_callback_sha256=sha(world),native_only_world_service_composed=True);out.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');return code
if __name__=='__main__':raise SystemExit(main())
