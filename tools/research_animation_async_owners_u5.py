#!/usr/bin/env python3
"""Complete original async constructor, worker and XAPI owner correspondence."""
import json
import research_unit_pose_requests_u5 as original
original.ENTRIES=(0x1221b0,0x121fd0,0x122020,0x1220e0,0x122150,0x122350,0x122360,
 0x2dc83a,0x2dc7b9,0x2d8606,0x2d83f6,0x2dc9c8,0x2dc8b2,0x2dc892,0x2dc814,
 0x2d84c4,0x2d837a,0x2d8532,0x2d856e)
save=original.save
def scoped_save(path,raw):
 if path.name=='report.json':
  value=json.loads(raw);value['comparator_dependency_sha256']=value['tool_sha256'];value['tool_sha256']=original.sha(__file__)
  value['scope']='Complete actual async bank/worker/XAPI direct owner static correspondence. Unseeded122350/2D856E come from original constructor function pointers. No handles, wait/release/thread/priority/IO success or worker schedule supplied.'
  raw=original.encoded(value)
 save(path,raw)
original.save=scoped_save
if __name__=='__main__':original.main()
