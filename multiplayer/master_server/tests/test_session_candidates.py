"""Differential outputs come from unchanged executable bytes, not this port."""
from dataclasses import replace
import gzip
import hashlib
import json
from pathlib import Path
import struct
import unittest

from multiplayer.master_server.session_candidates import (
    BANK_SIZE, COUNT, STRIDE, Candidates, Configuration, Search, threshold,
)


FIXTURE = Path(__file__).parent / "fixtures/retail-session-candidates.json.gz"


def bank(rows):
    value = bytearray(BANK_SIZE)
    for slot, data in rows:
        row = bytes.fromhex(data)
        assert 0 <= slot < COUNT and len(row) == STRIDE
        value[slot*STRIDE:(slot+1)*STRIDE] = row
    return bytes(value)


def search(values):
    values = values.copy()
    for key in ("clan", "own_address"):
        if values[key] is not None:
            values[key] = bytes.fromhex(values[key])
    return Search(**values)


def config(values):
    values = values.copy()
    values["latency_curve"] = tuple(values["latency_curve"])
    values["language"] = tuple(tuple(row) for row in values["language"])
    return Configuration(**values)


class SessionCandidateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = json.loads(gzip.decompress(FIXTURE.read_bytes()))

    def test_original_provenance(self):
        self.assertEqual(self.original["xbe_sha256"],
                         "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d")
        runner = Path(__file__).resolve().parents[3] / "tools/oracle_session_candidates.py"
        # The original-byte runner does not import the implementation tested here.
        self.assertEqual(self.original["oracle_sha256"],
                         "29133faa0b9a6a08868a3477f1f3a16bebc8e4c0660f8f2d11e9f4cf0a05e59e")
        # Production containers contain the fixture, not copyrighted reference
        # executables or the optional disassembler/emulator tooling.
        if runner.is_file():
            self.assertEqual(self.original["oracle_sha256"], hashlib.sha256(runner.read_bytes()).hexdigest())
        self.assertGreaterEqual(len(self.original["banks"]), 250)
        self.assertGreaterEqual(len(self.original["selection"]), 120)
        self.assertGreaterEqual(len(self.original["admission"]), 25)

    def test_every_original_full_bank_and_byte_cache(self):
        for i, fixture in enumerate(self.original["banks"]):
            with self.subTest(bank=i):
                candidates = Candidates(search(fixture["search"]), config(fixture["config"]))
                candidates.bank = bank(fixture["previous"])
                self.assertEqual(candidates.update(bank(fixture["bank"])), bank(fixture["expected"]))
                self.assertEqual(candidates.update(candidates.bank), bank(fixture["expected"]))

    def test_every_original_admission_gate(self):
        for i, fixture in enumerate(self.original["admission"]):
            with self.subTest(admission=i):
                candidates = Candidates(search(fixture["search"]))
                original_row = bytes.fromhex(fixture["record"])
                result = candidates.update(original_row + bytes(BANK_SIZE - STRIDE))
                rows = [result[i*STRIDE:(i+1)*STRIDE] for i in range(COUNT)]
                admitted_row = next(row for row in rows if row[0])
                self.assertEqual(bool(admitted_row[1]), fixture["expected"])

    def test_every_original_ready_threshold_and_selected_candidate(self):
        for i, fixture in enumerate(self.original["selection"]):
            with self.subTest(selection=i):
                candidates = Candidates(Search(1, 10, 0, 0, -1), config(fixture["config"]))
                candidates.bank = bank(fixture["bank"])
                selected = candidates.select(fixture["minimum_score"], fixture["elapsed_query_ms"],
                                             active=fixture["active"])
                self.assertEqual(None if selected is None else struct.unpack_from("<i", selected, 0xBC)[0],
                                 fixture["expected_id"])
                self.assertIsNone(candidates.select(fixture["minimum_score"], 6000, active=False))

    def test_every_original_radius_threshold(self):
        for fixture in self.original["thresholds"]:
            with self.subTest(fixture=fixture):
                self.assertEqual(threshold(fixture["radius"], fixture["maximum_radius"]), fixture["expected"])

    def test_internal_inputs_are_bounded_and_query_owner_is_explicit(self):
        value = Search(1, 10, 0, 0, -1)
        for changes in ({"player_count": 0}, {"player_count": 17}, {"player_count": True},
                        {"language": 9}, {"average_rank": -1}, {"quality_second": 1 << 31},
                        {"own_address": b"short"}, {"clan": b"short"}):
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                replace(value, **changes)
        defaults = Configuration()
        for changes in ({"rank_radius": -1}, {"age_coefficient": float("nan")},
                        {"latency_curve": (0,)}, {"language": ((0,),)}, {"partial_qos_delay_ms": -1}):
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                replace(defaults, **changes)
        candidates = Candidates(value)
        for invalid in (b"", bytes(BANK_SIZE - 1), bytearray(BANK_SIZE)):
            with self.assertRaises(ValueError):
                candidates.update(invalid)
        self.assertIsNone(candidates.select(100, 6000))
        for args in ((True, 100), (100, 1 << 31)):
            with self.assertRaises(ValueError):
                candidates.select(*args)
        with self.assertRaises(ValueError):
            candidates.select(100, 6000, active=1)
