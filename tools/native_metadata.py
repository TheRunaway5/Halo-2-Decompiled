"""Lossless bounded storage for frozen-cohort JSON metadata.

Hashes identify decoded original bytes. Compression and optional gzip-stream
chunks affect storage only; old plain JSON remains readable without rewriting.
"""
import gzip
import hashlib
import io
import json
from pathlib import Path
import re
import tempfile

FILE_LIMIT=1_048_576
TOTAL_LIMIT=8*1_048_576
RESERVE=128*1024
DECODE_LIMIT=64*1_048_576
INDEX_SCHEMA='native-json-gzip-index-v1'

class MetadataBudgetError(RuntimeError):
    def __init__(self,message,retained_bytes):
        super().__init__(message);self.retained_bytes=retained_bytes

def resolve(path):
    path=Path(path)
    candidates=[p for p in(path,Path(str(path)+'.gz'),Path(str(path)+'.gz.index.json'))if p.is_file()]
    if len(candidates)!=1:
        if not candidates:raise FileNotFoundError(path)
        raise ValueError('Ambiguous plain/compressed metadata: '+str(path))
    return candidates[0]

def _gunzip(encoded,limit):
    with gzip.GzipFile(fileobj=io.BytesIO(encoded),mode='rb')as stream:
        data=stream.read(limit+1)
        if len(data)>limit:raise ValueError('Decoded metadata exceeds its bound')
        return data

def read_bytes(path,max_bytes=DECODE_LIMIT):
    actual=resolve(path)
    if actual.name.endswith('.gz.index.json'):
        if actual.stat().st_size>FILE_LIMIT:raise ValueError('Metadata index exceeds1MiB')
        index=json.loads(actual.read_text())
        if index.get('schema')!=INDEX_SCHEMA or index.get('encoding')!='gzip':raise ValueError('Unsupported metadata index')
        if index.get('logical_name')!=actual.name[:-len('.gz.index.json')]:raise ValueError('Metadata logical filename differs')
        if not isinstance(index.get('raw_bytes'),int)or not 0<=index['raw_bytes']<=max_bytes:raise ValueError('Invalid decoded metadata size')
        if not isinstance(index.get('stored_bytes'),int)or not 0<index['stored_bytes']<=max_bytes+FILE_LIMIT:raise ValueError('Invalid compressed metadata size')
        chunks=[];seen=set()
        for row in index['chunks']:
            name=row['name']
            if not isinstance(name,str)or Path(name).name!=name or name in seen or not re.fullmatch(re.escape(index['logical_name'])+r'\.gz\.part\d{4}',name):raise ValueError('Invalid metadata chunk name')
            seen.add(name);chunk=actual.parent/name
            if chunk.is_symlink()or not chunk.is_file()or not 0<chunk.stat().st_size<=FILE_LIMIT:raise ValueError('Invalid metadata chunk size')
            data=chunk.read_bytes()
            if len(data)!=row['bytes']or hashlib.sha256(data).hexdigest()!=row['sha256']:raise ValueError('Metadata chunk hash/size differs')
            chunks.append(data)
        if not chunks:raise ValueError('Empty metadata chunk index')
        encoded=b''.join(chunks)
        if len(encoded)!=index['stored_bytes']or hashlib.sha256(encoded).hexdigest()!=index['stored_sha256']:raise ValueError('Compressed metadata hash/size differs')
        data=_gunzip(encoded,min(max_bytes,index['raw_bytes']))
        if len(data)!=index['raw_bytes']or hashlib.sha256(data).hexdigest()!=index['raw_sha256']:raise ValueError('Decoded metadata hash/size differs')
        return data
    if actual.name.endswith('.gz'):
        if actual.stat().st_size>FILE_LIMIT:raise ValueError('Compressed metadata exceeds1MiB')
        return _gunzip(actual.read_bytes(),max_bytes)
    if actual.stat().st_size>max_bytes:raise ValueError('Plain metadata exceeds its decoded bound')
    return actual.read_bytes()

def load(path,max_bytes=DECODE_LIMIT):return json.loads(read_bytes(path,max_bytes))
def digest(path,max_bytes=DECODE_LIMIT):return hashlib.sha256(read_bytes(path,max_bytes)).hexdigest()

def files(root):
    return [p for p in Path(root).rglob('*')if p.is_file()and(p.name.endswith(('.json','.json.gz'))or re.search(r'\.json\.gz\.part\d{4}$',p.name))]

def size(root):return sum(p.stat().st_size for p in files(root))

def check_budget(root,reserve=RESERVE):
    paths=files(root);retained=sum(p.stat().st_size for p in paths)
    if any(p.stat().st_size>FILE_LIMIT for p in paths):raise MetadataBudgetError('Retained metadata file exceeds1MiB',retained)
    if retained+reserve>TOTAL_LIMIT:raise MetadataBudgetError('Accounting/component metadata exceeds8MiB retention budget',retained)
    return retained

def retain_bytes(data,logical,root,max_bytes=DECODE_LIMIT):
    logical=Path(logical).resolve();root=Path(root).resolve()
    if not logical.resolve().is_relative_to(root)or logical.suffix!='.json':raise ValueError('Metadata destination must be owned JSON under output')
    if len(data)>max_bytes:raise ValueError('Raw metadata exceeds its bound')
    encoded=gzip.compress(data,compresslevel=9,mtime=0);raw_sha=hashlib.sha256(data).hexdigest();encoded_sha=hashlib.sha256(encoded).hexdigest()
    stored=Path(str(logical)+'.gz');pending={}
    if len(encoded)<=FILE_LIMIT:
        pending[stored]=encoded
    else:
        chunks=[]
        for number,start in enumerate(range(0,len(encoded),FILE_LIMIT)):
            if number>=10_000:raise ValueError('Too many metadata chunks')
            part=Path(str(stored)+f'.part{number:04d}');payload=encoded[start:start+FILE_LIMIT];pending[part]=payload
            chunks.append(dict(name=part.name,bytes=len(payload),sha256=hashlib.sha256(payload).hexdigest()))
        index=dict(schema=INDEX_SCHEMA,encoding='gzip',logical_name=logical.name,raw_bytes=len(data),raw_sha256=raw_sha,stored_bytes=len(encoded),stored_sha256=encoded_sha,chunks=chunks)
        stored=Path(str(stored)+'.index.json');pending[stored]=(json.dumps(index,indent=2,sort_keys=True)+'\n').encode()
    old={p for p in(logical,Path(str(logical)+'.gz'),Path(str(logical)+'.gz.index.json'),*logical.parent.glob(logical.name+'.gz.part[0-9][0-9][0-9][0-9]'))if p.is_file()}
    existing=[p for p in files(root)if p not in old and p not in pending];retained=sum(p.stat().st_size for p in existing)+sum(map(len,pending.values()))
    if any(p.stat().st_size>FILE_LIMIT for p in existing)or any(len(b)>FILE_LIMIT for b in pending.values()):raise MetadataBudgetError('Retained metadata file exceeds1MiB',retained)
    if retained+RESERVE>TOTAL_LIMIT:raise MetadataBudgetError('Accounting/component metadata exceeds8MiB retention budget',retained)
    logical.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.metadata-',dir=logical.parent)as staging:
        for target,payload in pending.items():(Path(staging)/target.name).write_bytes(payload)
        for target in pending:(Path(staging)/target.name).replace(target)
        for obsolete in old-set(pending):obsolete.unlink()
    if read_bytes(stored,max_bytes)!=data:raise ValueError('Retained metadata did not round-trip')
    storage_files=[dict(path=str(target),bytes=len(payload),sha256=hashlib.sha256(payload).hexdigest())for target,payload in pending.items()]
    return dict(path=str(stored),logical_path=str(logical),encoding='gzip',raw_bytes=len(data),raw_sha256=raw_sha,
        stored_bytes=sum(map(len,pending.values())),stored_file_bytes=len(pending[stored]),stored_sha256=hashlib.sha256(pending[stored]).hexdigest(),
        compressed_stream_sha256=encoded_sha,storage_files=storage_files,retained_metadata_bytes=retained)

def retain_file(original,logical,root,max_bytes=DECODE_LIMIT):
    return retain_bytes(read_bytes(original,max_bytes),logical,root,max_bytes)
