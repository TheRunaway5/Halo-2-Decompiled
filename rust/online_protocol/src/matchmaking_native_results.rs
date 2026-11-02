//! Native ONLINE result publication/admission caller policy.
//!
//! Native game statistics, original ranking/profile conversion and the numeric
//! PCR writer remain actual IO owners. This is the modern admission adapter,
//! not recovered Xbox Live arbitration or complete retained metadata.
use crate::security_policy::PCR_SIZE;

pub const PLAYERS: usize = 16;
pub const TEAMS: usize = 8;
pub const WEAPONS: usize = 34;
pub const FIRST_WEAPON: usize = 5;
pub const VARIANT_SIZE: usize = 304;
pub const CONFIGURATION_SIZE: usize = 132;
const DIFFERENT: &str = "ONLINE match results differ from the final admitted game state";

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Error {
    State(&'static str),
    Io(String),
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::State(s) => f.write_str(s),
            Self::Io(s) => f.write_str(s),
        }
    }
}
pub type Result<T> = std::result::Result<T, Error>;
fn need(value: bool, message: &'static str) -> Result<()> {
    if value {
        Ok(())
    } else {
        Err(Error::State(message))
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Player {
    pub connected: bool,
    pub membership: i32,
    pub total_score: i32,
    pub kills: i32,
    pub deaths: i32,
    pub assists: i32,
    pub suicides: i32,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct State {
    pub finished: bool,
    pub results_finalized: bool,
    pub game_over_countdown: i32,
    pub team_mode: bool,
    pub retained_teams: u8,
    pub players: [Player; PLAYERS],
    pub team_scores: [i32; TEAMS],
    pub accuracy: Option<[[[u16; 2]; WEAPONS]; PLAYERS]>,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Admission {
    pub slot: u32,
    pub team: i32,
    pub identity: [u8; 8],
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct LivePlayer {
    pub connected: bool,
    pub datum: u32,
    pub profile_present: bool,
    pub membership: i32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Recording {
    pub statistics_present: bool,
    pub enabled: u8,
    pub frozen: u8,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Ranks {
    pub players: [u8; PLAYERS],
    pub teams: [u8; TEAMS],
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PublicationPlayer {
    pub occupied: u8,
    pub identity: [u8; 8],
    pub configuration: [u8; CONFIGURATION_SIZE],
    pub score: i32,
    pub placement: i32,
}
impl Default for PublicationPlayer {
    fn default() -> Self {
        Self {
            occupied: 0,
            identity: [0; 8],
            configuration: [0; CONFIGURATION_SIZE],
            score: 0,
            placement: 0,
        }
    }
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct PublicationTeam {
    pub occupied: u8,
    pub score: i32,
    pub placement: i32,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Publication {
    pub variant: [u8; VARIANT_SIZE],
    pub players: [PublicationPlayer; PLAYERS],
    pub teams: [PublicationTeam; PLAYERS],
    pub current: u8,
    pub finalized: u8,
    pub engine_present: bool,
}

/// Synchronous reads from the real native match or its caller-owned snapshot.
/// No Rust state guard, mutable Session borrow or native owner is retained.
pub trait Io {
    fn state(&self) -> Result<State>;
    fn roster(&self) -> Result<Vec<Admission>>;
    fn round_flags(&self) -> Result<u32>;
    fn variant(&self) -> Result<[u8; VARIANT_SIZE]>;
    fn recording(&self) -> Result<Recording>;
    fn ranks(&self, extended: bool, count: u32) -> Result<Ranks>;
    fn changing_teams(&self) -> Result<bool>;
    fn live_player(&self, slot: u32) -> Result<LivePlayer>;
    fn configuration(&self, slot: u32) -> Result<[u8; CONFIGURATION_SIZE]>;
    fn name(&self, roster_index: u32) -> Result<[u8; 64]>;
    fn team_score(&self, team: u32) -> Result<i32>;
    fn accumulated_score(&self, datum: u32) -> Result<i16>;
    fn write(&self, input: Publication) -> Result<()>;
}

pub fn publish(io: &impl Io) -> Result<()> {
    let state = io.state()?;
    let complete = "ONLINE postgame requires a completed admitted original match";
    need(
        state.finished && state.results_finalized && state.game_over_countdown == 0,
        complete,
    )?;
    need(io.round_flags()? & 0x20 != 0, complete)?;
    let roster = io.roster()?;
    need(!roster.is_empty() && roster.len() <= PLAYERS, complete)?;
    let mut slots = 0u16;
    for p in &roster {
        need(
            p.slot < PLAYERS as u32 && slots & (1u16 << p.slot) == 0,
            "Invalid native network result roster",
        )?;
        slots |= 1u16 << p.slot;
    }
    let variant = io.variant()?;
    let recording = io.recording()?;
    need(
        recording.statistics_present && recording.enabled != 0 && recording.frozen != 0,
        "ONLINE postgame requires actual frozen numeric statistics",
    )?;
    need(
        state.accuracy.is_some(),
        "ONLINE postgame lacks its original native accuracy observations",
    )?;
    let standings = io.ranks(variant[0x48] & 0x10 != 0, roster.len() as u32)?;
    let changing = io.changing_teams()?;
    let mut admitted_teams = 0u8;
    if state.team_mode {
        for p in &roster {
            need(
                (0..TEAMS as i32).contains(&p.team),
                "Invalid admitted ONLINE team",
            )?;
            admitted_teams |= 1u8 << p.team;
        }
        need(
            if changing {
                admitted_teams & state.retained_teams == admitted_teams
            } else {
                admitted_teams == state.retained_teams
            },
            "ONLINE postgame team history differs from admission",
        )?;
        admitted_teams = state.retained_teams;
    }
    let mut output = Publication {
        variant,
        players: std::array::from_fn(|_| PublicationPlayer::default()),
        teams: [PublicationTeam::default(); PLAYERS],
        current: 1,
        finalized: 1,
        engine_present: true,
    };
    for team in 0..TEAMS {
        if admitted_teams & (1u8 << team) != 0 {
            output.teams[team] = PublicationTeam {
                occupied: 1,
                score: io.team_score(team as u32)?,
                placement: i32::from(standings.teams[team] / 2),
            };
        }
    }
    for slot in 0..PLAYERS {
        let player = io.live_player(slot as u32)?;
        let admitted = roster
            .iter()
            .enumerate()
            .find(|(_, p)| p.slot == slot as u32);
        let Some((index, admitted)) = admitted else {
            need(
                !player.connected,
                "Unadmitted native network postgame participant",
            )?;
            continue;
        };
        need(
            player.connected
                && admitted.slot == slot as u32
                && player.datum != u32::MAX
                && player.datum & 0xffff == slot as u32
                && player.profile_present,
            "ONLINE postgame participant lacks its actual selected profile",
        )?;
        let mut configuration = io.configuration(slot as u32)?;
        configuration[..64].copy_from_slice(&io.name(index as u32)?);
        if state.team_mode {
            need(
                (0..TEAMS as i32).contains(&player.membership)
                    && state.retained_teams & (1u8 << player.membership) != 0
                    && (changing || player.membership == admitted.team),
                "ONLINE final team differs from its admitted rule and source history",
            )?;
        }
        configuration[0x7c] = if state.team_mode {
            player.membership as u8
        } else {
            0xff
        };
        output.players[slot] = PublicationPlayer {
            occupied: 1,
            identity: admitted.identity,
            configuration,
            placement: i32::from(standings.players[slot] / 2),
            score: i32::from(io.accumulated_score(player.datum)?),
        };
    }
    io.write(output)
}

pub fn validate(io: &impl Io, raw: &[u8]) -> Result<()> {
    need(raw.len() == PCR_SIZE, DIFFERENT)?;
    let roster = io.roster()?;
    need(!roster.is_empty() && roster.len() <= PLAYERS, DIFFERENT)?;
    let state = io.state()?;
    need(
        state.finished && state.results_finalized && state.game_over_countdown == 0,
        DIFFERENT,
    )?;
    let accuracy = state.accuracy.as_ref().ok_or(Error::State(DIFFERENT))?;
    need(
        raw[0] == 1
            && raw[1] == 1
            && raw[0x10] == 1
            && raw[0x11] == 1
            && raw[0x13] == u8::from(state.team_mode),
        DIFFERENT,
    )?;
    let mut slots = 0u16;
    for p in &roster {
        need(p.slot < PLAYERS as u32, DIFFERENT)?;
        need(slots & (1u16 << p.slot) == 0, DIFFERENT)?;
        slots |= 1u16 << p.slot;
    }
    let variant = io.variant()?;
    need(raw[0x20..0x150] == variant, DIFFERENT)?;
    let standings = io.ranks(variant[0x48] & 0x10 != 0, roster.len() as u32)?;
    let changing = io.changing_teams()?;
    let word = |at: usize| u16::from_le_bytes([raw[at], raw[at + 1]]);
    let mut admitted_teams = 0u8;
    for slot in 0..PLAYERS {
        let at = 0x370 + slot * 0x94;
        let p = &state.players[slot];
        let admitted = roster
            .iter()
            .enumerate()
            .find(|(_, p)| p.slot == slot as u32);
        let Some((index, admitted)) = admitted else {
            need(!p.connected && raw[at] == 0, DIFFERENT)?;
            continue;
        };
        need(
            p.connected && admitted.slot == slot as u32 && raw[at] == 1,
            DIFFERENT,
        )?;
        need(raw[at + 2..at + 10] == admitted.identity, DIFFERENT)?;
        let name = io.name(index as u32)?;
        need(raw[at + 12..at + 76] == name, DIFFERENT)?;
        if state.team_mode {
            need(
                (0..TEAMS as i32).contains(&admitted.team)
                    && (0..TEAMS as i32).contains(&p.membership),
                DIFFERENT,
            )?;
            need(changing || p.membership == admitted.team, DIFFERENT)?;
            admitted_teams |= 1u8 << admitted.team;
            need(state.retained_teams & (1u8 << p.membership) != 0, DIFFERENT)?;
        }
        need(
            raw[at + 0x88]
                == if state.team_mode {
                    p.membership as u8
                } else {
                    0xff
                },
            DIFFERENT,
        )?;
        need(
            i32::from(word(at + 0x92) as i16) == p.total_score,
            DIFFERENT,
        )?;
        need(raw[at + 0x90] == standings.players[slot] / 2, DIFFERENT)?;
        for (stat, value) in [
            (7, p.kills),
            (9, p.deaths),
            (8, p.assists),
            (11, p.suicides),
        ] {
            need(
                (0..=32767).contains(&value)
                    && i32::from(word(0xe30 + 2 * (slot * 0x1b5 + stat)) & 0x7fff) == value,
                DIFFERENT,
            )?;
        }
        for weapon in 0..WEAPONS {
            for kind in 0..2 {
                let at = 0xecc + 2 * (slot * 437 + (weapon + FIRST_WEAPON) * 8 + 4 + kind);
                need(word(at) & 0x7fff == accuracy[slot][weapon][kind], DIFFERENT)?;
            }
        }
    }
    need(
        !state.team_mode
            || if changing {
                admitted_teams & state.retained_teams == admitted_teams
            } else {
                admitted_teams == state.retained_teams
            },
        DIFFERENT,
    )?;
    if state.team_mode {
        admitted_teams = state.retained_teams;
    }
    for team in 0..PLAYERS {
        let at = 0xcb0 + team * 0x18;
        let present = team < TEAMS && (admitted_teams & (1u8 << team) != 0);
        need(raw[at] == u8::from(present), DIFFERENT)?;
        if present {
            need(raw[at + 1] == standings.teams[team] / 2, DIFFERENT)?;
            need(
                i32::from(word(at + 2) as i16) == state.team_scores[team],
                DIFFERENT,
            )?;
        }
    }
    Ok(())
}
