"""Bounded, independent accounting for sources, principals and service work."""
from collections import OrderedDict
from collections.abc import MutableMapping
import ipaddress
import time

from .protocol import require


def source_prefix(source, ipv6_bits=64):
    try:
        address = ipaddress.ip_address(source)
    except ValueError:
        return 'unknown'
    if isinstance(address, ipaddress.IPv6Address) and address.ipv4_mapped:
        address = address.ipv4_mapped
    bits = ipv6_bits if address.version == 6 else 32
    return str(ipaddress.ip_network((address, bits), strict=False))


class TokenBuckets(MutableMapping):
    """O(1) LRU admission; exact-source churn never evicts principal budgets."""
    SOURCE_ROUTES = {'ip', 'owner', 'discovery', 'steam-start', 'steam-callback',
                     'steam-poll', 'steam-resume', 'steam-forget', 'steam-friends'}
    CAPACITY = {'source':4096, 'prefix':1024, 'principal':4096,
                'anonymous':1024, 'peer':8, 'global':32}

    def __init__(self):
        self.tables = {name:OrderedDict() for name in self.CAPACITY}
        self.evictions = 0
        self.rejections = 0

    def table(self, key):
        route = key.split(':', 1)[0]
        if route in ('global', 'prefix', 'peer'):
            scope = route
        elif key == 'discovery:global':
            scope = 'global'
        elif route == 'steam-device':
            scope = 'anonymous'
        else:
            scope = 'source' if route in self.SOURCE_ROUTES else 'principal'
        return self.tables[scope], self.CAPACITY[scope]

    def __getitem__(self, key):
        return self.table(key)[0][key]

    def __setitem__(self, key, value):
        table, capacity = self.table(key)
        if key not in table and len(table) >= capacity:
            table.popitem(last=False)
            self.evictions += 1
        table[key] = value
        table.move_to_end(key)

    def __delitem__(self, key):
        del self.table(key)[0][key]

    def __iter__(self):
        for table in self.tables.values():
            yield from table

    def __len__(self):
        return sum(map(len, self.tables.values()))

    def charge(self, key, refill, burst=None, interval=60, cost=1):
        now = time.monotonic()
        maximum = refill if burst is None else burst
        tokens, previous = self.get(key, (float(maximum), now))
        tokens = min(maximum, tokens + (now - previous) * refill / interval)
        if tokens < cost:
            self.rejections += 1
            require(False, 'Rate limit exceeded', 429)
        self[key] = tokens - cost, now

    def metrics(self):
        return {'occupancy':{name:len(table) for name, table in self.tables.items()},
                'evictions':self.evictions, 'rejections':self.rejections}


class RequestSlots:
    def __init__(self, limit=64):
        self.limit, self.active = limit, 0

    def acquire(self):
        require(self.active < self.limit, 'Service request capacity reached', 503)
        self.active += 1
        return RequestLease(self)


class RequestLease:
    def __init__(self, owner):
        self.owner = owner

    def release(self):
        if self.owner is not None:
            self.owner.active -= 1
            self.owner = None
