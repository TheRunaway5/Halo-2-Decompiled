"""Temporary typed IO for Rust-owned result admission and finalization.

Native unanimity is a consistency policy, not recovered Live arbitration or
independent simulation truth. Rust owns the immutable admission, report guards,
consent/quarantine state, XP and complete transactional storage plan.
"""
import json
import logging

from .native_database import run as database
from .native_matchmaking import native, value
from .protocol import require

ORIGINAL_PRESETS = {name: tuple(row) for name, row in value('result_presets').items()}


class MatchResults:
    def __init__(self, store, skills, clan_skills):
        self.store, self.skills, self.clan_skills = store, skills, clan_skills
        database(store.db, 'result_schema')
        from .carnage_history import ReportHistory
        self.history = ReportHistory(store)

    @staticmethod
    def _call(room, op, **values):
        return room.api.core._call(op, id=room.id, **values)

    def open(self, room):
        return self._call(room, 'result_open', context=dict(
            proposal=room.id, skill_configuration=room.skill_configuration,
            clan_configuration=room.clan_configuration, skills=room.skills,
            clans={str(team): clan for team, clan in room.clans.items()},
            clan_skills=room.clan_skills, created_wall=room.created_wall))

    def snapshot(self, room):
        if room._result_final is not None:
            return room._result_final[0]
        return self._call(room, 'result_snapshot')

    def report(self, room):
        if room._result_final is not None:
            return room._result_final[1]
        return bytes.fromhex(self._call(room, 'result_report'))

    def validate(self, room, report):
        digest = self._call(room, 'result_validate', report=report)
        return bytes.fromhex(report), digest

    def submit(self, room, actor, report):
        return room.room_call('room_state_submit', player=actor, report=report)['digest']

    def confirm(self, room, actor, digest):
        room.room_call('room_state_confirm', player=actor, digest=digest)
        return self.snapshot(room)

    def revoke(self, room, actor):
        self._call(room, 'result_revoke', actor=actor)

    def review(self, room):
        return self._call(room, 'result_review')

    def retire(self, room):
        if room._result_final is None:
            room._result_final = (self.snapshot(room), self.report(room))
            self._call(room, 'result_drop')

    def commit(self, room):
        result = database(self.store.db, 'result_commit', source=room.api.core._handle,
                          match=room.id, completed=self.store.now())
        self._call(room, 'result_committed', digest=result['digest'])
        if not result['already_committed']:
            logging.getLogger('halo2.results').info(json.dumps(dict(
                event='carnage_report_committed', match=result['public'],
                format='native-verified-summary-v2', published_players=result['published_players'])))

    def _fixture_accuracy(self, room):
        require(native().h2_matchmaking_test_inputs_enabled(),
                'Result fixture inputs are disabled', 503)
        self._call(room, 'result_fixture_accuracy')
        room.supports_accuracy = True

    def _validate_fixture(self, room, report):
        require(native().h2_matchmaking_test_inputs_enabled(),
                'Result fixture inputs are disabled', 503)
        digest = value('result_validate_fixture', context=dict(proposal=room.proposal,
            players=[dict(id=p, name=name, team=team) for p, name, team in room.players],
            team_mode=room.team_mode, identities=dict(room.identities)), report=report)
        return bytes.fromhex(report), digest
