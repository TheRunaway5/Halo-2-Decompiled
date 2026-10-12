#!/usr/bin/env python3
"""Compare complete original DEFC0 postphysics after ordinary death.

Reexecutes the unchanged phase against its natural source capture, checking
every original store including private ABI memory. No contact, support, pose,
physical mode or native expected result is supplied.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0xdefc0}
exporter.ARGUMENT_COUNTS[0xdefc0]=1
exporter.CAPTURE_ENTRY='000defc0'
exporter.CAPTURE_ORACLE='oracle_mp_biped_postphysics_phase_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
