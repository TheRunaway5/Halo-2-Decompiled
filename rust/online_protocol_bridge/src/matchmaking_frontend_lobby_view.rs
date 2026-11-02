use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_lobby_view as core;

fn boolean(value: Option<bool>) -> ffi::LobbyViewBool {
    ffi::LobbyViewBool {
        present: value.is_some(),
        value: value.unwrap_or_default(),
    }
}
fn unsigned(value: Option<u32>) -> ffi::LobbyViewU32 {
    ffi::LobbyViewU32 {
        present: value.is_some(),
        value: value.unwrap_or_default(),
    }
}
fn signed(value: Option<i32>) -> ffi::LobbyViewI32 {
    ffi::LobbyViewI32 {
        present: value.is_some(),
        value: value.unwrap_or_default(),
    }
}
pub fn lobby_view_session(f: &ffi::LobbyViewSession) -> i32 {
    core::session(&core::Session {
        class: f.session_class,
        live_subtype: f.live_subtype,
        campaign_id: f.campaign_id,
        variant: f.variant.present.then_some(f.variant.value),
        settings_valid: f.settings_valid,
    })
}
pub fn lobby_view_pane(session: i32, previous: i32) -> i32 {
    core::pane(session, previous)
}
pub fn lobby_view_protocol(session: i32) -> u32 {
    core::protocol(session)
}
pub(crate) fn input(f: &ffi::LobbyViewInput) -> core::Input {
    core::Input {
        session: f.session,
        previous: f.previous,
        host: f.host,
        team_mode: f.team_mode,
        local_team_selection: f.local_team_selection,
        countdown: f.countdown,
        start_permission: f.start_permission,
        update_guard: f.update_guard,
        status: f.status,
        delayed_player: f.delayed_player,
        countdown_cancelled: f.countdown_cancelled,
        loading_map: f.loading_map,
        editing_options: f.editing_options,
        team_count: f.team_count,
        player_count: f.player_count,
        max_players: f.max_players,
        overflow_status: f.overflow_status,
        privacy: f.privacy,
        prior_stats: f.prior_stats,
    }
}
pub fn lobby_view_presentation(f: &ffi::LobbyViewInput) -> ffi::LobbyViewPresentation {
    let p = core::presentation(&input(f));
    ffi::LobbyViewPresentation {
        pane: p.pane,
        buttons_present: p.buttons.is_some(),
        button0: p.buttons.unwrap_or_default()[0],
        button1: p.buttons.unwrap_or_default()[1],
        leader_text: unsigned(p.leader_text),
        leader_visible: boolean(p.leader_visible),
        start_eligible: boolean(p.start_eligible),
        status: unsigned(p.status),
        status_warning: p.status_warning,
        subtitle: p.subtitle,
        privacy_text: unsigned(p.privacy_text),
        privacy_visible: boolean(p.privacy_visible),
        privacy: unsigned(p.privacy),
        prior_stats_text: unsigned(p.prior_stats_text),
        prior_stats_visible: boolean(p.prior_stats_visible),
    }
}
pub fn lobby_view_variant_type(engine: u32) -> ffi::LobbyViewI32 {
    signed(core::variant_type(engine))
}
pub fn lobby_view_game_icon(kind: i32) -> i32 {
    core::game_icon(kind)
}
pub fn lobby_view_quick_category(kind: i32) -> ffi::LobbyViewU32 {
    unsigned(core::quick_category(kind))
}
pub fn lobby_view_difficulty(value: i16) -> u32 {
    core::difficulty(value)
}
pub fn lobby_view_help(
    pane: i32,
    protocol: i32,
    machines: i32,
    privacy: i32,
    session: i32,
) -> ffi::LobbyViewHelp {
    let p = core::help(pane, protocol, machines, privacy, session);
    ffi::LobbyViewHelp {
        text: unsigned(p.text),
        visible: p.visible,
        value: unsigned(p.value),
        string_id: p.string,
    }
}
pub fn lobby_view_player_count(previous: i32, a: i32, b: i32) -> i32 {
    core::player_count(previous, a, b)
}
pub fn lobby_view_player_count_string(count: i32) -> ffi::LobbyViewU32 {
    unsigned(core::player_count_string(count))
}
struct Io<'a>(&'a ffi::NativeFrontendLobbyViewIo);
impl core::Io for Io<'_> {
    fn category(&self, id: u32) -> core::Result<Option<Vec<u32>>> {
        let f = self.0.lobby_view_category(id).map_err(|e| e.to_string())?;
        Ok(f.present.then_some(f.rows))
    }
    fn parameter(&self, tag: u32) -> core::Result<Option<core::Parameter>> {
        let f = self
            .0
            .lobby_view_parameter(tag)
            .map_err(|e| e.to_string())?;
        Ok(f.present.then(|| core::Parameter {
            parameter: f.parameter,
            title: f.title,
            choices: f
                .choices
                .into_iter()
                .map(|c| core::Choice {
                    label: c.label,
                    value: c.value,
                })
                .collect(),
        }))
    }
    fn value(&self, parameter: u32) -> core::Result<Option<i32>> {
        let f = self
            .0
            .lobby_view_value(parameter)
            .map_err(|e| e.to_string())?;
        Ok(f.present.then_some(f.value))
    }
    fn default_value(&self, engine: u32, parameter: u32) -> core::Result<Option<i32>> {
        let f = self
            .0
            .lobby_view_default(engine, parameter)
            .map_err(|e| e.to_string())?;
        Ok(f.present.then_some(f.value))
    }
}
pub fn lobby_view_default_value(
    engine: u32,
    parameter: u32,
    io: &ffi::NativeFrontendLobbyViewIo,
) -> core::Result<ffi::LobbyViewI32> {
    core::default_value(engine, parameter, &Io(io)).map(signed)
}
pub fn lobby_view_quick_options(
    engine: u32,
    separator: &[u8],
    count: usize,
    io: &ffi::NativeFrontendLobbyViewIo,
) -> core::Result<Vec<ffi::LobbyViewBytes>> {
    core::quick_options(engine, separator, count, &Io(io)).map(|rows| {
        rows.into_iter()
            .map(|value| ffi::LobbyViewBytes {
                present: value.is_some(),
                value: value.unwrap_or_default(),
            })
            .collect()
    })
}
