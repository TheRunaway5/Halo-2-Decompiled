#!/usr/bin/env python3
"""Actual async request startup owners and changed BSP resource body, read-only."""
import json
import research_unit_pose_requests_u5 as original
original.ENTRIES=(0x1221b0,0x121fd0,0x1220e0,0x1a4870,0x19d8c0,0x2dc8b2,0x2dc9c8,0x2dc70a)
save=original.save
def scoped_save(path,raw):
 if path.name=='report.json':
  v=json.loads(raw);v['comparator_dependency_sha256']=v['tool_sha256'];v['tool_sha256']=original.sha(__file__)
  v['scope']='Complete static async request/owning startup and XAPI wait bodies. These are next required owners; no mutex/semaphore/thread/file/IO output is supplied or implemented here.'
  raw=original.encoded(v)
 save(path,raw)
original.save=scoped_save
if __name__=='__main__':original.main()
