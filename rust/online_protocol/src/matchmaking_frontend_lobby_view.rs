//! Source-backed lobby presentation and quick-option selection. Native code
//! supplies query results, immutable cache records and the gameplay default
//! getter; all selection, retention and formatting rules live here.
use std::collections::BTreeMap;
pub type Result<T> = std::result::Result<T, String>;

#[derive(Clone, Debug)]
pub struct Session {
    pub class: i32,
    pub live_subtype: i32,
    pub campaign_id: i32,
    pub variant: Option<i32>,
    pub settings_valid: bool,
}
impl Default for Session {
    fn default() -> Self {
        Self {
            class: -1,
            live_subtype: 0,
            campaign_id: -1,
            variant: None,
            settings_valid: false,
        }
    }
}
pub fn session(input: &Session) -> i32 {
    let campaign = input.settings_valid && input.campaign_id != -1;
    let variant = input.variant.is_some_and(|v| matches!(v, 1..=4 | 7..=9));
    if matches!(input.class, 0 | 1) {
        if campaign {
            return input.class * 2;
        }
        if variant {
            return input.class * 2 + 1;
        }
    } else if input.class == 2 {
        if input.live_subtype == 2 {
            return 6;
        }
        if input.live_subtype == 1 {
            if campaign {
                return 4;
            }
            if variant {
                return 5;
            }
        }
    }
    -1
}
pub fn pane(session: i32, previous: i32) -> i32 {
    match session {
        0 | 2 | 4 => 1,
        1 | 3 | 5 => 0,
        6 => 2,
        _ => previous,
    }
}
pub fn protocol(session: i32) -> u32 {
    match session {
        0 => 0x100005be,
        1 => 0x120005c0,
        2 => 0x100005c1,
        3 => 0x120005c3,
        4 => 0x090005c4,
        5 => 0x0b0005c6,
        6 => 0x0e0005c8,
        _ => 0,
    }
}
#[derive(Clone, Debug)]
pub struct Input {
    pub session: i32,
    pub previous: i32,
    pub host: bool,
    pub team_mode: bool,
    pub local_team_selection: bool,
    pub countdown: bool,
    pub start_permission: bool,
    pub update_guard: bool,
    pub status: i32,
    pub delayed_player: i32,
    pub countdown_cancelled: bool,
    pub loading_map: bool,
    pub editing_options: bool,
    pub team_count: i32,
    pub player_count: i32,
    pub max_players: i32,
    pub overflow_status: u32,
    pub privacy: i32,
    pub prior_stats: i32,
}
impl Default for Input {
    fn default() -> Self {
        Self {
            session: -1,
            previous: 0,
            host: false,
            team_mode: false,
            local_team_selection: false,
            countdown: false,
            start_permission: false,
            update_guard: false,
            status: -1,
            delayed_player: -1,
            countdown_cancelled: false,
            loading_map: false,
            editing_options: false,
            team_count: 1,
            player_count: 0,
            max_players: 0,
            overflow_status: 0,
            privacy: 0,
            prior_stats: 0,
        }
    }
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Presentation {
    pub pane: i32,
    pub buttons: Option<[bool; 2]>,
    pub leader_text: Option<u32>,
    pub leader_visible: Option<bool>,
    pub start_eligible: Option<bool>,
    pub status: Option<u32>,
    pub status_warning: bool,
    pub subtitle: u32,
    pub privacy_text: Option<u32>,
    pub privacy_visible: Option<bool>,
    pub privacy: Option<u32>,
    pub prior_stats_text: Option<u32>,
    pub prior_stats_visible: Option<bool>,
}
fn campaign(session: i32) -> bool {
    matches!(session, 0 | 2 | 4)
}
fn status(f: &Input) -> u32 {
    match f.status {
        0 => 0x0a0001e3,
        1 => 0x0b0001e4,
        2 => 0x0f000205,
        3 | 24 => 0x0f000206,
        4 => 0x0a000208,
        5 => 0x11000209,
        6 => 0x190007bb,
        7 => {
            if campaign(f.session) {
                0x0e0001ec
            } else {
                0x0c0001e5
                    + if (2..=8).contains(&f.team_count) {
                        (f.team_count - 2) as u32
                    } else {
                        0
                    }
            }
        }
        8 => {
            if f.editing_options {
                0x170001ee
            } else {
                0x100001ed
            }
        }
        9 => {
            if f.player_count > f.max_players {
                f.overflow_status
            } else {
                0x240001ef
            }
        }
        10 => {
            if f.delayed_player != -1 {
                if f.countdown_cancelled {
                    0x1200020d
                } else {
                    0x1b00020c
                }
            } else if f.loading_map {
                if campaign(f.session) {
                    0x1500015d
                } else {
                    0x0a000208
                }
            } else if f.host {
                0x200001f0
            } else {
                0x2b00020b
            }
        }
        11 => 0x0c0001f2,
        12 => 0x0e0001f3,
        13 | 14 => 0x210001f4,
        15 => 0x230001f5,
        16 => 0x0f0001f7,
        17 => 0x0f0001f8,
        18 => 0x140001f9,
        19 => 0x190001fa,
        20 => 0x160001fb,
        21 => 0x0f0001fc,
        22 => 0x1c0001f6,
        25 => 0x1a000207,
        _ => 0,
    }
}
fn warning(status: i32) -> bool {
    matches!(
        status,
        2 | 5 | 6 | 7 | 11 | 12 | 13 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 25
    )
}
pub fn presentation(f: &Input) -> Presentation {
    let mut out = Presentation {
        pane: pane(f.session, f.previous),
        ..Presentation::default()
    };
    let known = (0..=2).contains(&out.pane);
    if !f.update_guard {
        out.buttons = Some([f.host, f.host]);
        out.start_eligible =
            Some(f.host && !f.local_team_selection && !f.countdown && f.start_permission);
        if known {
            out.leader_text = Some(if out.pane == 1 { 38 } else { 43 });
            out.leader_visible = Some(!f.host);
        }
    }
    out.subtitle = if f.countdown {
        if f.host { 0x2700021b } else { 0x2000021a }
    } else if f.host {
        if f.team_mode { 0x25000217 } else { 0x26000216 }
    } else if f.team_mode {
        0x1d000219
    } else {
        0x1e000218
    };
    if known {
        out.status = Some(status(f));
        out.status_warning = warning(f.status);
        out.prior_stats_text = Some(if out.pane == 1 { 40 } else { 45 });
        out.prior_stats_visible = Some(f.prior_stats > 0);
        out.privacy_text = Some(if out.pane == 1 { 5 } else { 10 });
        let visible = f.session != 0 && f.session != 1;
        out.privacy_visible = Some(visible);
        if visible {
            out.privacy = match f.privacy {
                0 => Some(0x04000201),
                1 => Some(0x0f000202),
                2 => Some(0x06000203),
                _ => None,
            };
        }
    }
    out
}
pub fn variant_type(engine: u32) -> Option<i32> {
    [2, 3, 7, 4, 1, 9, 8].get(engine as usize).copied()
}
pub fn game_icon(kind: i32) -> i32 {
    match kind {
        1 => 6,
        2 => 0,
        3 => 3,
        4 => 1,
        7 => 4,
        8 => 8,
        9 => 7,
        _ => 10,
    }
}
pub fn quick_category(kind: i32) -> Option<u32> {
    match kind {
        1 => Some(22),
        2 => Some(23),
        3 => Some(24),
        4 => Some(25),
        7 => Some(28),
        8 => Some(29),
        9 => Some(30),
        _ => None,
    }
}
pub fn difficulty(value: i16) -> u32 {
    match value {
        0 => 0x0400028e,
        2 => 0x0600028f,
        3 => 0x09000290,
        _ => 0x060000b8,
    }
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Help {
    pub text: Option<u32>,
    pub visible: bool,
    pub value: Option<u32>,
    pub string: u32,
}
pub fn help(pane: i32, protocol: i32, machines: i32, privacy: i32, session: i32) -> Help {
    let mut out = Help::default();
    out.text = match pane {
        0 => Some(46),
        1 => Some(41),
        2 => Some(47),
        _ => return out,
    };
    out.visible = protocol == 2 && machines <= 1;
    if !out.visible {
        return out;
    }
    let value = if privacy == 2 {
        0
    } else if privacy == 1 {
        1
    } else if session == 6 {
        3
    } else {
        2
    };
    out.value = Some(value);
    out.string = [0x17001824, 0x16001825, 0x18001826, 0x16001827][value as usize];
    out
}
pub fn player_count(previous: i32, a: i32, b: i32) -> i32 {
    let count = 16.min(a.max(b));
    if count > 0 { count } else { previous }
}
pub fn player_count_string(count: i32) -> Option<u32> {
    if !(0..=16).contains(&count) {
        None
    } else if count < 10 {
        Some(0x0100030a + count as u32)
    } else {
        Some(0x02000314 + (count - 10) as u32)
    }
}
#[derive(Clone, Debug, Default)]
pub struct Choice {
    pub label: Vec<u8>,
    pub value: i32,
}
#[derive(Clone, Debug, Default)]
pub struct Parameter {
    pub parameter: u32,
    pub title: Vec<u8>,
    pub choices: Vec<Choice>,
}
#[derive(Clone, Debug, Default)]
pub struct QuickFacts {
    pub categories: BTreeMap<u32, Vec<u32>>,
    pub parameters: BTreeMap<u32, Parameter>,
    pub values: BTreeMap<u32, i32>,
}
pub trait Io {
    fn category(&self, id: u32) -> Result<Option<Vec<u32>>>;
    fn parameter(&self, tag: u32) -> Result<Option<Parameter>>;
    fn value(&self, parameter: u32) -> Result<Option<i32>>;
    fn default_value(&self, engine: u32, parameter: u32) -> Result<Option<i32>>;
}
pub fn default_value(engine: u32, parameter: u32, io: &impl Io) -> Result<Option<i32>> {
    if engine >= 7 || parameter >= 112 {
        Ok(None)
    } else {
        io.default_value(engine, parameter)
    }
}
pub fn quick_options(
    engine: u32,
    separator: &[u8],
    count: usize,
    io: &impl Io,
) -> Result<Vec<Option<Vec<u8>>>> {
    let mut out = vec![None; count];
    let Some(category) = variant_type(engine).and_then(quick_category) else {
        return Ok(out);
    };
    let Some(rows) = io.category(category)? else {
        return Ok(out);
    };
    for (row, tag) in rows.iter().take(count).enumerate() {
        let Some(parameter) = io.parameter(*tag)? else {
            continue;
        };
        if parameter.title.is_empty() {
            continue;
        }
        let value = match io.value(parameter.parameter)? {
            Some(value) => Some(value),
            None => default_value(engine, parameter.parameter, io)?,
        };
        let Some(value) = value else { continue };
        let mut text = parameter.title.clone();
        text.extend_from_slice(separator);
        if let Some(choice) = parameter.choices.iter().find(|c| c.value == value) {
            text.extend_from_slice(&choice.label)
        }
        out[row] = Some(text);
    }
    Ok(out)
}
