"""Finite owned-session budgets; these controls never enter the game runtime."""
from dataclasses import dataclass
import json
import math
import os
from pathlib import Path
import signal
import threading
import time


class QualificationBudgetExhausted(RuntimeError):
    pass


@dataclass(frozen=True)
class CaseBudget:
    native_seconds: float
    wrapper_seconds: float
    replay_seconds: float
    transport_seconds: float
    retention_seconds: float
    usable_seconds_at_start: float


class QualificationBudget:
    def __init__(self, total_seconds=14400, cleanup_seconds=60, maximum_seconds=21600,
                 *, clock=time.monotonic, wall_clock=time.time):
        if not all(math.isfinite(v) for v in (cleanup_seconds, total_seconds, maximum_seconds)) or not 0 < cleanup_seconds < total_seconds <= maximum_seconds:
            raise ValueError('Invalid finite owned-session budget')
        self.clock, self.wall_clock = clock, wall_clock
        self.started = clock()
        self.started_wall = wall_clock()
        self.total_seconds = float(total_seconds)
        self.cleanup_seconds = float(cleanup_seconds)
        self.maximum_seconds = float(maximum_seconds)
        self.revision = 0
        self.extensions = []
        self._lock = threading.Lock()

    def remaining(self, *, cleanup=True):
        with self._lock:
            limit = self.started + self.total_seconds
            margin = self.cleanup_seconds if cleanup else 0
        return max(0., limit - self.clock() - margin)

    def timeout(self, requested=None, *, reserve_after=0, minimum=.01):
        remaining = self.remaining() - reserve_after
        if requested is not None:
            remaining = min(remaining, float(requested))
        if not math.isfinite(remaining) or remaining < minimum:
            raise QualificationBudgetExhausted('Insufficient owned time after cleanup/reserve margins')
        return remaining

    def plan_case(self, native_seconds=180, replay_seconds=180, *, transport_seconds=90,
                  retention_seconds=90):
        values = (native_seconds, replay_seconds, transport_seconds, retention_seconds)
        if any(not math.isfinite(v) or v <= 0 for v in values):
            raise ValueError('Every case phase needs a positive finite budget')
        usable = self.remaining()
        native = self.timeout(native_seconds,
                              reserve_after=replay_seconds + transport_seconds + retention_seconds)
        # Do not silently shorten a declared genuine-input test to make it fit.
        if native < native_seconds:
            raise QualificationBudgetExhausted('Whole declared case/replay cannot fit before cleanup')
        return CaseBudget(native, native + transport_seconds, replay_seconds,
                          transport_seconds, retention_seconds, usable)

    def extend(self, *, revision, total_seconds, reason, authorization):
        if not reason or not authorization:
            raise ValueError('Extension needs its explicit reason and authorization')
        with self._lock:
            if revision <= self.revision:
                return False
            if self.clock() >= self.started + self.total_seconds - self.cleanup_seconds:
                raise QualificationBudgetExhausted('An expired owner cannot be revived')
            if not math.isfinite(total_seconds) or not self.total_seconds < total_seconds <= self.maximum_seconds:
                raise ValueError('Extension must increase the finite total within its declared maximum')
            self.extensions.append(dict(revision=revision, previous_seconds=self.total_seconds,
                                        total_seconds=total_seconds, reason=reason,
                                        authorization=authorization, accepted_wall_time=self.wall_clock()))
            self.total_seconds = float(total_seconds)
            self.revision = revision
            return True

    def receipt(self):
        with self._lock:
            return dict(total_limit_seconds=self.total_seconds,
                        maximum_total_limit_seconds=self.maximum_seconds,
                        cleanup_margin_seconds=self.cleanup_seconds,
                        deadline_epoch=self.started_wall + self.total_seconds,
                        usable_deadline_epoch=self.started_wall + self.total_seconds - self.cleanup_seconds,
                        extension_revision=self.revision, extensions=list(self.extensions))


class QualificationWatchdog:
    """Stop only this run_test-owned process group before its cleanup reserve.

    A revisioned budget-control.json may extend the owner up to its declared
    finite maximum. Existing native/replay command caps stay at their declared
    values; a new command obtains a fresh remaining-time cap.
    """
    def __init__(self, budget, report_directory, *, interval=.2, expire=None):
        self.budget = budget
        self.directory = Path(report_directory)
        self.interval = interval
        self.stopped = threading.Event()
        self.expired = threading.Event()
        self.last_control = None
        self.expire = expire or self._stop_owned_group
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.previous_term = None
        self.expiry_reason = "Absolute usable deadline reached; cleanup margin retained"

    def _stop_owned_group(self):
        if os.name != 'posix' or os.getpgrp() != os.getpid():
            raise RuntimeError('Absolute watchdog requires its own run_test process group')
        os.killpg(os.getpgrp(), signal.SIGTERM)

    def _handle_term(self, number, _frame):
        if self.expired.is_set():
            raise QualificationBudgetExhausted(self.expiry_reason)
        raise SystemExit(128 + number)

    def start(self):
        if self.expire == self._stop_owned_group:
            if os.name != 'posix' or os.getpgrp() != os.getpid():
                raise RuntimeError('Use tools/run_test.py to own this coordinator')
            self.previous_term = signal.signal(signal.SIGTERM, self._handle_term)
        self.thread.start()
        return self

    def _read_control(self):
        control = self.directory / 'budget-control.json'
        if not control.exists():
            return
        signature = control.stat().st_mtime_ns
        if signature == self.last_control:
            return
        try:
            data = json.loads(control.read_text())
            accepted = self.budget.extend(revision=int(data['revision']),
                total_seconds=float(data['total_limit_seconds']), reason=data['reason'],
                authorization=data['authorization'])
        except (ValueError, TypeError, KeyError, QualificationBudgetExhausted) as error:
            # A partially written or invalid control never extends the budget.
            (self.directory / 'budget-control-rejection.json').write_text(
                json.dumps(dict(accepted=False, error=str(error)), indent=2) + '\n')
            return
        self.last_control = signature
        if accepted:
            (self.directory / f"budget-extension-{self.budget.revision:02d}.json").write_text(
                json.dumps(self.budget.receipt(), indent=2) + '\n')

    def _run(self):
        while not self.stopped.wait(self.interval):
            try:
                self._read_control()
            except OSError:
                # Filesystem errors cannot disable the absolute time check.
                pass
            if self.budget.remaining() <= 0:
                self.expired.set()
                try:
                    (self.directory / 'absolute-budget-expiry.json').write_text(
                        json.dumps(self.budget.receipt(), indent=2) + '\n')
                finally:
                    self.expire()
                return

    def abort_owned(self, reason):
        self.expiry_reason = reason
        self.expired.set()
        try:
            (self.directory / 'owned-command-budget-expiry.json').write_text(
                json.dumps(dict(reason=reason, budget=self.budget.receipt()), indent=2) + '\n')
        finally:
            self._stop_owned_group()

    def close(self):
        self.stopped.set()
        self.thread.join(timeout=max(1., self.interval * 2))
        if self.previous_term is not None:
            signal.signal(signal.SIGTERM, self.previous_term)
