"""Bounded verifier admission and exact worker-stream retirement under contention."""
import asyncio
import contextlib
import json
import unittest
from unittest.mock import AsyncMock, patch
from multiplayer.master_server.protocol import Problem
from . import test_eos_identity as identity_tests
from . import test_matchmaking_http as baseline


class Worker:
    def __init__(self):
        self.returncode = None
        self.writes = []
        self.written = asyncio.Event()
        self.killed = 0
        self.waiting_cleanup = asyncio.Event()
        self.cleanup_allowed = asyncio.Event(); self.cleanup_allowed.set()
        self.responses = asyncio.Queue()
        self.stdin = self
        self.stdout = self

    def write(self, value):
        self.writes.append(json.loads(value))
        self.written.set()

    async def drain(self):
        return None

    async def readline(self):
        return await self.responses.get()

    def reply(self, account):
        self.responses.put_nowait(b'H2EOS ' + json.dumps({'ok': True, 'value': {'account': account}}).encode() + b'\n')

    def kill(self):
        self.killed += 1

    async def wait(self):
        self.waiting_cleanup.set()
        await self.cleanup_allowed.wait()
        self.returncode = -9
        return self.returncode


class EosVerificationQueueTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    configure = identity_tests.EosIdentityTests.configure

    def budget(self, service=.5):
        actual = asyncio.timeout
        observed = []
        def accelerated(seconds):
            observed.append(seconds)
            return actual(.02 if seconds == 2 else service if seconds == 20 else seconds)
        return patch('multiplayer.master_server.eos_identity.asyncio.timeout', accelerated), observed

    async def task(self, awaitable):
        result = asyncio.create_task(awaitable)
        async def stop():
            if not result.done(): result.cancel()
            with contextlib.suppress(asyncio.CancelledError, Problem): await result
        self.addAsyncCleanup(stop)
        return result

    async def test_queue_timeout_refuses_without_retiring_active_worker(self):
        identity = self.configure(); identity.binary = 'test-worker'
        worker = Worker(); identity.process = worker
        budgets, observed = self.budget()
        with budgets:
            active = await self.task(identity.verify('1' * 32, 'first-proof'))
            await asyncio.wait_for(worker.written.wait(), 1)
            with self.assertRaises(Problem) as failure:
                await identity.verify('2' * 32, 'queued-proof')
            self.assertEqual(failure.exception.status, 503)
            self.assertEqual(worker.killed, 0)
            self.assertIs(identity.process, worker)
            self.assertEqual(identity.pending, 1)
            self.assertFalse(active.done())
            worker.reply('first-account')
            self.assertEqual(await active, 'first-account')
        self.assertEqual(observed, [2, 20, 2])
        self.assertEqual(len(worker.writes), 1)
        self.assertEqual(identity.pending, 0)

    async def test_cancelled_queue_waiter_leaves_active_worker_and_reply_untouched(self):
        identity = self.configure(); identity.binary = 'test-worker'
        worker = Worker(); identity.process = worker
        budgets, _ = self.budget()
        with budgets:
            active = await self.task(identity.verify('1' * 32, 'first-proof'))
            await asyncio.wait_for(worker.written.wait(), 1)
            waiting = await self.task(identity.verify('2' * 32, 'queued-proof'))
            await asyncio.sleep(0)
            self.assertEqual(identity.pending, 2)
            waiting.cancel()
            with self.assertRaises(asyncio.CancelledError): await waiting
            self.assertEqual(worker.killed, 0); self.assertIs(identity.process, worker)
            self.assertEqual(identity.pending, 1)
            worker.reply('first-account')
            self.assertEqual(await active, 'first-account')
        self.assertEqual(identity.pending, 0)
        self.assertEqual(len(worker.writes), 1)

    async def test_owned_timeout_detaches_worker_so_stale_reply_cannot_reach_next_caller(self):
        identity = self.configure(); identity.binary = 'test-worker'
        old = Worker(); identity.process = old
        budgets, observed = self.budget(service=.02)
        with budgets:
            with self.assertRaises(Problem) as failure:
                await identity.verify('1' * 32, 'expired-proof')
            self.assertEqual(failure.exception.status, 503)
            self.assertEqual(old.killed, 1); self.assertIsNone(identity.process)
            self.assertFalse(identity.lock.locked()); self.assertEqual(identity.pending, 0)
            # A late old reply exists, but the next call must own a fresh stream.
            old.reply('wrong-stale-account')
            fresh = Worker(); fresh.reply('fresh-account')
            with patch('multiplayer.master_server.eos_identity.asyncio.create_subprocess_exec', AsyncMock(return_value=fresh)) as create:
                self.assertEqual(await identity.verify('2' * 32, 'fresh-proof'), 'fresh-account')
            create.assert_awaited_once()
        self.assertEqual(observed, [2, 20, 2, 20])
        self.assertEqual(len(old.writes), 1); self.assertEqual(len(fresh.writes), 1)
        self.assertIs(identity.process, fresh)

    async def test_cold_worker_creation_is_inside_owned_service_budget(self):
        identity = self.configure(); identity.binary = 'test-worker'
        created = asyncio.Event(); cancelled = asyncio.Event()
        async def cold_start(*_args, **_kwargs):
            created.set()
            try:
                await asyncio.Event().wait()
            finally:
                cancelled.set()
        budgets, observed = self.budget(service=.02)
        with budgets, patch('multiplayer.master_server.eos_identity.asyncio.create_subprocess_exec', cold_start):
            with self.assertRaises(Problem) as failure:
                await asyncio.wait_for(identity.verify('1' * 32, 'cold-proof'), 1)
            self.assertEqual(failure.exception.status, 503)
        self.assertTrue(created.is_set()); self.assertTrue(cancelled.is_set())
        self.assertEqual(observed, [2, 20])
        self.assertIsNone(identity.process); self.assertEqual(identity.pending, 0)
        self.assertFalse(identity.lock.locked())

    async def test_owner_cancellation_holds_lock_until_exact_worker_cleanup_finishes(self):
        identity = self.configure(); identity.binary = 'test-worker'
        old = Worker(); old.cleanup_allowed.clear(); identity.process = old
        fresh = Worker(); fresh.reply('fresh-account')
        budgets, _ = self.budget()
        try:
            with budgets, patch('multiplayer.master_server.eos_identity.asyncio.create_subprocess_exec', AsyncMock(return_value=fresh)) as create:
                active = await self.task(identity.verify('1' * 32, 'cancelled-proof'))
                await asyncio.wait_for(old.written.wait(), 1)
                active.cancel()
                await asyncio.wait_for(old.waiting_cleanup.wait(), 1)
                self.assertIsNone(identity.process); self.assertTrue(identity.lock.locked())
                queued = await self.task(identity.verify('2' * 32, 'next-proof'))
                await asyncio.sleep(0)
                create.assert_not_awaited()
                self.assertFalse(queued.done())
                old.cleanup_allowed.set()
                with self.assertRaises(asyncio.CancelledError): await active
                self.assertEqual(await queued, 'fresh-account')
                create.assert_awaited_once()
            self.assertEqual(old.killed, 1)
            self.assertEqual(len(old.writes), 1); self.assertEqual(len(fresh.writes), 1)
            self.assertEqual(identity.pending, 0)
        finally:
            old.cleanup_allowed.set()
