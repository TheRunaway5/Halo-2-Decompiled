#!/usr/bin/env python3
"""Compare original biped action, recovery, object and movement producers.

Uses the frozen original natural-death prephysics capture and reexecutes its
unchanged authenticated bodies. No action, orientation, root motion, recovery
or object-update result is supplied by the native implementation.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0xe1800,0xe4f40,0xdf7c0,0x142e80,0x143f60,0x1d1290}
exporter.CAPTURE_ENTRY='000de960'
exporter.CAPTURE_ORACLE='oracle_mp_biped_prephysics_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
