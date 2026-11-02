#!/usr/bin/env python3
"""Coherent staged World default/factory/merge plus declared pair-wake prefix."""
import inspect
import validate_native_world_island_merge_stage_tu15 as parent
def main():
 text=inspect.getsource(parent.main)
 needle="tests=[('default',''),"
 assert text.count(needle)==1
 text=text.replace(needle,"tests=[('pair_wake_prefix','pair-wake-prefix \\\"'+str(reference)+'\\\"'),('default',''),")
 text=text.replace('default|factory_prefix|merge_prefix','default|factory_prefix|merge_prefix|pair_wake_prefix')
 text=text.replace("'--jobs','2'", "'--jobs','1'")
 env=dict(parent.__dict__);env['main']=None;exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
