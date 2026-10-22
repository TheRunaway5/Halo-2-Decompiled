"""EOS-purpose OpenID identity, preserving existing verified account ownership.

The native client uses our short-lived token with EOS Connect. Only the official
server SDK verifies the returned EOS token before we bind its PUID to a player.
"""
import asyncio
import base64
import hashlib
import json
import os
from pathlib import Path
import re
from aiohttp import web
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from .protocol import fields, require
from .player_statistics import enqueue


def b64(value):
    return base64.urlsafe_b64encode(value).decode().rstrip('=')


class EosIdentity:
    def __init__(self,directory):
        self.directory=directory
        self.audience=os.environ.get('H2_EOS_OPENID_AUDIENCE','')
        self.binary=os.environ.get('H2_EOS_WORKER','')
        self.key=None
        self.process=None
        self.lock=asyncio.Lock()
        self.pending=0
        if self.audience:
            require(1<=len(self.audience)<=128 and not directory.settings.allow_http,
                    'EOS OpenID requires a public HTTPS deployment',500)
            path=Path(directory.settings.state)/'eos-openid-key.pem'
            if not path.exists():
                key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
                data=key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption())
                try:
                    with path.open('xb') as output:
                        path.chmod(0o600);output.write(data)
                except FileExistsError:pass
            self.key=serialization.load_pem_private_key(path.read_bytes(),password=None)
            require(isinstance(self.key,rsa.RSAPrivateKey),'Invalid EOS signing key',500)
            der=self.key.public_key().public_bytes(serialization.Encoding.DER,serialization.PublicFormat.SubjectPublicKeyInfo)
            self.kid=hashlib.sha256(der).hexdigest()[:32]

    async def jwks(self,request):
        require(self.key is not None,'EOS identity is not configured',503)
        numbers=self.key.public_key().public_numbers()
        encode=lambda n:b64(n.to_bytes((n.bit_length()+7)//8,'big'))
        return web.json_response({'keys':[dict(kty='RSA',use='sig',alg='RS256',kid=self.kid,n=encode(numbers.n),e=encode(numbers.e))]})

    def token(self,who,name):
        require(self.key is not None,'EOS identity is not configured',503)
        now=self.directory.store.now()
        encode=lambda v:b64(json.dumps(v,separators=(',',':')).encode())
        header=encode(dict(alg='RS256',typ='JWT',kid=self.kid))
        payload=encode(dict(iss=self.directory.settings.public_url,sub=who,aud=self.audience,
                            name=name,iat=now,nbf=now-5,exp=now+120))
        message=(header+'.'+payload).encode()
        return message.decode()+'.'+b64(self.key.sign(message,padding.PKCS1v15(),hashes.SHA256()))

    async def issue(self,request):
        _,who=self.directory.clans.session(request)
        name=self.directory.store.db.execute('SELECT screen_name FROM player_profiles WHERE player=?',(who,)).fetchone()[0]
        with self.directory.store.db:
            token=self.token(who,name)
        return web.json_response(dict(token=token,expires_in=120))

    async def verify(self,puid,token):
        require(self.binary and self.key is not None,'EOS verification is not configured',503)
        require(self.pending<32,'EOS verification busy',429)
        self.pending+=1
        try:
            async with self.lock:
                try:
                    if self.process is None or self.process.returncode is not None:
                        self.process=await asyncio.create_subprocess_exec(self.binary,stdin=asyncio.subprocess.PIPE,
                            stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.DEVNULL,limit=262144)
                    async with asyncio.timeout(20):
                        self.process.stdin.write(json.dumps(dict(op='verify',user=puid,token=token)).encode()+b'\n')
                        await self.process.stdin.drain()
                        for _ in range(64):
                            line=await self.process.stdout.readline()
                            if not line:raise ValueError('Worker exited')
                            if not line.startswith(b'H2EOS '):continue
                            reply=json.loads(line[6:])
                            require(reply.get('ok') is True,'EOS identity proof rejected',403)
                            return reply['value']['account']
                        raise ValueError('No worker reply')
                except (OSError,TimeoutError,ValueError,KeyError):
                    await self.close(None)
                    require(False,'EOS verification unavailable',503)
        finally:self.pending-=1

    async def bind(self,request):
        _,who=self.directory.clans.session(request)
        body=await self.directory.body(request)
        fields(body,('puid','token'))
        require(type(body['puid']) is str and re.fullmatch(r'[0-9a-f]{32}',body['puid']) and
                type(body['token']) is str and 1<=len(body['token'])<=8192,'Invalid EOS identity proof')
        account=await self.verify(body['puid'],body['token'])
        require(account==who,'EOS identity belongs to another account',403)
        # Authentication can change during SDK verification; bind only while
        # this exact account session remains active.
        _,current=self.directory.clans.session(request)
        require(current==who,'Account session changed',401)
        db=self.directory.store.db
        with db:
            old=db.execute('SELECT puid FROM eos_players WHERE player=?',(who,)).fetchone()
            require(old is None or old[0]==body['puid'],'EOS identity migration requires explicit recovery',409)
            linked=db.execute('SELECT player FROM eos_players WHERE puid=?',(body['puid'],)).fetchone()
            require(linked is None or linked[0]==who,'EOS identity already linked',409)
            if old is None:
                db.execute('INSERT INTO eos_players VALUES(?,?)',(who,body['puid']))
                enqueue(db,who)
        return web.json_response({'linked':True})

    async def close(self,_):
        process,self.process=self.process,None
        if process is not None and process.returncode is None:
            process.kill();await process.wait()

    def routes(self):
        return [web.get('/v1/auth/eos/jwks',self.jwks),web.post('/v1/account/eos-token',self.issue),
                web.post('/v1/account/eos-proof',self.bind)]
