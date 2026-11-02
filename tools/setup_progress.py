"""Measured setup work counters, with a bounded line protocol for the GUI child."""
import json
import re
import threading
import time

PREFIX = 'H2_SETUP_PROGRESS\t'
SCHEMA = 'H2_SETUP_PROGRESS_V1'


class ProgressUpdate(str):
    """Still a readable string for terminal and existing progress callbacks."""

    def __new__(cls, stage, label, completed, total, unit='items'):
        if (not isinstance(stage, str) or not re.fullmatch('[a-z][a-z0-9-]{0,63}', stage) or
                not isinstance(label, str) or not 1 <= len(label) <= 120 or
                any(c in label for c in '\r\n\t') or
                type(completed) is not int or type(total) is not int or
                not 0 <= completed <= total or not 0 < total < 2**63 or
                unit not in ('items', 'bytes')):
            raise ValueError('Invalid setup work counter')
        percent = completed * 100 // total
        result = super().__new__(cls, f'{label} — {percent}%')
        result.percent = percent
        result.record = dict(schema=SCHEMA, stage=stage, label=label,
                             completed=completed, total=total, unit=unit)
        return result


def parse_update(line):
    if isinstance(line, ProgressUpdate):
        return line
    if not isinstance(line, str) or not line.startswith(PREFIX) or len(line) > 2048:
        return None
    try:
        record = json.loads(line[len(PREFIX):])
        if (not isinstance(record, dict) or
                set(record) != {'schema', 'stage', 'label', 'completed', 'total', 'unit'} or
                record.pop('schema') != SCHEMA):
            return None
        return ProgressUpdate(**record)
    except (ValueError, TypeError):
        return None


def write_progress(message):
    if isinstance(message, ProgressUpdate):
        print(PREFIX + json.dumps(message.record, ensure_ascii=True), flush=True)
    else:
        print(message, flush=True)


class WorkProgress:
    """Aggregate parallel work; only finish() may publish a completed phase."""

    def __init__(self, stage, label, total, progress, unit='items'):
        self.stage, self.label, self.total = stage, label, max(1, total)
        self.progress, self.unit = progress, unit
        self.completed = 0
        self.lock = threading.Lock()
        self.last_percent, self.last_time = -1, 0
        self._emit(0)

    def _emit(self, completed):
        update = ProgressUpdate(self.stage, self.label, completed, self.total, self.unit)
        now = time.monotonic()
        if update.percent != self.last_percent or now - self.last_time >= .25:
            self.progress(update)
            self.last_percent, self.last_time = update.percent, now

    def advance(self, amount=1):
        with self.lock:
            self.completed += amount
            self._emit(min(self.completed, self.total - 1))

    def finish(self):
        with self.lock:
            self._emit(self.total)
