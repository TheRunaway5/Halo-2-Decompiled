"""Temporary HTTP/SQLite IO for Rust-owned public PCR history reads.

Rust owns cursor/query validation, SQL, guest exclusion, source-index detail
selection and public field projection. Verified summary/accuracy coverage is
never expanded into invented weapon/medal/opponent detail. The shared current
profile statistics cache remains a separately recorded provider boundary.
"""
from aiohttp import web
from .native_database import run as database
from .native_party_directory import run

FORMAT = "native-verified-summary-v2"
ACCURACY_FORMAT = "native-verified-accuracy-v3"


class ReportHistory:
    def __init__(self, store):
        self.store,self.db=store,store.db
        database(self.db,'history_schema')

    def identity(self,player):
        return database(self.db,'history_identity',player=player)


class HistoryApi:
    def __init__(self,directory):
        self.directory,self.db=directory,directory.store.db
        # History has no lease/publication clock to advance. Observe the
        # supplied wall clock without writing the registry's durable floor.
        self.wall=directory.store.clock

    def read(self,mode,request,target=None):
        return run(self,mode,family='history',target=target,
                   features=list(request.query.items()))

    async def search(self,request):
        return web.json_response(self.read('search',request))

    async def matches(self,request):
        return web.json_response(self.read('matches',request,request.match_info['player']))

    async def report(self,request):
        return web.json_response(self.read('report',request,request.match_info['match']))

    def routes(self):
        return [web.get('/v1/history/players',self.search),
                web.get('/v1/history/players/{player}/matches',self.matches),
                web.get('/v1/history/matches/{match}',self.report)]
