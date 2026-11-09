//! Native ONLINE pregame caller state. The accepted service countdown is an
//! observation, never a local replacement clock. Effects are returned before
//! native account/input/render operations, so no Rust borrow crosses reentry.

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Member {
    pub handle: String,
    pub ready_playlist: String,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Countdown {
    pub serial: u64,
    pub seconds: Option<i32>,
    pub complete: bool,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Party {
    pub leader: bool,
    pub countdown_supported: bool,
    pub countdown: Option<Countdown>,
    pub members: Vec<Member>,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Playlist {
    pub id: String,
    pub revision: String,
    pub eligible: bool,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Facts {
    pub mode: i32,
    pub screens: Vec<u32>,
    pub account_client: bool,
    pub authenticated: bool,
    pub busy: bool,
    pub party_message_present: bool,
    pub party: Option<Party>,
    pub queue_client: bool,
    pub selected_playlist: String,
    pub playlists: Vec<Playlist>,
}
impl Facts {
    fn lobby(&self) -> bool {
        self.mode == 3 && self.screens.contains(&14)
    }
    fn screen(&self) -> Option<u32> {
        self.screens.last().copied()
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Lobby {
    matchmaking: bool,
    seconds: i32,
    stage: i32,
    consumed: u64,
    counted: Vec<String>,
    leave_pending: bool,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct View {
    pub matchmaking: bool,
    pub seconds: i32,
    pub stage: i32,
    pub consumed: u64,
    pub counted: Vec<String>,
    pub leave_pending: bool,
}
impl Default for Lobby {
    fn default() -> Self {
        Self {
            matchmaking: false,
            seconds: -1,
            stage: -1,
            consumed: 0,
            counted: Vec::new(),
            leave_pending: false,
        }
    }
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Observation {
    pub revision: bool,
    pub beep: bool,
    pub departure: bool,
    pub search: bool,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Input {
    Accept(i32),
    Back,
    Auxiliary,
    BeginLeave,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Command {
    #[default]
    None,
    Start,
    Cancel,
    Request,
    Leave,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
enum Completion {
    #[default]
    None,
    Refresh,
    BeginLeave,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
enum Refusal {
    #[default]
    None,
    Sound,
    Start,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Plan {
    pub handled: bool,
    pub command: Command,
    pub playlist: String,
    pub revision: String,
    effects: InputEffects,
    completion: Completion,
    refusal: Refusal,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct InputEffects {
    pub handled: bool,
    pub sound: bool,
    pub bounded_sound: bool,
    pub error: String,
    pub confirm_leave: bool,
    pub friends: bool,
    pub refresh_account: bool,
    pub begin_leave: bool,
}
impl Default for InputEffects {
    fn default() -> Self {
        Self {
            handled: false,
            sound: false,
            bounded_sound: true,
            error: String::new(),
            confirm_leave: false,
            friends: false,
            refresh_account: false,
            begin_leave: false,
        }
    }
}
impl Lobby {
    pub fn view(&self) -> View {
        View {
            matchmaking: self.matchmaking,
            seconds: self.seconds,
            stage: self.stage,
            consumed: self.consumed,
            counted: self.counted.clone(),
            leave_pending: self.leave_pending,
        }
    }
    pub fn matchmaking(&self) -> bool {
        self.matchmaking
    }
    pub fn seconds(&self) -> i32 {
        self.seconds
    }
    pub fn leave_pending(&self) -> bool {
        self.leave_pending
    }
    pub fn cancel(&mut self) {
        self.seconds = -1;
        self.stage = -1;
    }
    pub fn exit(&mut self) {
        self.matchmaking = false;
        self.cancel();
    }
    pub fn enter(&mut self, f: &Facts) {
        self.matchmaking = true;
        self.counted.clear();
        self.cancel();
        if let Some(c) = f
            .party
            .as_ref()
            .and_then(|p| p.countdown.as_ref())
            .filter(|c| c.complete)
        {
            self.consumed = c.serial;
        }
    }
    pub fn observe(&mut self, f: &Facts) -> Observation {
        let mut out = Observation::default();
        if self.leave_pending {
            return out;
        }
        let Some(party) = f
            .party
            .as_ref()
            .filter(|p| f.lobby() && p.countdown_supported)
        else {
            self.counted.clear();
            if self.seconds >= 0 {
                self.cancel();
                out.revision = true;
            }
            return out;
        };
        let published = party.countdown.as_ref();
        if published.is_some_and(|c| c.seconds.is_some_and(|s| s > 0)) && self.counted.is_empty() {
            self.counted = party.members.iter().map(|m| m.handle.clone()).collect();
        }
        if published.is_none() && !self.counted.is_empty() {
            let counted = std::mem::take(&mut self.counted);
            if counted
                .iter()
                .any(|h| !party.members.iter().any(|m| m.handle == *h))
            {
                self.cancel();
                return Observation {
                    revision: true,
                    departure: true,
                    ..out
                };
            }
        }
        let seconds = published
            .filter(|c| !c.complete || c.serial != self.consumed)
            .and_then(|c| c.seconds)
            .unwrap_or(-1);
        if seconds != self.seconds {
            self.seconds = seconds;
            out.revision = true;
        }
        out.beep = crate::matchmaking_frontend_lobby::beeps(self.stage, self.seconds);
        self.stage = self.seconds;
        if let Some(c) = published.filter(|c| c.complete && c.serial != self.consumed) {
            self.consumed = c.serial;
            self.counted.clear();
            self.cancel();
            out.search = true;
        }
        out
    }
    pub fn start_error(&self, f: &Facts) -> &'static str {
        if !f.account_client || !f.authenticated {
            return "Sign in to ONLINE before starting matchmaking.";
        }
        let Some(party) = &f.party else {
            return "Create or join a party before starting matchmaking.";
        };
        if !party.leader {
            return "Only the party leader can start matchmaking.";
        }
        if !self.matchmaking || f.selected_playlist.is_empty() {
            return "Choose a matchmaking playlist in Game Setup first.";
        }
        if !f.queue_client {
            return "No matchmaking service is configured.";
        }
        if !party.countdown_supported {
            return "This ONLINE service does not support the shared lobby countdown. Update the server before starting matchmaking.";
        }
        if f.busy {
            return "Wait for the current ONLINE operation to finish, then start matchmaking.";
        }
        let Some(playlist) = f
            .playlists
            .iter()
            .find(|p| p.id == f.selected_playlist)
            .filter(|p| p.eligible)
        else {
            return "The selected playlist is unavailable. Choose a playlist in Game Setup.";
        };
        if !party
            .members
            .iter()
            .all(|m| m.ready_playlist == playlist.id)
        {
            return "Every party member must select this playlist before matchmaking can start.";
        }
        ""
    }
    pub fn plan(&self, input: Input, f: &Facts) -> Plan {
        let mut out = Plan::default();
        let online = f.mode == 3 && f.screen() == Some(14);
        match input {
            Input::Accept(destination) if online && (self.seconds >= 0 || destination == -17) => {
                out.handled = true;
                out.effects.bounded_sound = false;
                let error = self.start_error(f);
                if !error.is_empty() {
                    out.effects.sound = true;
                    out.effects.error = error.into();
                } else {
                    let playlist = f
                        .playlists
                        .iter()
                        .find(|p| p.id == f.selected_playlist)
                        .unwrap();
                    out.command = Command::Start;
                    out.playlist = playlist.id.clone();
                    out.revision = playlist.revision.clone();
                    out.completion = Completion::Refresh;
                    out.refusal = Refusal::Start;
                }
            }
            Input::Back if online => {
                out.handled = true;
                if let Some(p) = f.party.as_ref().filter(|_| self.seconds >= 0) {
                    if p.leader {
                        if f.account_client {
                            out.command = Command::Cancel;
                            out.refusal = Refusal::Sound;
                        } else {
                            out.effects.sound = true;
                        }
                    } else {
                        if f.account_client {
                            out.command = Command::Request;
                        }
                        out.effects.confirm_leave = true;
                    }
                } else {
                    out.effects.confirm_leave = true;
                }
            }
            Input::Auxiliary if online => {
                out.handled = true;
                if self.seconds < 0 {
                    out.effects.friends = true;
                } else if let Some(p) = f.party.as_ref().filter(|_| f.account_client) {
                    out.command = if p.leader {
                        Command::Cancel
                    } else {
                        Command::Request
                    };
                    out.refusal = Refusal::Sound;
                } else {
                    out.effects.sound = true;
                }
            }
            Input::BeginLeave if f.lobby() => {
                out.handled = true;
                if !self.leave_pending {
                    if f.party.is_some() {
                        if f.account_client {
                            out.command = Command::Leave;
                            out.completion = Completion::BeginLeave;
                            out.refusal = Refusal::Sound;
                        } else {
                            out.effects.sound = true;
                        }
                    } else {
                        out.completion = Completion::BeginLeave;
                    }
                }
            }
            _ => {}
        }
        out.effects.handled = out.handled;
        out
    }
    pub fn finish_input(&mut self, plan: Plan, accepted: bool) -> InputEffects {
        let mut out = plan.effects;
        if accepted {
            match plan.completion {
                Completion::Refresh => out.refresh_account = true,
                Completion::BeginLeave => {
                    self.leave_pending = true;
                    out.begin_leave = true;
                }
                Completion::None => {}
            }
        } else {
            match plan.refusal {
                Refusal::None => {}
                Refusal::Sound => out.sound = true,
                Refusal::Start => {
                    out.sound = true;
                    out.error = "The ONLINE operation changed before Start Game was accepted. Wait for it to finish, then try again.".into();
                }
            }
        }
        out
    }
    /// The caller signs out its queue before committing native teardown. No
    /// command refusal can clear the accepted party or close its confirmation.
    pub fn observe_leave(&mut self, f: &Facts) -> bool {
        if !self.leave_pending || f.busy {
            return false;
        }
        if f.party.is_some() {
            if f.party_message_present {
                self.leave_pending = false;
            }
            return false;
        }
        self.leave_pending = false;
        true
    }
    pub fn left(&mut self) {
        self.matchmaking = false;
        self.counted.clear();
        self.cancel();
    }
}

/// TU1.5 250A8B presentation cue, separate from original timer publication.
pub fn beeps(previous: i32, stage: i32) -> bool {
    (0..=3).contains(&stage) && stage != previous
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SetupRoute {
    Open,
    SwitchCoop,
    SwitchCustom,
    AlternateSession,
    OpenPlaylist,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SetupRow {
    pub semantic: i32,
    pub label: u32,
    pub destination: i32,
    pub route: SetupRoute,
}

fn setup_row(semantic: i32) -> Option<SetupRow> {
    use SetupRoute::*;
    let (label, destination, route) = match semantic {
        0 => (0x0a0005d3, 206, Open),
        1 => (0x0e0005d4, 207, Open),
        2 => (0x0c0005d5, 11, Open),
        3 => (0x110005d6, 12, Open),
        4 => (0x0d00042a, 222, Open),
        6 => (0x0e0005db, 14, SwitchCoop),
        7 => (0x120005dd, 14, SwitchCustom),
        8 => (0x130005dc, 237, AlternateSession),
        9 => (0x0d0005d8, 237, AlternateSession),
        _ => return None,
    };
    Some(SetupRow {
        semantic,
        label,
        destination,
        route,
    })
}

pub fn setup_label(semantic: i32) -> u32 {
    setup_row(semantic).map_or(0, |row| row.label)
}

pub fn setup_rows(session: i32) -> impl Iterator<Item = SetupRow> {
    // Original Setup constructor uses these semantic values in datum order.
    let order: &[i32] = match session {
        0 | 2 => &[2, 3, 7],
        1 | 3 => &[0, 1, 4, 6],
        4 => &[2, 3, 7, 8],
        5 => &[0, 1, 4, 6, 8],
        6 => &[9, 6, 7],
        _ => &[],
    };
    order.iter().filter_map(|&semantic| setup_row(semantic))
}

pub fn setup_focus(session: i32, index: i32) -> i32 {
    usize::try_from(index)
        .ok()
        .and_then(|index| setup_rows(session).nth(index))
        .map_or(-1, |row| row.semantic)
}

pub fn setup_accept(
    session: i32,
    index: u32,
    online: bool,
    leader: bool,
) -> Result<Option<SetupRow>, &'static str> {
    let Some(mut row) = setup_rows(session).nth(index as usize) else {
        return Ok(None);
    };
    if online {
        if !leader {
            return Err("Only the party leader can change Game Setup.");
        }
        match row.semantic {
            6 => return Err("ONLINE cooperative sessions are not available from this service."),
            7 => return Err("ONLINE custom-game hosting is not available from this service."),
            9 => row.route = SetupRoute::OpenPlaylist,
            _ => {}
        }
    }
    if row.route == SetupRoute::AlternateSession {
        return Err("This network session mode is not implemented by the native runtime.");
    }
    Ok(Some(row))
}

pub fn setup_privacy(
    online: bool,
    session: Option<i32>,
    privacy_present: bool,
    leader: bool,
    busy: bool,
) -> Result<bool, &'static str> {
    if !online {
        return Ok(session.is_some_and(|session| !matches!(session, 0 | 1)));
    }
    // ONLINE is backed by the replacement party service. Its admitted
    // privacy capability can be absent even on an authored Setup screen.
    if !privacy_present {
        return Err("This ONLINE service does not support party privacy settings.");
    }
    if !leader {
        return Err("Only the party leader can change party privacy.");
    }
    if busy {
        return Err("Wait for the current ONLINE operation to finish.");
    }
    Ok(true)
}
