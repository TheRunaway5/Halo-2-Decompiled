"""Player-hosted native authority generations and complete-world acknowledgements.

This coordinates an existing complete simulation; it never promotes a display
snapshot or recreates players. The native continuation owner must be installed
before a room enables this protocol. Election ordering is an explicit native
relay policy, not a recovered interpretation of Xbox host-quality fields.
"""
from dataclasses import dataclass
import math
import time

from .protocol import require


@dataclass(frozen=True)
class WorldSeal:
    tick: int
    digest: str


class PlayerHostedContinuity:
    def __init__(self, owners, host, *, clock=time.monotonic, timeout=15):
        self.owners = tuple(owners)
        self.active = set(self.owners)
        require(2 <= len(self.owners) <= 16 and all(
            isinstance(p, int) and 0 <= p < len(self.owners) and self.owners[p] == p
            and self.owners.count(p) <= 4 for p in self.active), "Invalid physical match ownership")
        require(host in self.active, "Invalid match authority")
        self.host, self.epoch, self.phase = host, 0, "running"
        self.clock, self.timeout = clock, timeout
        self.deadline = None
        self.seals, self.proofs = {}, {}
        self.selected = None
        self.rebound, self.failed_candidates = set(), set()
        self.quality = {}

    @property
    def members(self):
        return sum(1 << i for i, p in enumerate(self.owners) if p in self.active)

    def observe_quality(self, peer, milliseconds):
        require(peer in self.active and isinstance(milliseconds, (int, float))
                and math.isfinite(milliseconds) and 0 <= milliseconds <= 60000,
                "Invalid observed match quality")
        self.quality[peer] = milliseconds

    def seal(self, peer, epoch, tick, digest):
        require(self.phase == "running" and peer == self.host and peer in self.active
                and epoch == self.epoch, "Stale or unauthorized world seal")
        require(type(tick) is int and 0 <= tick < 2**64 and isinstance(digest, str)
                and len(digest) == 64 and all(c in "0123456789abcdef" for c in digest),
                "Invalid complete-world seal")
        if self.seals:
            require(tick >= max(self.seals), "World seal moved backwards")
        seal = WorldSeal(tick, digest)
        require(tick not in self.seals or self.seals[tick] == seal,
                "Authority changed a sealed completed tick")
        self.seals[tick] = seal
        # Bound coordination metadata. The actual world remains on peers.
        for old in sorted(self.seals)[:-64]:
            del self.seals[old]
        self.proofs[self.host] = seal
        return seal

    def acknowledge(self, peer, epoch, tick, digest):
        awaiting = self.phase == "awaiting_world"
        require(peer in self.active and epoch == self.epoch - int(awaiting)
                and self.phase in ("running", "awaiting_world"), "Stale world acknowledgement")
        seal = self.seals.get(tick)
        require(seal is not None and seal.digest == digest,
                "Standby simulation differs from the sealed world")
        previous = self.proofs.get(peer)
        require(previous is None or tick >= previous.tick,
                "Standby simulation moved backwards")
        self.proofs[peer] = seal
        if awaiting:
            self._choose_candidate()

    def disconnect(self, peer):
        if peer not in self.active:
            return False
        self.active.remove(peer)
        self.proofs.pop(peer, None)
        self.rebound.discard(peer)
        if not self.active:
            self.phase = "empty"
            self.deadline = None
        elif peer == self.host:
            self._elect()
        elif self.phase == "rebinding":
            self._finish_rebinding()
        return True

    def _elect(self):
        self.epoch += 1
        self.phase = "migrating"
        self.rebound.clear()
        self._choose_candidate()

    def _choose_candidate(self):
        # Only a physical connection which has acknowledged an actual complete
        # world can become host. Guest and secondary couch slots never qualify.
        candidates = [p for p in self.active if p not in self.failed_candidates
                      and p in self.proofs and self.proofs[p].tick in self.seals
                      and self.seals[self.proofs[p].tick] == self.proofs[p]]
        if self.selected is not None:
            candidates = [p for p in candidates if self.proofs[p] == self.selected]
        elif self.seals:
            # A peer may already have simulated a newer relayed tick before
            # its acknowledgement arrives. Never select an older private bank
            # or silently roll the match back to an easier candidate.
            newest = max(self.seals)
            candidates = [p for p in candidates if self.proofs[p].tick == newest]
        if not candidates:
            if self.selected is None and self.seals:
                self.phase = "awaiting_world"
                if self.deadline is None:
                    self.deadline = self.clock() + self.timeout
            else:
                self.phase, self.deadline = "unavailable", None
            return
        self.phase = "migrating"
        self.host = min(candidates, key=lambda p: (self.quality.get(p, math.inf), p))
        self.selected = self.proofs[self.host]
        self.deadline = self.clock() + self.timeout

    def accept(self, peer, epoch, tick, digest):
        require(self.phase == "migrating" and peer == self.host and peer in self.active
                and epoch == self.epoch and self.selected == WorldSeal(tick, digest),
                "Authority handoff does not match the retained complete world")
        self.phase = "rebinding"
        self.rebound = {peer}
        self.deadline = self.clock() + self.timeout
        self._finish_rebinding()

    def rebind(self, peer, epoch, tick, digest):
        require(self.phase == "rebinding" and peer in self.active and epoch == self.epoch
                and self.selected == WorldSeal(tick, digest),
                "Peer rebind does not match the retained complete world")
        self.rebound.add(peer)
        self._finish_rebinding()

    def _finish_rebinding(self):
        if self.active <= self.rebound:
            self.phase, self.deadline = "running", None
            # Subsequent elections start from the next acknowledged world;
            # current acknowledgements retain the handoff state until then.
            self.proofs = {p: self.selected for p in self.active}
            self.seals = {self.selected.tick: self.selected}
            self.selected = None
            self.failed_candidates.clear()

    def expire(self):
        if self.deadline is None or self.clock() < self.deadline:
            return False
        if self.phase == "awaiting_world":
            self.phase, self.deadline = "unavailable", None
        elif self.phase == "migrating":
            self.failed_candidates.add(self.host)
            self._elect()
        elif self.phase == "rebinding":
            # A peer which cannot rebind is a departure, never an invented
            # successful world import. Already rebound survivors can continue.
            for peer in self.active - self.rebound:
                self.proofs.pop(peer, None)
            self.active.intersection_update(self.rebound)
            self._finish_rebinding()
        return True

    def control(self):
        value = dict(epoch=self.epoch, host=None if self.phase in ("awaiting_world", "empty", "unavailable") else self.host, members=self.members,
                     phase=self.phase)
        if self.selected is not None:
            value.update(tick=self.selected.tick, digest=self.selected.digest)
        return value
