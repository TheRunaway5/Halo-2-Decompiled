"""Fail-fast admission held through response drain, without a waiter queue."""
import asyncio
from contextlib import contextmanager

from aiohttp import web

from .protocol import MAX_SYNC, require


class ResponseBudget:
    def __init__(self, requests=4, byte_limit=2 * MAX_SYNC):
        self.request_limit = requests
        self.byte_limit = byte_limit
        self.active = 0
        self.reserved_bytes = 0
        self.peers = set()

    @contextmanager
    def reserve(self, size):
        # All access is on the owning event loop, with no await during admission.
        require(self.active < self.request_limit and self.reserved_bytes + size <= self.byte_limit,
                "Directory response capacity reached", 503)
        self.active += 1
        self.reserved_bytes += size
        try:
            yield
        finally:
            self.active -= 1
            self.reserved_bytes -= size

    @contextmanager
    def peer(self, sender):
        require(sender not in self.peers, "Peer synchronization already in flight", 429)
        self.peers.add(sender)
        try:
            yield
        finally:
            self.peers.remove(sender)

    async def send(self, request, raw):
        # Returning an unprepared Response would release admission before aiohttp
        # writes it. Drain it here so slow readers retain their reservation.
        response = web.Response(body=raw, content_type="application/json",
                                headers={"Cache-Control": "no-store"})
        try:
            async with asyncio.timeout(5):
                await response.prepare(request)
                await response.write_eof()
        except BaseException:
            # A response may already have sent headers. Abort, never append a
            # JSON error to a partially written response on the same connection.
            if request.transport is not None:
                request.transport.abort()
            raise
        return response
