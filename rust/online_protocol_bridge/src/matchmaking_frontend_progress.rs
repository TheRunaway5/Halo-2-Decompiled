use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_progress as core;
fn countdown(c: (i32, bool)) -> ffi::ProgressCountdown {
    ffi::ProgressCountdown {
        seconds: c.0,
        ready: c.1,
    }
}
pub fn progress_countdown(elapsed: u32) -> ffi::ProgressCountdown {
    countdown(core::start_countdown(elapsed))
}
pub fn progress_published(present: bool, remaining: u32) -> ffi::ProgressCountdown {
    countdown(core::published_countdown(present.then_some(remaining)))
}
pub fn progress_pane(ui: i32, seconds: i32) -> u32 {
    core::pane(ui, seconds)
}
pub fn progress_pc_pane(ui: i32, seconds: i32) -> u32 {
    core::pc_pane(ui, seconds)
}
pub fn progress_sound(previous: u32, current: u32) -> i32 {
    core::pane_sound(previous, current).map_or(-1, |n| n as i32)
}
pub fn progress_title(ui: i32) -> u32 {
    core::title(ui).unwrap_or(0)
}
pub fn progress_titles(ui: i32) -> ffi::ProgressTitle {
    let xbox = core::title(ui);
    ffi::ProgressTitle {
        xbox: xbox.unwrap_or(0),
        pc: xbox.map_or(0, |n| n - 2),
    }
}
pub fn progress_cancel(pane: u32, elapsed: u32) -> bool {
    core::cancel_allowed(pane, elapsed)
}
pub fn progress_y(pane: u32) -> bool {
    core::consumes_y(pane)
}
pub fn progress_leave(party: usize, local: usize) -> u32 {
    core::leave_dialog(party, local)
}
pub fn progress_team_colors(countdown: i32, teams: bool) -> bool {
    core::team_colors(countdown, teams)
}
pub fn progress_uses_party(pane: u32, ranked: bool) -> bool {
    core::uses_party(pane, ranked)
}
pub fn progress_order(players: &[ffi::ProgressKey], ranked: bool, teams: bool) -> Vec<u32> {
    core::order(
        &players
            .iter()
            .map(|p| core::Player {
                team: p.team,
                value: p.value,
            })
            .collect::<Vec<_>>(),
        ranked,
        teams,
    )
}
pub fn progress_pane_order(
    players: &[ffi::ProgressKey],
    pane: u32,
    ranked: bool,
    teams: bool,
) -> Vec<u32> {
    core::pane_order(
        &players
            .iter()
            .map(|p| core::Player {
                team: p.team,
                value: p.value,
            })
            .collect::<Vec<_>>(),
        pane,
        ranked,
        teams,
    )
}
pub fn progress_rows(
    pane: u32,
    ranked: bool,
    party: u32,
    game: u32,
    present: bool,
    maximum: u32,
) -> Vec<u8> {
    core::rows(pane, ranked, party, game, present.then_some(maximum))
        .into_iter()
        .map(kind)
        .collect()
}
pub fn progress_category(kind: i32) -> i32 {
    core::option_category(kind).map_or(-1, |n| n as i32)
}
pub fn progress_icon(kind: i32) -> u32 {
    core::game_icon(kind)
}
pub fn progress_needed(available: bool, count: i32, required: i32) -> u8 {
    core::needed(available, count, required)
}
pub struct ProgressTipClock(core::TipClock);
pub fn new_progress_tip_clock(last: u32, initialized: bool) -> Box<ProgressTipClock> {
    Box::new(ProgressTipClock(core::TipClock::new(last, initialized)))
}
pub fn clone_progress_tip_clock(clock: &ProgressTipClock) -> Box<ProgressTipClock> {
    Box::new(ProgressTipClock(clock.0.clone()))
}
pub fn progress_tip_update(clock: &mut ProgressTipClock, now: u32) -> bool {
    clock.0.update(now)
}
pub fn progress_tip_index(mut seed: u32, count: u32) -> ffi::ProgressTip {
    let index = core::tip_index(&mut seed, count);
    ffi::ProgressTip {
        present: index.is_some(),
        index: index.unwrap_or(0),
        seed,
    }
}
pub fn progress_lobby_press(current: i32) -> i32 {
    halo2_online_protocol::party_countdown::press(current)
}
pub fn progress_lobby_due(current: i32, elapsed: u32) -> bool {
    halo2_online_protocol::party_countdown::tick(current, elapsed) != current
}
pub fn progress_lobby_beeps(previous: i32, current: i32) -> bool {
    halo2_online_protocol::matchmaking_frontend_lobby::beeps(previous, current)
}
fn members(input: &[ffi::ProgressMember]) -> Vec<core::Member> {
    input
        .iter()
        .map(|m| core::Member {
            key: core::Player {
                team: m.key.team,
                value: m.key.value,
            },
            level: m.level_present.then_some(m.level),
            self_player: m.is_self,
            local_controller: m.local_controller,
            handle: m.handle.clone(),
        })
        .collect()
}
fn facts(f: &ffi::ProgressFacts) -> core::Facts {
    core::Facts {
        pane: f.pane,
        ui: f.ui,
        status: f.status,
        ranked: f.ranked,
        teams: f.teams,
        party_present: f.party_present,
        match_present: f.match_present,
        gathering_present: f.gathering_present,
        party: members(&f.party),
        local: members(&f.local),
        matched: members(&f.matched),
        gathering: members(&f.gathering),
        maximum: f.maximum,
        viewer_level: f.viewer_level_present.then_some(f.viewer_level),
        party_voice: f.party_voice,
        voice_joined: f.voice_joined,
        microphone_muted: f.microphone_muted,
        transmitting: f.transmitting,
        talking: f.talking.clone(),
    }
}
fn kind(k: core::RowKind) -> u8 {
    match k {
        core::RowKind::Player => 0,
        core::RowKind::Matched => 1,
        core::RowKind::Empty => 2,
    }
}
fn row(r: Option<core::Row>) -> ffi::ProgressRow {
    match r {
        Some(r) => ffi::ProgressRow {
            present: true,
            kind: kind(r.kind),
            source: match r.source {
                core::Source::None => 0,
                core::Source::Party => 1,
                core::Source::Local => 2,
                core::Source::Match => 3,
                core::Source::Gathering => 4,
            },
            index: r.index,
            rank: r.rank.map_or(-1, |n| n as i32),
            speaker: r.speaker.map_or(-1, |n| n as i32),
        },
        None => ffi::ProgressRow {
            present: false,
            kind: 2,
            source: 0,
            index: 0,
            rank: -1,
            speaker: -1,
        },
    }
}
pub fn progress_project(f: &ffi::ProgressFacts) -> Vec<ffi::ProgressRow> {
    core::project(&facts(f))
        .into_iter()
        .map(|r| row(Some(r)))
        .collect()
}
pub fn progress_query(
    f: &ffi::ProgressFacts,
    screen: i32,
    entries: &[i32],
    index: u32,
) -> ffi::ProgressRow {
    row(core::query(&facts(f), screen, entries, index))
}
