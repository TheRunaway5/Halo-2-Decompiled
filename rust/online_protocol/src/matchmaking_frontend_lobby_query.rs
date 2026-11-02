//! Complete native lobby query caller. This preserves the modern producer's
//! observations; missing original Live status/rank producers remain unknown.
use crate::matchmaking_frontend_lobby_view as lobby;
pub type Result<T> = std::result::Result<T, String>;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Edit {
    pub valid: bool,
    pub team: i16,
}
impl Edit {
    pub const EMPTY: Self = Self {
        valid: false,
        team: -1,
    };
}
impl Default for Edit {
    fn default() -> Self {
        Self::EMPTY
    }
}
#[derive(Clone, Debug, Default)]
pub struct Map {
    pub scenario: Vec<u8>,
    pub campaign_id: i32,
    pub counts: Vec<u8>,
}
#[derive(Clone, Debug, Default)]
pub struct Member {
    pub handle: Vec<u8>,
    pub name: Vec<u8>,
    pub is_self: bool,
    pub leader: bool,
    pub appearance: bool,
}
#[derive(Clone, Debug, Default)]
pub struct SystemPlayer {
    pub name: Vec<u8>,
    pub team: i32,
    pub session_index: i32,
}
#[derive(Clone, Debug)]
pub struct Facts {
    pub mode: i32,
    pub party: bool,
    pub leader: bool,
    pub members: Vec<Member>,
    pub requester: Vec<u8>,
    pub privacy: Option<u32>,
    pub voice_party: bool,
    pub joined: bool,
    pub transmitting: bool,
    pub microphone_muted: bool,
    pub talking: Vec<Vec<u8>>,
    pub muted: Vec<Vec<u8>>,
    pub engine: u32,
    pub difficulty: u32,
    pub level: u32,
    pub map_index: u32,
    pub protocol: i32,
    pub profile: bool,
    pub team: i32,
    pub edit: Edit,
    pub edits: [Edit; 4],
    pub local_indices: [i32; 4],
    pub countdown: Option<i32>,
    pub system_host: Option<bool>,
    pub system_machines: u32,
    pub system_players: Vec<SystemPlayer>,
    pub campaign_maps: Vec<Map>,
    pub multiplayer_maps: Vec<Map>,
}
impl Default for Facts {
    fn default() -> Self {
        Self {
            mode: 0,
            party: false,
            leader: false,
            members: vec![],
            requester: vec![],
            privacy: None,
            voice_party: false,
            joined: false,
            transmitting: false,
            microphone_muted: false,
            talking: vec![],
            muted: vec![],
            engine: 0,
            difficulty: 0,
            level: 0,
            map_index: 0,
            protocol: 0,
            profile: false,
            team: 0,
            edit: Edit::EMPTY,
            edits: [Edit::EMPTY; 4],
            local_indices: [0, 1, 2, 3],
            countdown: None,
            system_host: None,
            system_machines: 0,
            system_players: vec![],
            campaign_maps: vec![],
            multiplayer_maps: vec![],
        }
    }
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MapRef {
    pub campaign: bool,
    pub index: usize,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Row {
    pub system: bool,
    pub source: usize,
    pub session_index: i32,
    pub name: Vec<u8>,
    pub appearance: bool,
}
#[derive(Clone, Debug)]
pub struct View {
    pub input: lobby::Input,
    pub protocol: i32,
    pub machines: i32,
    pub engine: u32,
    pub difficulty: u32,
    pub map: Option<MapRef>,
    pub profile: bool,
    pub team: i32,
    pub edit: Edit,
    pub edits: [Edit; 4],
    pub local_indices: [i32; 4],
    pub team_count: u32,
    pub allow_none: bool,
    pub quick: bool,
    pub names: Option<Vec<Vec<u8>>>,
    pub speakers: Vec<u32>,
    pub rows: Vec<Row>,
}
impl Default for View {
    fn default() -> Self {
        Self {
            input: lobby::Input::default(),
            protocol: -1,
            machines: 1,
            engine: 0,
            difficulty: 0,
            map: None,
            profile: false,
            team: 0,
            edit: Edit::EMPTY,
            edits: [Edit::EMPTY; 4],
            local_indices: [0, 1, 2, 3],
            team_count: 1,
            allow_none: false,
            quick: false,
            names: None,
            speakers: vec![],
            rows: vec![],
        }
    }
}
pub trait Io {
    fn facts(&self) -> Result<Facts>;
    fn start_error(&self) -> Result<Vec<u8>>;
    fn seconds(&self) -> Result<i32>;
    fn members(&self) -> Result<Vec<Member>>;
    fn locals(&self) -> Result<Vec<i32>>;
    fn has_stats(&self) -> Result<bool>;
    fn prior_count(&self) -> Result<i32>;
    fn campaign_file(&self, level: u32) -> Result<Vec<u8>>;
    fn profile_option(&self, screen: u32, parameter: u32) -> Result<i32>;
    fn variant(&self) -> Result<()>;
    fn defusal(&self) -> Result<bool>;
    fn value(&self, parameter: u32) -> Result<i32>;
    fn appearance(&self) -> Result<()>;
    fn custom(&self) -> Result<bool>;
    fn capacity(&self, parameter: u32) -> Result<i32>;
    fn quick(&self, engine: u32, values: &[(u32, i32)], screen: u32, separator: u32) -> Result<()>;
}
// Original-backed 2510E1/19C5F0 and the bounded established-local-host19A2CE
// branch, already used by the captured native caller. No online state inferred.
pub fn team_count(counts: &[u8], kind: i32, valid: bool) -> u32 {
    if valid && (1..10).contains(&kind) {
        if let Some(&count) = counts.get(kind as usize) {
            if count > 1 {
                return count as u32;
            }
        }
    }
    1
}
pub fn ready(
    session: i32,
    present: i32,
    count: i32,
    team: bool,
    distinct: i32,
    maximum: i32,
) -> i32 {
    if matches!(session, 0 | 2 | 4) {
        if count == 2 && present == count { 9 } else { 7 }
    } else if team && (distinct < 1 || distinct > maximum) {
        7
    } else {
        9
    }
}
pub fn query(io: &impl Io) -> Result<Option<View>> {
    let f = io.facts()?;
    if f.mode == 3 {
        let mut v = View::default();
        v.protocol = 2;
        v.input.session = 6;
        v.input.host = f.party && f.leader;
        v.input.start_permission = io.start_error()?.is_empty();
        v.input.status = -1;
        v.input.countdown = io.seconds()? >= 0;
        if v.input.countdown {
            v.input.status = 10;
        }
        if f.party && !f.requester.is_empty() {
            if let Some((i, m)) = f
                .members
                .iter()
                .enumerate()
                .find(|(_, m)| m.handle == f.requester)
            {
                v.input.delayed_player = i as u32 as i32;
                v.input.countdown_cancelled = m.leader;
            }
        }
        v.input.privacy = if f.party {
            f.privacy.map(|p| p as i32).unwrap_or(-1)
        } else {
            -1
        };
        v.input.max_players = 16;
        v.input.player_count = io.members()?.len() as u32 as i32;
        v.machines = if f.party {
            f.members.len() as u32 as i32
        } else {
            i32::from(!io.locals()?.is_empty())
        };
        v.input.prior_stats = if io.has_stats()? {
            io.prior_count()?
        } else {
            0
        };
        v.names = Some(vec![]);
        for (index, m) in io.members()?.into_iter().enumerate() {
            if v.rows.len() == 16 {
                break;
            }
            v.names.as_mut().unwrap().push(m.name.clone());
            v.rows.push(Row {
                system: false,
                source: index,
                session_index: v.rows.len() as i32,
                name: m.name,
                appearance: m.appearance,
            });
            let talking = f.voice_party
                && f.joined
                && ((m.is_self && f.transmitting) || f.talking.contains(&m.handle));
            let muted = f.muted.contains(&m.handle);
            v.speakers.push(if !m.is_self && !talking && !muted {
                3
            } else if f.voice_party && f.joined && !muted && !(m.is_self && f.microphone_muted) {
                u32::from(!talking)
            } else {
                2
            });
        }
        return Ok(Some(v));
    }
    if !matches!(f.mode, 1 | 2 | 4) {
        return Ok(None);
    }
    let mut v = View::default();
    v.protocol = if f.mode == 2 {
        1
    } else if f.mode == 4 {
        f.protocol
    } else {
        0
    };
    v.engine = f.engine;
    v.difficulty = f.difficulty;
    let campaign = f.mode == 4;
    let selected = if campaign {
        let mut selected = None;
        for (i, map) in f.campaign_maps.iter().enumerate() {
            let leaf = map
                .scenario
                .rsplit(|c| matches!(c, b'\\' | b'/'))
                .next()
                .unwrap();
            if leaf == io.campaign_file(f.level)? {
                selected = Some((
                    MapRef {
                        campaign: true,
                        index: i,
                    },
                    map,
                ));
                break;
            }
        }
        selected
    } else {
        f.multiplayer_maps.get(f.map_index as usize).map(|m| {
            (
                MapRef {
                    campaign: false,
                    index: f.map_index as usize,
                },
                m,
            )
        })
    };
    v.map = selected.map(|(r, _)| r);
    v.input.session = lobby::session(&lobby::Session {
        class: v.protocol,
        live_subtype: 1,
        campaign_id: if campaign {
            selected.map_or(-1, |(_, m)| m.campaign_id)
        } else {
            -1
        },
        variant: lobby::variant_type(f.engine),
        settings_valid: selected.is_some(),
    });
    v.input.host = true;
    v.input.privacy = io.profile_option(25, 0)?;
    io.variant()?;
    let defusal = !campaign && io.defusal()?;
    v.input.team_mode = !campaign && io.value(91)? != 0;
    v.allow_none = !campaign && io.value(13)? != 0;
    v.team = f.team;
    if f.system_host.is_some() {
        v.local_indices = f.local_indices;
    }
    v.edit = f.edit;
    v.edits = f.edits;
    v.edits[0] = f.edit;
    v.input.local_team_selection = v.edits.iter().any(|e| e.valid);
    v.team_count = team_count(
        selected.map_or(&[], |(_, m)| m.counts.as_slice()),
        lobby::variant_type(f.engine).unwrap_or(-1),
        selected.is_some(),
    );
    if defusal {
        v.team_count = 2;
        v.input.team_mode = true;
        v.allow_none = false;
    }
    v.input.team_count = v.team_count as i32;
    v.input.player_count = io.locals()?.len() as u32 as i32;
    if f.profile {
        io.appearance()?;
        v.profile = true;
    }
    v.input.max_players = if campaign { 2 } else { 16 };
    let mut mask = 0u16;
    for team in io.locals()? {
        //19A2CE counts only signed team IDs0..15. Keep the original bound
        // explicit: the former C++ caller's wider shifts have undefined inputs.
        if (0..16).contains(&team) && team < (v.team_count as i32) {
            mask |= 1u16 << team;
        }
    }
    if selected.is_some() && v.input.player_count > 0 {
        v.input.status = ready(
            v.input.session,
            v.input.player_count,
            v.input.player_count,
            v.input.team_mode,
            mask.count_ones() as i32,
            v.team_count as i32,
        );
    }
    v.input.start_permission = v.input.host;
    if f.countdown.is_some_and(|n| n >= 0) {
        v.input.countdown = true;
        v.input.start_permission = false;
        v.input.status = 10;
    }
    if !campaign {
        let mut values = Vec::with_capacity(112);
        for id in 0..112 {
            values.push((id, io.value(id)?));
        }
        if io.custom()? {
            values[90].1 = io.capacity(90)?;
        }
        io.quick(f.engine, &values, 14, 0x1a001820)?;
        v.quick = true;
    }
    if f.mode == 2 {
        if let Some(host) = f.system_host {
            v.input.host = host;
            v.input.start_permission = host && f.countdown.is_none();
            v.names = Some(vec![]);
            v.machines = f.system_machines as i32;
            let mut teams = 0u32;
            for (i, p) in f.system_players.iter().enumerate() {
                v.names.as_mut().unwrap().push(p.name.clone());
                v.speakers.push(2);
                v.rows.push(Row {
                    system: true,
                    source: i,
                    session_index: p.session_index,
                    name: p.name.clone(),
                    appearance: false,
                });
                if (0..8).contains(&p.team) {
                    teams |= 1u32 << p.team;
                }
            }
            v.input.player_count = f.system_players.len() as u32 as i32;
            if f.countdown.is_none() {
                v.input.status = ready(
                    v.input.session,
                    v.input.player_count,
                    v.input.player_count,
                    v.input.team_mode,
                    teams.count_ones() as i32,
                    v.team_count as i32,
                );
            }
        }
    }
    Ok(Some(v))
}
