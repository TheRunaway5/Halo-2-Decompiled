"""Temporary generic SQLite IO for Rust-owned matchmaking transactions.

Only Rust emits statements, parameters, branch decisions and continuation IDs.
The adapter preserves one connection/transaction for a complete operation and
rolls it back on SQL, protocol, cancellation or native-owner errors.
"""
from contextlib import nullcontext
import weakref

from .native_matchmaking import native, _response
from .protocol import require


def _parameter(value):
    if type(value) is dict:
        require(set(value) == {'blob'}, 'Invalid Rust database parameter', 503)
        return bytes.fromhex(value['blob'])
    return value


def apply(db, plan):
    """Execute a Rust plan inside the caller's existing transaction boundary."""
    for statement in plan['statements']:
        cursor = db.execute(statement['sql'], tuple(_parameter(v) for v in statement['params']))
        expected = statement['changes']
        require(expected is None or cursor.rowcount == expected,
                'Accepted party changed during the transaction', 409)


class _Database:
    def __init__(self, request):
        lib = native()
        self.handle = _response(lib.h2_matchmaking_object_open, request={'database':request})
        self.finalizer = weakref.finalize(self, lib.h2_matchmaking_owner_drop, self.handle)

    def call(self, op, **values):
        require(self.handle, 'Rust database owner retired', 503)
        return _response(native().h2_matchmaking_owner_request, self.handle, request=dict(op=op, **values))

    def close(self):
        self.finalizer()
        self.handle = 0


def run(db, mode, **values):
    owner = _Database(dict(mode=mode, **values))
    try:
        event = owner.call('begin')
        with db if event['atomic'] else nullcontext():
            if event['atomic'] and not db.in_transaction:
                db.execute('BEGIN')
            while event['kind'] == 'sql':
                records, changes = [], []
                for statement in event['statements']:
                    cursor = db.execute(statement['sql'], tuple(_parameter(v) for v in statement['params']))
                    changes.append(cursor.rowcount)
                    if event['mode'] == 'query':
                        records = [list(row) for row in cursor.fetchmany(129)]
                    else:
                        require(event['mode'] == 'batch', 'Invalid Rust database operation', 503)
                event = owner.call('reply', frame=event['frame'], rows=records, changes=changes)
            require(event['kind'] == 'done', 'Invalid Rust database response', 503)
            result = event['value']
        return result
    finally:
        owner.close()
