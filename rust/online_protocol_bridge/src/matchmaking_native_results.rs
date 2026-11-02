//! Typed native result observations only; policy is in the safe core.
use crate::ffi;
use halo2_online_protocol::matchmaking_native_results as core;
fn error() -> core::Error {
    core::Error::Io("Invalid native result ABI".into())
}
fn io<T>(value: Result<T, cxx::Exception>) -> core::Result<T> {
    value.map_err(|e| core::Error::Io(e.to_string()))
}
fn protect(f: impl FnOnce() -> core::Result<()>) -> Result<(), String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(f))
        .map_err(|_| "Invalid native result ABI".to_owned())?
        .map_err(|e| e.to_string())
}
struct Io<'a>(&'a ffi::NativeResultIo);
impl core::Io for Io<'_> {
    fn state(&self) -> core::Result<core::State> {
        let s = io(self.0.result_state())?;
        let players: Vec<_> = s
            .players
            .into_iter()
            .map(|p| core::Player {
                connected: p.connected,
                membership: p.membership,
                total_score: p.total_score,
                kills: p.kills,
                deaths: p.deaths,
                assists: p.assists,
                suicides: p.suicides,
            })
            .collect();
        let accuracy = if s.accuracy_present {
            if s.accuracy.len() != 16 * 34 * 2 {
                return Err(error());
            }
            Some(std::array::from_fn(|p| {
                std::array::from_fn(|w| {
                    [
                        s.accuracy[(p * 34 + w) * 2],
                        s.accuracy[(p * 34 + w) * 2 + 1],
                    ]
                })
            }))
        } else {
            if !s.accuracy.is_empty() {
                return Err(error());
            }
            None
        };
        Ok(core::State {
            finished: s.finished,
            results_finalized: s.results_finalized,
            game_over_countdown: s.game_over_countdown,
            team_mode: s.team_mode,
            retained_teams: s.retained_teams,
            players: players.try_into().map_err(|_| error())?,
            team_scores: s.team_scores.try_into().map_err(|_| error())?,
            accuracy,
        })
    }
    fn roster(&self) -> core::Result<Vec<core::Admission>> {
        io(self.0.result_roster())?
            .into_iter()
            .map(|a| {
                Ok(core::Admission {
                    slot: a.slot,
                    team: a.team,
                    identity: a.identity.try_into().map_err(|_| error())?,
                })
            })
            .collect()
    }
    fn round_flags(&self) -> core::Result<u32> {
        io(self.0.result_round_flags())
    }
    fn variant(&self) -> core::Result<[u8; 304]> {
        io(self.0.result_variant())?.try_into().map_err(|_| error())
    }
    fn recording(&self) -> core::Result<core::Recording> {
        let r = io(self.0.result_recording())?;
        Ok(core::Recording {
            statistics_present: r.statistics_present,
            enabled: r.enabled,
            frozen: r.frozen,
        })
    }
    fn ranks(&self, extended: bool, count: u32) -> core::Result<core::Ranks> {
        let r = io(self.0.result_ranks(extended, count))?;
        Ok(core::Ranks {
            players: r.players.try_into().map_err(|_| error())?,
            teams: r.teams.try_into().map_err(|_| error())?,
        })
    }
    fn changing_teams(&self) -> core::Result<bool> {
        io(self.0.result_changing_teams())
    }
    fn live_player(&self, slot: u32) -> core::Result<core::LivePlayer> {
        let p = io(self.0.result_live_player(slot))?;
        Ok(core::LivePlayer {
            connected: p.connected,
            datum: p.datum,
            profile_present: p.profile_present,
            membership: p.membership,
        })
    }
    fn configuration(&self, slot: u32) -> core::Result<[u8; 132]> {
        io(self.0.result_configuration(slot))?
            .try_into()
            .map_err(|_| error())
    }
    fn name(&self, index: u32) -> core::Result<[u8; 64]> {
        io(self.0.result_name(index))?
            .try_into()
            .map_err(|_| error())
    }
    fn team_score(&self, team: u32) -> core::Result<i32> {
        io(self.0.result_team_score(team))
    }
    fn accumulated_score(&self, datum: u32) -> core::Result<i16> {
        io(self.0.result_accumulated_score(datum))
    }
    fn write(&self, p: core::Publication) -> core::Result<()> {
        io(self.0.result_write(&ffi::NativeResultPublication {
            variant: p.variant.to_vec(),
            players: p
                .players
                .into_iter()
                .map(|p| ffi::NativeResultPublicationPlayer {
                    occupied: p.occupied,
                    identity: p.identity.to_vec(),
                    configuration: p.configuration.to_vec(),
                    score: p.score,
                    placement: p.placement,
                })
                .collect(),
            teams: p
                .teams
                .into_iter()
                .map(|t| ffi::NativeResultPublicationTeam {
                    occupied: t.occupied,
                    score: t.score,
                    placement: t.placement,
                })
                .collect(),
            current: p.current,
            finalized: p.finalized,
            engine_present: p.engine_present,
        }))
    }
}
pub fn native_result_publish(v: &ffi::NativeResultIo) -> Result<(), String> {
    protect(|| core::publish(&Io(v)))
}
pub fn native_result_validate(v: &ffi::NativeResultIo, raw: &[u8]) -> Result<(), String> {
    protect(|| core::validate(&Io(v), raw))
}
