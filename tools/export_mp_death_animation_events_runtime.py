#!/usr/bin/env python3
"""Compare original death animation event delivery and biped recovery requests.

Reexecutes unchanged C04F0/E8050, including the original E6670/E63C0 path,
against the frozen ordinary-death input bank. No event response, recovery
state, contact or effect result is supplied. Live world/effect owners remain
required if their original conditional branches are reached.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0xc04f0,0xe8050}
exporter.CAPTURE_ENTRY='000de960'
exporter.CAPTURE_ORACLE='oracle_mp_biped_prephysics_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
