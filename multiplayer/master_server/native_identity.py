"""Temporary clock/SQLite IO for the Rust canonical identity transaction."""
import time
from types import SimpleNamespace
from .native_party_directory import run


def database(db,mode,*,body=None,now=None):
    # Low-level allocation retains the caller-owned transaction contract.
    # Its caller can still roll back subsequent dependent identity operations.
    if mode=='allocate' and not db.in_transaction:db.execute('BEGIN')
    io=SimpleNamespace(directory=None,db=db,wall=time.time if now is None else lambda:now,clock=time.monotonic)
    if not db.in_transaction:return run(io,mode,family='identity',body=body)
    # A failed composed operation must preserve the caller's earlier writes.
    # SQLite resolves nested equal savepoint names to the innermost scope.
    db.execute('SAVEPOINT h2_canonical_identity_io')
    try:
        result=run(io,mode,family='identity',body=body)
    except BaseException:
        if db.in_transaction:
            db.execute('ROLLBACK TO h2_canonical_identity_io')
            db.execute('RELEASE h2_canonical_identity_io')
        raise
    db.execute('RELEASE h2_canonical_identity_io')
    return result
