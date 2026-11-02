#!/usr/bin/env python3
"""Owned positive World binding prefix using authentic02 pool/shape inputs."""
import inspect
import validate_native_pair_create_world_stage_tu15 as base
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 source=inspect.getsource(base.main);needle="  tree=ast.parse";assert source.count(needle)==1
 prepare='''  archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-flip-controller-original-v1465-20261101/iteration-02-native-pair-owned-release-input-vehicle-camera/required-native-pair-consumer-before/actual-native-before.tar.zst'
  assert sha(archive)=='05ce4baa6c8d8a71326a629ce93a25eca8259a5b52edd9f51fc62fd5b3cd90dd'
  reference=scratch/'reference';reference.mkdir();subprocess.run(['tar','--zstd','-xf',str(archive),'-C',str(reference)],check=True)
  image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN
  selected=[(0x41c3d0,64),(0x461518,64),(0x4638b8,24),(0x461960,16),(0x44aac8,4)]
  (reference/'selected-readonly.json').write_text(json.dumps(dict(original_sha256=NEW_PIN,regions=[dict(address=p,hex=image.read(p,n).hex())for p,n in selected]),indent=2)+'\\n')
  report.update(actual_before_archive=str(archive),archive_sha256=sha(archive),selected_readonly_sha256=sha(reference/'selected-readonly.json'),prefix_only=True,full_positive_World_physics_claimed=False)
''';source=source.replace(needle,prepare+needle)
 begin=source.index("  world=project/'src/physics/native_tu15_world_integration_residuals.cpp'");end=source.index('  original_sources=',begin)
 transform='''  world=project/'src/physics/native_tu15_world_integration_residuals.cpp'
  test=project/'tests/native_tu15_world_residuals_tests.cpp';text=test.read_text();needle='int main(){try{';assert text.count(needle)==1 or text.count('int main(int argc,char**argv){try{if(argc==2)return native_world_convex_factory_prefix(argv[1]);')==1;text=text.replace(needle,'#include "helpers/native_world_convex_factory_prefix_tu15.hpp"'+chr(10)+'int main(int argc,char**argv){try{if(argc==2)return native_world_convex_factory_prefix(argv[1]);');test.write_text(text)
  helper=project/'tests/helpers/native_world_convex_factory_prefix_tu15.hpp';helper.parent.mkdir(exist_ok=True);shutil.copyfile(ROOT/'tests/helpers/native_world_convex_factory_prefix_tu15.hpp',helper)
  cmake=cmake.replace('add_test(NAME native_world_residual_model_tu15','add_test(NAME native_world_convex_factory_prefix_tu15').replace('-- $<TARGET_FILE:native_tu15_world_residuals_tests>)','-- $<TARGET_FILE:native_tu15_world_residuals_tests> "'+str(reference)+'")')
''';source=source[:begin]+transform+source[end:];source=source.replace("'^native_world_residual_model_tu15$'","'^native_world_convex_factory_prefix_tu15$'");source=source.replace("native_paircreate_positive_confirmation_model=True,positive_factory_World_model_claimed=False", "native_paircreate_positive_confirmation_model=False,positive_factory_World_prefix_model_claimed=True,positive_factory_World_model_claimed=False");source=source.replace("scope='Private coherent World model; actual matching cached confirmation invokes native320EF0; positive nativefactory is independently physical40parentqualified, not a newWorldfactoryliveclaim'", "scope='Private coherent actualRootWorld source; nativefactory positive prefix at3210DE, using exact02BEFORE pool/authoredshape/transform DATA; no unconvertedprocess outcome or fullpositiveWorldphysics claim'")
 env=dict(base.__dict__,Xbe=Xbe,NEW_DEFAULT=NEW_DEFAULT,NEW_PIN=NEW_PIN,main=None);exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
