#!/usr/bin/env python3
"""Compare complete original DDBC0 physical input against native unit runtime.

Uses the frozen original natural-death prephysics capture, unchanged original
bodies and the previously declared actual-cache-byte delivery adapter. No
contact, physical mode, root motion, input packet or lower return is invented.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0xddbc0}
exporter.CAPTURE_ENTRY='000de960'
exporter.CAPTURE_ORACLE='oracle_mp_biped_prephysics_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
