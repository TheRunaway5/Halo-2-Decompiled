#!/usr/bin/env python3
"""Actual1472 full native added/deletion/detachment/spatial composition."""
import inspect
import validate_native_phantom_added_actual_tu15 as parent
import validate_native_object_detachment_tu15 as detachment
def main():
 source=inspect.getsource(parent.main)
 source=source.replace("'tools/validate_native_phantom_added_actual_tu15.py','tools/validate_native_object_deletion_tu15.py'", "'tools/validate_native_phantom_added_actual_tu15.py','tools/validate_native_phantom_added_spatial_actual_tu15.py','include/halo2/native_tu15_object_deletion_detaching_binding.hpp','src/physics/native_tu15_object_detachment.cpp','include/halo2/native_tu15_object_detachment.hpp','include/halo2/native_tu15_object_deletion_spatial_binding.hpp','src/physics/native_tu15_object_spatial_packet.cpp','include/halo2/native_tu15_object_spatial_packet.hpp','tools/validate_native_object_deletion_tu15.py'")
 needle="meta['sites']+=extra['sites'];";assert source.count(needle)==1
 after="detachment_meta=detachment.metadata(image);seen={s['instruction']for s in meta['sites']};meta['sites']+=[s for s in detachment_meta['sites']if s['instruction']not in seen];meta['boundaries']=[p for p in meta['boundaries']if p not in(0xc0f60,0xba910,0x1d30a0)]+detachment_meta['boundaries'];array.OWNERS=(0x16e290,0xba910,0x1d30a0,0x29fff0);spatial_meta,_=array.metadata(image,{});seen={s['instruction']for s in meta['sites']};meta['sites']+=[s for s in spatial_meta['sites']if s['instruction']not in seen];"
 source=source.replace(needle,needle+after)
 needle="  original_physical=inspect.getsource(deletion.physical.physical)";assert source.count(needle)==1
 setup='''  raw_observer=deletion.physical.observer_source
  def spatial_observer():
   text=raw_observer()
   text=text.replace('    for(const auto&a:site->second.at("operands")){','    if(!((pc==0x2a0030u||pc==0x2a0037u)&&registers[1]==0))for(const auto&a:site->second.at("operands")){')
   return text
  deletion.physical.observer_source=spatial_observer
  for site in meta['sites']:
   if deletion._Code(image,'.text').at(site['instruction']).mnemonic=='test':
    for operand in site['operands']:operand['read']=True
''';source=source.replace(needle,setup+needle)
 needle="  (project/'driver.cpp').write_text(driver);";assert source.count(needle)==1
 setup='''  driver='#include "halo2/native_tu15_object_deletion_spatial_binding.hpp"\\n'+driver
  driver=driver.replace('NativeTu15ObjectDeletionBindingActivity binding;','NativeTu15ObjectDeletionSpatialBindingActivity binding;').replace('native_tu15_delete_bound_object(','native_tu15_delete_spatially_bound_object(')
  shutil.copyfile(ROOT/'src/physics/native_tu15_object_detachment.cpp',project/'detachment.cpp')
  shutil.copyfile(ROOT/'src/physics/native_tu15_object_spatial_packet.cpp',project/'spatial.cpp')
''';source=source.replace(needle,setup+needle)
 source=source.replace('add_executable(actual driver.cpp delete.cpp added.cpp activation.cpp)','add_executable(actual driver.cpp delete.cpp added.cpp activation.cpp detachment.cpp spatial.cpp)')
 source=source.replace('captured_typed_input=typed,','captured_typed_input=typed,native_detachment_composed=True,native_spatial_composed=True,ordinary_host_packet_bytes=20,')
 environment=dict(parent.__dict__,detachment=detachment,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
