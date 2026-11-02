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
