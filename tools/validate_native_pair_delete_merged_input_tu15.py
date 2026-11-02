#!/usr/bin/env python3
"""Original3190D0-generated multi-contact inputs for native public320260."""
import inspect,os
import validate_native_pair_delete_native_children_tu15 as parent
def main():
 assert os.environ.get('HALO2_TEST_SCRATCH');outer=inspect.getsource(parent.main)
 preparation='''
 if(variant==9||variant==10){
   const auto owner=original_handler-0x1cu,world=original.word(original_handler+4u),active=original.word(world+8u),active_count=original.word(world+0xcu);
   check(original.word(owner+0x6cu)>0,"Source merge preparation requires genuine first contact island");
   std::uint32_t other{};for(unsigned n=0;n<active_count;++n){const auto candidate=original.word(active+n*4u);if(candidate!=owner&&original.word(candidate+0x6cu)>0&&original.word(candidate+0x40u)>0){other=candidate;break;}}
   check(other!=0,"Source merge preparation requires genuine second contact island");
   J preparation;Bridge prepare{original,sp-0x3000u,before_fp,preparation};NativeTu15PhysicsFrameFp state{before_fp.cwd,before_fp.swd,before_fp.mxcsr};
   const auto survivor=prepare.call(0x3190d0u,world,{owner,other},0x321053u,state);before_fp=original.fp();
   const auto count=original.word(survivor+0x6cu),rows=original.word(survivor+0x68u);check(count>=2&&count<128,"Original3190D0 did not produce a multi-contact island");
   const auto record=rows+(variant==9?count-1u:0u)*16u,first_handle=original.word(record),second_handle=original.word(record+4u);
   put(pair,first_handle);put(pair+4u,second_handle);handler=survivor+0x1cu;
   if(variant==10){
     const auto first=original.word(original.word(first_handle-0x10u+0x20u)+0x44u);
     if(first!=survivor)handler=first+0x1cu;
     else{const auto current_active=original.word(world+8u),current_count=original.word(world+0xcu);std::uint32_t third{};for(unsigned n=0;n<current_count;++n){const auto candidate=original.word(current_active+n*4u);if(candidate!=survivor){third=candidate;break;}}check(third!=0,"Persistent shift profile requires a second actual handler island");handler=third+0x1cu;}
   }
   put(handler+0x80u,variant==9?rows:rows+count*16u);put(handler+0x84u,rows+count*16u);put(handler+0x88u,0x4002000u);
   report["ordinary_original_merge_preparation"]=preparation;report["source_generated_contact_count"]=count;
 }
'''
 extra='driver=driver.replace("const auto before_fp=original.fp();","auto before_fp=original.fp();");driver=driver.replace("auto declared_entry=entry;",'+repr(preparation)+'+"auto declared_entry=entry;");'
 extra+='driver=driver.replace("std::ifstream meta_input(argv[5]);","if(variant==9||variant==10)banks.push_back({0x4002000u,original.read(0x4002000u,4096),{}});std::ifstream meta_input(argv[5]);");'
 anchor=" addition='\\n  '+extra+'\\n  '"
 assert outer.count(anchor)==1
 outer=outer.replace(anchor,' extra+='+repr(extra)+'\n'+anchor)
 anchor=" outer=outer.replace(anchor,extra_main+anchor)";assert outer.count(anchor)==1
 outer=outer.replace(anchor,' extra_main+='+repr(' text=text.replace("for poison in range(18):","for poison in range(22):")\n')+'\n'+anchor)
 env=dict(parent.__dict__);env['main']=None;exec(outer,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
