#!/usr/bin/env python3
"""Old and new native-service APIs on the same genuine pair-wake inputs."""
import inspect,os
from pathlib import Path
import validate_native_pair_create_island_wake_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 stage=Path(os.environ['HALO2_NATIVE_ACTIVATION_FACTORY_SERVICES_STAGE']).resolve();outer=inspect.getsource(parent.main)
 anchor="    env=dict(parent.__dict__);env['main']=None;exec(text,env);return env['main']()";assert outer.count(anchor)==1
 code='''    transform+='shutil.copyfile(Path('''+repr(str(stage/'staged/src/physics/native_tu15_object_activation_residuals.cpp'))+'''),project/"src/physics/native_tu15_object_activation_residuals.cpp");shutil.copyfile(Path('''+repr(str(stage/'staged/include/halo2/native_tu15_object_activation_residuals.hpp'))+'''),project/"include/halo2/native_tu15_object_activation_residuals.hpp");'
'''
 # The transform is injected into the generated parent after its existing
 # source copies; only these two owned overlay files replace frozen source.
 extra='shutil.copyfile(Path('+repr(str(stage/'staged/src/physics/native_tu15_object_activation_residuals.cpp'))+'),project/"src/physics/native_tu15_object_activation_residuals.cpp");shutil.copyfile(Path('+repr(str(stage/'staged/include/halo2/native_tu15_object_activation_residuals.hpp'))+'),project/"include/halo2/native_tu15_object_activation_residuals.hpp");'
 extra+='driver='+repr('#include "halo2/native_tu15_pair_island_wake_native_factory_binding.hpp"\n')+'+driver;'
 old='native_tu15_activate_pair_island(child_memory,world,island,fp,0,\n        {0x4002020u,0x4002024u,0x4002040u,0x4002080u,0x800u},{},&activity,&activation);'
 new='''if(poison&1u)native_tu15_activate_pair_island_with_native_factories(child_memory,world,island,fp,0,
        {0x4002020u,0x4002024u,0x4002040u,0x4002080u,0x800u},{},{},&activity,&activation);
      else native_tu15_activate_pair_island(child_memory,world,island,fp,0,
        {0x4002020u,0x4002024u,0x4002040u,0x4002080u,0x800u},{},&activity,&activation);'''
 extra+='assert driver.count('+repr(old)+')==1;driver=driver.replace('+repr(old)+','+repr(new)+');'
 capture=ROOT/'analysis/physics/native-pair-wake-factory-capture-stage-20261102/revision-02/staged/include/halo2'
 for name in('native_tu15_pair_island_wake_binding.hpp','native_tu15_contact_agent_factory_input_observer.hpp'):
  extra+='shutil.copyfile(Path('+repr(str(capture/name))+'),project/"include/halo2/'+name+'");'
 marker='    text=text.replace("(project/\'driver.cpp\').write_text(driver)",extra+transform+"(project/\'driver.cpp\').write_text(driver)")'
 assert outer.count(marker)==1;outer=outer.replace(marker,'    transform+='+repr(extra)+'\n'+marker)
 additions=[str(stage/'staged/src/physics/native_tu15_object_activation_residuals.cpp'),str(stage/'staged/include/halo2/native_tu15_object_activation_residuals.hpp'),'include/halo2/native_tu15_pair_island_wake_native_factory_binding.hpp','tools/validate_native_activation_factory_services_interface_tu15.py']
 injection='    text=text.replace('+repr("'tools/validate_native_pair_create_tu15.py'")+','+repr("'tools/validate_native_pair_create_tu15.py',"+','.join(repr(x)for x in additions))+')\n'
 outer=outer.replace(anchor,injection+anchor)
 env=dict(parent.__dict__);env['main']=None;exec(outer,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
