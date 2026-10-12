#!/usr/bin/env python3
"""Compare complete original1C7660 scheduling after ordinary death.

Reexecutes unchanged original bodies over their captured initial bank and
checks all original stores before exporting independent CPU estimate inputs.
No contact, body, pose, physical mode or solved answer is supplied.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0x1c7660}
exporter.ARGUMENT_COUNTS[0x1c7660]=0
exporter.CAPTURE_ENTRY='001c7660'
exporter.CAPTURE_ORACLE='oracle_mp_death_world_scheduler_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
