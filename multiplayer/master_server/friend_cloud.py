"""Versioned EOS private files; master transactions own consent and an outbox.

The dedicated storage client requires an EOS user. The trusted coordinator uses
its existing OpenID issuer to delegate an operation for the exact bound account;
it never substitutes a device ID, creates a new PUID, or exposes this token.
"""
import asyncio
import contextlib
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import tempfile
from .protocol import canonical, require

FORMAT = 'halo2-friends-v1'
CONFIG_KEYS = frozenset(('H2_EOS_PRODUCT_ID','H2_EOS_SANDBOX_ID','H2_EOS_DEPLOYMENT_ID',
    'H2_EOS_FRIEND_CLIENT_ID','H2_EOS_FRIEND_CLIENT_SECRET','H2_EOS_FRIEND_CACHE_DIR',
    'H2_EOS_FRIEND_ENCRYPTION_KEY'))


def private_config(path):
    """Read one bounded private regular file, without following a final symlink."""
    require(Path(path).is_absolute(),'EOS friend config must be an absolute private file',503)
    flags=os.O_RDONLY|getattr(os,'O_CLOEXEC',0)|getattr(os,'O_NOFOLLOW',0)|getattr(os,'O_NONBLOCK',0)
    with os.fdopen(os.open(path,flags),'rb') as source:
        info=os.fstat(source.fileno())
        require(stat.S_ISREG(info.st_mode) and info.st_size<=16384,'Invalid EOS friend config file',503)
        if os.name=='posix':
            require(info.st_uid in (0,os.geteuid()) and not info.st_mode&0o077,
                    'EOS friend config must be owner-only',503)
        raw=source.read(16385)
    require(len(raw)<=16384,'EOS friend config exceeds bound',503)
    result={}
    for line in raw.decode('utf-8').splitlines():
        line=line.strip()
        if not line or line.startswith('#'):continue
        key,separator,value=line.partition('=')
        require(separator and key in CONFIG_KEYS and key not in result and 1<=len(value)<=4096 and
                all(32<=ord(c)<127 for c in value),'Invalid EOS friend config',503)
        result[key]=value
    require(result.keys()==CONFIG_KEYS,'Incomplete EOS friend config',503)
    require(all(re.fullmatch('[0-9a-f]{32}',result[key]) for key in
        ('H2_EOS_PRODUCT_ID','H2_EOS_SANDBOX_ID','H2_EOS_DEPLOYMENT_ID')) and
        re.fullmatch('[0-9a-fA-F]{64}',result['H2_EOS_FRIEND_ENCRYPTION_KEY']) and
        Path(result['H2_EOS_FRIEND_CACHE_DIR']).is_absolute(),'Invalid EOS friend storage configuration',503)
    return result


class FriendCloud:
    def __init__(self,directory,*,upload=None):
        self.directory,self.db=directory,directory.store.db
        self.enabled=os.environ.get('H2_EOS_FRIEND_STORAGE')=='1'
        self.binary=os.environ.get('H2_EOS_WORKER','')
        self.config=os.environ.get('H2_EOS_FRIEND_STORAGE_CONFIG','')
        self.upload=upload or self.transfer
        self.wake=asyncio.Event()
        self.lock=asyncio.Lock()
        self.task=None
        self.process=None
        self.db.executescript("""
          CREATE TABLE IF NOT EXISTS friend_cloud_outbox(
            player TEXT PRIMARY KEY REFERENCES players(id), revision INTEGER NOT NULL,
            fingerprint TEXT NOT NULL, payload TEXT, digest TEXT NOT NULL,
            uploaded_revision INTEGER NOT NULL DEFAULT 0,
            next_attempt INTEGER NOT NULL DEFAULT 0, failures INTEGER NOT NULL DEFAULT 0,
            last_error TEXT NOT NULL DEFAULT '');
          CREATE INDEX IF NOT EXISTS friend_cloud_pending ON friend_cloud_outbox(next_attempt)
            WHERE revision>uploaded_revision;
        """)

    def enqueue(self,who):
        if not self.enabled:return
        snapshot=self.directory.friends.view(who)
        # Dormant legacy accounts receive tags only on their next verified login.
        if not snapshot['friend_tag']:return
        public=self.db.execute('SELECT public_id FROM history_players WHERE player=?',(who,)).fetchone()
        require(public is not None,'Friend cloud account is not published',500)
        state=dict(format=FORMAT,player=public[0],friend_tag=snapshot['friend_tag'],
                   relationships=sorted((dict(player=f['id'],state=f['state']) for f in snapshot['friends']),
                                        key=lambda f:f['player']))
        # Presence and mutable peer names are deliberately not persisted as facts.
        fingerprint=hashlib.sha256(canonical(state)).hexdigest()
        previous=self.db.execute('SELECT revision,fingerprint FROM friend_cloud_outbox WHERE player=?',(who,)).fetchone()
        if previous and previous[1]==fingerprint:return
        revision=previous[0]+1 if previous else 1
        payload=canonical(dict(**state,revision=revision)).decode()
        require(len(payload.encode())<=32768,'Friend cloud record exceeds bound',500)
        digest=hashlib.sha256(payload.encode()).hexdigest()
        self.db.execute('''INSERT INTO friend_cloud_outbox(player,revision,fingerprint,payload,digest) VALUES(?,?,?,?,?)
          ON CONFLICT(player) DO UPDATE SET revision=excluded.revision,fingerprint=excluded.fingerprint,
          payload=excluded.payload,digest=excluded.digest,next_attempt=0,failures=0,last_error='' ''',
          (who,revision,fingerprint,payload,digest))
        self.wake.set()

    def status(self,who):
        if not self.enabled:return dict(state='disabled')
        row=self.db.execute('SELECT revision,uploaded_revision,last_error FROM friend_cloud_outbox WHERE player=?',(who,)).fetchone()
        if not row:return dict(state='pending',revision=0,uploaded_revision=0)
        return dict(state='synced' if row[0]==row[1] else 'unavailable' if row[2] else 'pending',
                    revision=row[0],uploaded_revision=row[1])

    async def transfer(self,puid,token,payload):
        require(self.binary and self.config,'EOS friend storage is not configured',503)
        require(re.fullmatch('[0-9a-f]{32}',puid) and type(token) is str and 1<=len(token)<=8192 and
                all(32<ord(c)<127 for c in token) and type(payload) is str and len(payload.encode())<=32768,
                'Invalid EOS friend operation',503)
        data=canonical(dict(op='friend_store',user=puid,token=token,record=payload))+b'\n'
        require(len(data)<=65536,'EOS friend operation exceeds bound',503)
        config=private_config(self.config)
        executable=Path(self.binary)
        require(executable.is_absolute() and executable.is_file() and os.access(executable,os.X_OK),
                'EOS friend worker must be an absolute executable',503)
        cache=Path(os.environ.get('H2_EOS_FRIEND_CACHE_DIR',config['H2_EOS_FRIEND_CACHE_DIR']))
        require(cache.is_absolute(),'EOS friend cache must be absolute',503)
        cache.mkdir(mode=0o700,parents=True,exist_ok=True)
        info=cache.lstat()
        require(stat.S_ISDIR(info.st_mode) and (os.name!='posix' or
                (info.st_uid==os.geteuid() and not info.st_mode&0o077)),
                'EOS friend cache must be a private directory',503)
        # The worker does not inherit Steam keys, publisher credentials, private
        # master paths, preload hooks or a user's HOME. Only its SDK runtime and
        # this operation's dedicated config/cache cross the process boundary.
        env={key:os.environ[key] for key in ('PATH','LD_LIBRARY_PATH','SSL_CERT_FILE','SSL_CERT_DIR',
            'LANG','LC_ALL','TZ','SYSTEMROOT','WINDIR') if key in os.environ}
        with tempfile.TemporaryDirectory(prefix='sync-',dir=cache) as owned:
            env.update(H2_EOS_WORKER_MODE='friends',H2_EOS_FRIEND_STORAGE_CONFIG=self.config,
                H2_EOS_FRIEND_CACHE_DIR=owned,HOME=owned,TMPDIR=owned,TMP=owned,TEMP=owned,
                XDG_CACHE_HOME=owned,XDG_CONFIG_HOME=owned,XDG_DATA_HOME=owned)
            await self.run_worker(env,data,payload)

    async def retire(self,process):
        try:
            if process.returncode is None:
                with contextlib.suppress(ProcessLookupError):process.kill()
            await process.wait()
        finally:
            if self.process is process:self.process=None

    async def run_worker(self,env,data,payload):
        # Shield launch so cancellation cannot abandon a child between spawn
        # and assigning its handle. Reap it before releasing its private cache.
        launch=asyncio.create_task(asyncio.create_subprocess_exec(self.binary,stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.DEVNULL,limit=131072,env=env))
        try:process=await asyncio.shield(launch)
        except asyncio.CancelledError:
            try:process=await launch
            except Exception:pass
            else:await self.retire(process)
            raise
        self.process=process
        try:
            async with asyncio.timeout(130):
                process.stdin.write(data);await process.stdin.drain()
                for _ in range(64):
                    line=await process.stdout.readline()
                    if not line:raise ValueError('Storage worker exited')
                    if not line.startswith(b'H2EOS '):continue
                    reply=json.loads(line[6:])
                    require(type(reply) is dict and reply.get('ok') is True,'EOS friend storage unavailable',503)
                    value=reply['value']
                    require(type(value) is dict and value.get('record')==payload,'EOS friend storage read-back mismatch',503)
                    return
                raise ValueError('No storage worker reply')
        finally:
            await self.retire(process)

    async def flush(self):
        if not self.enabled:return False
        async with self.lock:
            row=self.db.execute('''SELECT o.player,o.revision,o.payload,o.digest,e.puid,p.screen_name
              ,o.failures FROM friend_cloud_outbox o JOIN eos_players e ON e.player=o.player
              JOIN player_profiles p ON p.player=o.player
              WHERE o.revision>o.uploaded_revision AND o.next_attempt<=? ORDER BY o.next_attempt,o.player LIMIT 1''',
              (self.directory.store.now(),)).fetchone()
            if not row:return False
            who,revision,payload,digest,puid,name,failures=row
            try:
                token=self.directory.eos_identity.token(who,name)
                await self.upload(puid,token,payload)
                with self.db:
                    binding=self.db.execute('SELECT puid FROM eos_players WHERE player=?',(who,)).fetchone()
                    require(binding and binding[0]==puid,'EOS friend binding changed',409)
                    current=self.db.execute('SELECT revision,digest FROM friend_cloud_outbox WHERE player=?',(who,)).fetchone()
                    if current and current[0]==revision and current[1]==digest:
                        self.db.execute("UPDATE friend_cloud_outbox SET uploaded_revision=?,payload=NULL,failures=0,last_error='' WHERE player=?",(revision,who))
                    # A newer transaction stays pending; its file follows this one.
            except asyncio.CancelledError:raise
            except Exception:
                with self.db:
                    self.db.execute("UPDATE friend_cloud_outbox SET failures=failures+1,last_error='unavailable',next_attempt=? WHERE player=? AND revision=?",
                        (self.directory.store.now()+min(900,30*(2**min(failures,5))),who,revision))
            return True

    async def run(self):
        while True:
            self.wake.clear()
            if await self.flush():continue
            try:
                async with asyncio.timeout(10):await self.wake.wait()
            except TimeoutError:pass

    async def start(self,_):
        if self.enabled:self.task=asyncio.create_task(self.run())

    async def close(self,_):
        if self.task:
            self.task.cancel()
            with contextlib.suppress(asyncio.CancelledError):await self.task
            self.task=None
