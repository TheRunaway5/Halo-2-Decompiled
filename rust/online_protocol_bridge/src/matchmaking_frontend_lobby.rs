//! Typed native pregame observations/effects. No callback or native pointer is
//! held by the Rust controller; the caller applies effects after each return.
use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_lobby as core;
pub struct FrontendLobby(core::Lobby);
pub struct FrontendLobbyPlan(core::Plan);
fn facts(f: &ffi::FrontendLobbyFacts) -> core::Facts {
    core::Facts {
        mode: f.mode,
        screens: f.screens.clone(),
        account_client: f.account_client,
        authenticated: f.authenticated,
        busy: f.busy,
        party_message_present: f.party_message_present,
        queue_client: f.queue_client,
        selected_playlist: f.selected_playlist.clone(),
        playlists: f
            .playlists
            .iter()
            .map(|p| core::Playlist {
                id: p.id.clone(),
                revision: p.revision.clone(),
                eligible: p.eligible,
            })
            .collect(),
        party: f.party_present.then(|| core::Party {
            leader: f.leader,
            countdown_supported: f.countdown_supported,
            countdown: f.countdown_present.then(|| core::Countdown {
                serial: f.serial,
                seconds: f.seconds_present.then_some(f.seconds),
                complete: f.complete,
            }),
            members: f
                .members
                .iter()
                .map(|m| core::Member {
                    handle: m.handle.clone(),
                    ready_playlist: m.ready_playlist.clone(),
                })
                .collect(),
        }),
    }
}
pub fn new_frontend_lobby() -> Box<FrontendLobby> {
    Box::new(FrontendLobby(core::Lobby::default()))
}
pub fn clone_frontend_lobby(l: &FrontendLobby) -> Box<FrontendLobby> {
    Box::new(FrontendLobby(l.0.clone()))
}
pub fn frontend_lobby_seconds(l: &FrontendLobby) -> i32 {
    l.0.seconds()
}
pub fn frontend_lobby_matchmaking(l: &FrontendLobby) -> bool {
    l.0.matchmaking()
}
pub fn frontend_lobby_leave_pending(l: &FrontendLobby) -> bool {
    l.0.leave_pending()
}
pub fn frontend_lobby_cancel(l: &mut FrontendLobby) {
    l.0.cancel();
}
pub fn frontend_lobby_exit(l: &mut FrontendLobby) {
    l.0.exit();
}
pub fn frontend_lobby_enter(l: &mut FrontendLobby, f: &ffi::FrontendLobbyFacts) {
    l.0.enter(&facts(f));
}
pub fn frontend_lobby_observe(
    l: &mut FrontendLobby,
    f: &ffi::FrontendLobbyFacts,
) -> ffi::FrontendLobbyObservation {
    let o = l.0.observe(&facts(f));
    ffi::FrontendLobbyObservation {
        revision: o.revision,
        beep: o.beep,
        departure: o.departure,
        search: o.search,
    }
}
pub fn frontend_lobby_start_error(l: &FrontendLobby, f: &ffi::FrontendLobbyFacts) -> String {
    l.0.start_error(&facts(f)).into()
}
pub fn frontend_lobby_plan(
    l: &FrontendLobby,
    f: &ffi::FrontendLobbyFacts,
    event: u32,
    destination: i32,
) -> Result<Box<FrontendLobbyPlan>, String> {
    let event = match event {
        0 => core::Input::Accept(destination),
        1 => core::Input::Back,
        2 => core::Input::Auxiliary,
        3 => core::Input::BeginLeave,
        _ => return Err("Invalid native lobby input".into()),
    };
    Ok(Box::new(FrontendLobbyPlan(l.0.plan(event, &facts(f)))))
}
pub fn frontend_lobby_plan_command(p: &FrontendLobbyPlan) -> u8 {
    match p.0.command {
        core::Command::None => 0,
        core::Command::Start => 1,
        core::Command::Cancel => 2,
        core::Command::Request => 3,
        core::Command::Leave => 4,
    }
}
pub fn frontend_lobby_plan_playlist(p: &FrontendLobbyPlan) -> String {
    p.0.playlist.clone()
}
pub fn frontend_lobby_plan_revision(p: &FrontendLobbyPlan) -> String {
    p.0.revision.clone()
}
pub fn frontend_lobby_finish_input(
    l: &mut FrontendLobby,
    p: Box<FrontendLobbyPlan>,
    accepted: bool,
) -> ffi::FrontendLobbyInputEffects {
    let e = l.0.finish_input(p.0, accepted);
    ffi::FrontendLobbyInputEffects {
        handled: e.handled,
        sound: e.sound,
        bounded_sound: e.bounded_sound,
        error: e.error,
        confirm_leave: e.confirm_leave,
        friends: e.friends,
        refresh_account: e.refresh_account,
        begin_leave: e.begin_leave,
    }
}
pub fn frontend_lobby_observe_leave(l: &mut FrontendLobby, f: &ffi::FrontendLobbyFacts) -> bool {
    l.0.observe_leave(&facts(f))
}
pub fn frontend_lobby_left(l: &mut FrontendLobby) {
    l.0.left()
}
pub fn frontend_lobby_view(l: &FrontendLobby) -> ffi::FrontendLobbyView {
    let v = l.0.view();
    ffi::FrontendLobbyView {
        matchmaking: v.matchmaking,
        seconds: v.seconds,
        stage: v.stage,
        consumed: v.consumed,
        counted: v.counted,
        leave_pending: v.leave_pending,
    }
}
