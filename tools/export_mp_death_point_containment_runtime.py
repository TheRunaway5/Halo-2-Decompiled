#!/usr/bin/env python3
"""Compare complete original world/object point containment after death.

Reexecutes16A9C0 against each original captured bank and compares all writes.
No object, bound, node, model permutation or containment result is replayed.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0x16a9c0}
exporter.ARGUMENT_COUNTS[0x16a9c0]=4
exporter.CAPTURE_ENTRY='0016a9c0'
exporter.CAPTURE_ORACLE='oracle_mp_death_point_containment_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
