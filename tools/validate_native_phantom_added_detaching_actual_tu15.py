#!/usr/bin/env python3
"""Continue actual1472 phantom deletion through qualified native C0F60."""
import inspect
import validate_native_phantom_added_actual_tu15 as parent
import validate_native_object_detachment_tu15 as detachment
def main():
 source=inspect.getsource(parent.main)
 source=source.replace("'tools/validate_native_phantom_added_actual_tu15.py','tools/validate_native_object_deletion_tu15.py'", "'tools/validate_native_phantom_added_actual_tu15.py','tools/validate_native_phantom_added_detaching_actual_tu15.py','include/halo2/native_tu15_object_deletion_detaching_binding.hpp','src/physics/native_tu15_object_detachment.cpp','include/halo2/native_tu15_object_detachment.hpp','tools/validate_native_object_deletion_tu15.py'")
 needle="meta['sites']+=extra['sites'];";assert source.count(needle)==1
 source=source.replace(needle,needle+"detachment_meta=detachment.metadata(image);seen={s['instruction']for s in meta['sites']};meta['sites']+=[s for s in detachment_meta['sites']if s['instruction']not in seen];meta['boundaries']=[p for p in meta['boundaries']if p!=0xc0f60]+detachment_meta['boundaries'];")
 needle="  (project/'driver.cpp').write_text(driver);";assert source.count(needle)==1
 setup='''  driver='#include "halo2/native_tu15_object_deletion_detaching_binding.hpp"\\n'+driver
  driver=driver.replace('NativeTu15ObjectDeletionBindingActivity binding;','NativeTu15ObjectDeletionDetachingBindingActivity binding;').replace('native_tu15_delete_bound_object(','native_tu15_delete_detaching_bound_object(')
  shutil.copyfile(ROOT/'src/physics/native_tu15_object_detachment.cpp',project/'detachment.cpp')
''';source=source.replace(needle,setup+needle)
 source=source.replace('add_executable(actual driver.cpp delete.cpp added.cpp activation.cpp)','add_executable(actual driver.cpp delete.cpp added.cpp activation.cpp detachment.cpp)')
 source=source.replace('captured_typed_input=typed,','captured_typed_input=typed,native_detachment_composed=True,')
 environment=dict(parent.__dict__,detachment=detachment,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
