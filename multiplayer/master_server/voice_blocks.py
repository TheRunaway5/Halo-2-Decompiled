"""Private client-supplied Discord voice preferences; Rust owns all policy.

Verified provider identity, HTTP parsing and SQLite execution remain typed IO.
The block list is a preference snapshot, not an independently verified graph.
"""
import time
from aiohttp import web
from .native_party_directory import run


class VoiceBlocks:
    def __init__(self, directory):
        self.directory,self.db=directory,directory.store.db
        self.wall,self.clock=time.time,time.monotonic
        run(self,'schema',family='voice')

    def subject(self,who):return run(self,'subject',who,family='voice')
    def allowed(self,a,b):return run(self,'allowed',a,family='voice',target=b)

    async def update(self,request):
        body=await self.directory.body(request,limit=48*1024)
        provider,subject=await self.directory.owner(request)
        result=run(self,'update',family='voice',body=body,verified_owner=dict(provider=provider,subject=subject))
        return web.json_response(result)
