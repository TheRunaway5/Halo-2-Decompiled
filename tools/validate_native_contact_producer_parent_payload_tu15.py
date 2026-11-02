#!/usr/bin/env python3
"""Qualify actual3043B5 typed payload-parent publication at its genuine phase."""
import inspect
import validate_native_surface_convex_factory_actual_tu15 as parent
UNITS=('native_tu15_contact_producer.cpp','native_tu15_contact_buffers.cpp',
       'native_tu15_contact_records.cpp','native_tu15_geometry.cpp','native_tu15_array_growth.cpp')
def main():
    source=inspect.getsource(parent.main)
    source=source.replace("candidate=args.stage/'staged/include/halo2/native_tu15_convex_agent_binding.hpp'","candidate=args.stage/'staged/src/physics/native_tu15_contact_producer.cpp'")
    source=source.replace('tools/observe_native_surface_convex_factory_actual_tu15.cpp','tools/observe_native_contact_producer_parent_payload_tu15.cpp')
    source=source.replace('tools/validate_native_surface_convex_factory_actual_tu15.py','tools/validate_native_contact_producer_parent_payload_tu15.py')
    before=";shutil.copyfile(candidate,project/'include/halo2/native_tu15_convex_agent_binding.hpp')"
    assert source.count(before)==1;source=source.replace(before,'')
    before="        shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp'"
    assert source.count(before)==1
    source=source.replace(before,"        shutil.copyfile(candidate,project/'native_tu15_contact_producer.cpp')\n"+before)
    source=source.replace('Actual native23/24 factory differs','Native typed-parent prefix differs')
    source=source.replace("'candidate_header_sha256'","'candidate_cpp_sha256'")
    before="            scope='Genuine Source304"
    begin=source.index(before);end=source.index("')\n    except",begin)
    source=source[:begin]+"            scope='Genuine304 prefix before2F, all89.8MB andFP, exact initialized16B caller DATA/read/store phase under4rounding modes andA5/5A native scratch. Native does not execute remaining children. Wholeproducer/303EE0 lifetime/World/live remainopen"+source[end:]
    environment=dict(parent.__dict__,__file__=__file__,UNITS=UNITS)
    exec(source,environment)
    return environment['main']()
if __name__=='__main__':raise SystemExit(main())
