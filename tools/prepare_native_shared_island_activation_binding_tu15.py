#!/usr/bin/env python3
"""Stage one shared native activation-service owner for setters and World wakes."""
import argparse,difflib
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parent.parent
def once(text,needle,replacement):
 assert text.count(needle)==1,needle[:100];return text.replace(needle,replacement,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 cpp=Path('src/physics/native_tu15_object_activation_residuals.cpp');hpp=Path('include/halo2/native_tu15_object_activation_residuals.hpp')
 originals={p:(ROOT/p).read_text()for p in[cpp,hpp]};text=originals[cpp]
 start='''void native_tu15_set_object_velocities_with_residuals(SourcePhysicsContactMemory&memory,
 std::uint32_t object,const Vec3*linear,const Vec3*angular,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){'''
 text=once(text,start,'''namespace {
void with_activation_services(SourcePhysicsContactMemory&memory,unsigned depth,
 NativeTu15ObjectActivationScratch scratch,const NativeTu15ObjectActivationResidualCall&invoke,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ObjectActivationBindingActivity*activity,
 const std::function<void(const NativeTu15IslandActivationServices&)>&use){''')
 text=once(text,'    if(activity)++activity->setters;\n','')
 end='''    NativeTu15ObjectActivationContext context{fp,activation,observer,activity?&activity->activation:nullptr,depth};
    native_tu15_set_object_velocities(memory,object,linear,angular,context);
}
} // namespace halo2
'''
 text=once(text,end,'''    use(activation);
}
} // namespace
void native_tu15_set_object_velocities_with_residuals(SourcePhysicsContactMemory&memory,
 std::uint32_t object,const Vec3*linear,const Vec3*angular,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    if(activity)++activity->setters;
    with_activation_services(memory,depth,scratch,invoke,observer,activity,[&](const auto&activation){
        NativeTu15ObjectActivationContext context{fp,activation,observer,activity?&activity->activation:nullptr,depth};
        native_tu15_set_object_velocities(memory,object,linear,angular,context);
    });
}
void native_tu15_activate_island_with_residuals(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,
 unsigned depth,NativeTu15ObjectActivationScratch scratch,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    with_activation_services(memory,depth,scratch,invoke,observer,activity,[&](const auto&activation){
        native_tu15_activate_physics_island(memory,world,island,fp,activation,observer,
            activity?&activity->activation:nullptr);
    });
}
} // namespace halo2
''')
 header=once(originals[hpp],'} // namespace halo2\n','''// Actual316D40 using the SAME native agents/body-listener/constraint owner
// as B9800. This is a typed World operation, never a synthesized velocity call.
void native_tu15_activate_island_with_residuals(SourcePhysicsContactMemory&,
    NativeTu15PhysicsWorld,NativeTu15PhysicsIsland,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,NativeTu15ObjectActivationScratch,
    const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ObjectActivationBindingActivity* = nullptr);
} // namespace halo2
''')
 staged={cpp:text,hpp:header};patch=[]
 for relative,content in staged.items():
  output=a.output/'staged'/relative;output.parent.mkdir(parents=True,exist_ok=True);output.write_text(content)
  patch.extend(difflib.unified_diff(originals[relative].splitlines(True),content.splitlines(True),fromfile='a/'+str(relative),tofile='b/'+str(relative)))
 diff=a.output/'native-shared-island-activation.diff';diff.write_text(''.join(patch))
 save(a.output/'source-pins.json',dict(qualified=False,working_tree_modified=False,base_hashes={str(p):sha(ROOT/p)for p in staged},
  staged_hashes={str(p):sha(a.output/'staged'/p)for p in staged},patch=dict(path=str(diff),sha256=sha(diff)),
  source_hashes={str(Path(__file__).relative_to(ROOT)):sha(Path(__file__))},remaining=['Setter transport requalification','Actual World activation composition and live invocation']))
 print({str(p):sha(a.output/'staged'/p)for p in staged});return 0
if __name__=='__main__':raise SystemExit(main())
