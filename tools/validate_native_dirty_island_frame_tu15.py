#!/usr/bin/env python3
"""Exact staged native282C10 Frame builder and model in owned scratch."""
import inspect
import validate_native_memory_pressure_frame_binding_tu15 as parent

def main():
 text=inspect.getsource(parent.main)
 text=text.replace("for n in ['src/physics/native_tu15_memory_pressure.cpp','src/physics/native_tu15_contact_pressure.cpp']:","for n in ['src/physics/native_tu15_memory_pressure.cpp','src/physics/native_tu15_contact_pressure.cpp','src/physics/native_tu15_dirty_island_objects.cpp','src/physics/native_tu15_component_state.cpp']:")
 text=text.replace("'--jobs','2'","'--jobs','1'")
 text=text.replace("scope='Exact staged nativeGC", "scope='Exact staged native-only282C10")
 environment=dict(parent.__dict__);exec(text,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
