#!/usr/bin/env python3
"""Compare complete original world segment queries on the death world's banks.

The original query executes again against every captured initial bank. Every
store, including query temporaries, must match before exporting a fixture.
No terrain, instance, posed-object, shape or hit result is replayed.
"""
import export_mp_unit_animation_pose_runtime as exporter
exporter.ROOTS={0x16b240}
exporter.ARGUMENT_COUNTS[0x16b240]=6
exporter.CAPTURE_ENTRY='0016b240'
exporter.CAPTURE_ORACLE='oracle_mp_death_segment_collision_1_1.py'
exporter.CONFIGURATION_TOOL=__file__
exporter.__doc__=__doc__
if __name__=='__main__':exporter.main()
