//! Native frontend search caller policy. Native snapshots are typed IO views;
//! the controller owns intent, consent, profile, pane and countdown state.
//! Mutex guards never span IO, including reentrant menu updates.
use std::sync::Mutex;
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Error {
    Policy(&'static str),
    Io,
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(match self {
            Self::Policy(s) => s,
            Self::Io => "Native frontend search IO failed",
        })
    }
}
pub type Result<T> = std::result::Result<T, Error>;
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Playlist {
    pub id: String,
    pub revision: String,
    pub eligible: bool,
    pub quickmatch: bool,
    pub clan_match: bool,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Member {
    pub handle: String,
    pub is_self: bool,
    pub ready: String,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Party {
    pub leader: bool,
    pub members: Vec<Member>,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Account {
    pub party: Option<Party>,
    pub busy: bool,
    pub message: Vec<u8>,
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Frame {
    pub screen: u32,
    pub context: u32,
    pub party_screen: bool,
    pub first_destination: Option<i32>,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Context {
    pub mode: i32,
    pub clock: u32,
    pub couch_revision: u64,
    pub frames: Vec<Frame>,
    pub party: Option<Party>,
    pub queue: bool,
    pub account: bool,
}
impl Context {
    fn shown(&self, id: u32) -> bool {
        self.frames.iter().any(|f| f.screen == id)
    }
    fn top(&self) -> u32 {
        self.frames.last().map_or(0, |f| f.screen)
    }
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Status {
    #[default]
    Unconfigured,
    Idle,
    Connecting,
    Ready,
    Joining,
    Searching,
    Checking,
    Awaiting,
    Loading,
    Failed,
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Snapshot {
    pub status: Status,
    pub playlists: Vec<Playlist>,
    pub matched: bool,
    pub gathering: bool,
    pub revision: u64,
    pub message_empty: bool,
    pub countdown_supported: bool,
    pub countdown_ms: Option<u32>,
    pub failure: Option<i32>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct State {
    pub active: bool,
    pub quick: bool,
    pub consent: bool,
    pub selected: String,
    pub name: Vec<u8>,
    pub couch_revision: u64,
    pub lobby_pending: bool,
    pub create_submitted: bool,
    pub countdown: i32,
    pub have_fun: bool,
    pub pane: u32,
    pub title: i32,
    pub pane_clock: u32,
    pub party: Vec<String>,
}
impl Default for State {
    fn default() -> Self {
        Self {
            active: false,
            quick: false,
            consent: false,
            selected: String::new(),
            name: vec![],
            couch_revision: 0,
            lobby_pending: false,
            create_submitted: false,
            countdown: -1,
            have_fun: false,
            pane: 0,
            title: 0,
            pane_clock: 0,
            party: vec![],
        }
    }
}
pub struct Controller(Mutex<State>);
impl Default for Controller {
    fn default() -> Self {
        Self(Mutex::new(State::default()))
    }
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Ui {
    ClearSnapshot,
    NoService(Vec<u8>),
    Failure(Vec<u8>),
    ClearFailure,
    ModeOnline,
    PopLobbyOrOnline,
    PopLobby,
    OpenLobby,
    EnterLobby,
    OpenSearch,
    Dialog(u32),
    Revision,
    RepopulateLobby,
    PopTop,
    Sound(u32),
    Render(u32),
    Unavailable(Vec<u8>),
    PlaylistError(Vec<u8>),
}
pub trait Io {
    fn context(&self) -> Result<Context>;
    fn view(&self) -> Result<Snapshot>;
    fn identity(&self) -> Result<bool>;
    fn profile(&self) -> Result<Vec<u8>>;
    fn local_count(&self) -> Result<u32>;
    fn pull_after(&self, revision: u64) -> Result<bool>;
    fn pull(&self) -> Result<()>;
    fn account_snapshot(&self) -> Result<Account>;
    fn command(&self, action: &str, playlist: &str, revision: &str) -> Result<bool>;
    fn sign_out(&self) -> Result<()>;
    fn refresh(&self, name: &[u8]) -> Result<()>;
    fn cancel(&self) -> Result<()>;
    fn join(&self, playlist: &str) -> Result<()>;
    fn lobby_countdown(&self) -> Result<bool>;
    fn launch(&self) -> Result<bool>;
    fn has_launch(&self) -> Result<bool>;
    fn ui(&self, effect: Ui) -> Result<()>;
}
fn screen(f: &Frame) -> bool {
    f.screen == 210 || ((f.screen == 219 || f.screen == 237) && f.context == 0)
}
fn choose(io: &impl Io, s: &Snapshot, id: &str, keep_invalid: bool) -> Result<Option<Playlist>> {
    let current = s.playlists.iter().find(|p| p.id == id);
    if current.is_some_and(|p| p.eligible) {
        return Ok(current.cloned());
    }
    let mut fallback = s.playlists.iter().find(|p| p.quickmatch && p.eligible);
    if fallback.is_none() && (io.context()?.party.is_some() || io.local_count()? > 1) {
        fallback = s.playlists.iter().find(|p| p.eligible && !p.clan_match)
    }
    Ok(fallback
        .or(if keep_invalid { current } else { None })
        .cloned())
}
pub fn ui_state(s: &Snapshot, fun: bool) -> i32 {
    if fun {
        return 8;
    }
    match s.status {
        Status::Failed => 1,
        Status::Searching => {
            if s.gathering {
                4
            } else {
                3
            }
        }
        Status::Checking | Status::Awaiting => {
            if s.matched {
                5
            } else {
                3
            }
        }
        Status::Loading => {
            if s.matched {
                7
            } else {
                3
            }
        }
        _ => 0,
    }
}
pub fn pane(ui: i32, countdown: i32) -> u32 {
    crate::matchmaking_frontend_progress::pane(ui, countdown)
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct VariantChoice {
    pub kind: u32,
    pub index: u32,
}
pub fn variant_choice(
    name: &[u8],
    settings: Option<&[u8]>,
    content: &[Vec<u8>],
    bundled: &[Vec<u8>],
) -> VariantChoice {
    if let Some(raw) = settings {
        if !crate::matchmaking_variant::valid(raw) {
            return VariantChoice::default();
        }
        let source = i32::from_le_bytes(raw[0x44..0x48].try_into().unwrap());
        return [2, 3, 7, 4, 1, 9, 8]
            .iter()
            .position(|v| *v == source)
            .map_or_else(VariantChoice::default, |i| VariantChoice {
                kind: 1,
                index: i as u32,
            });
    }
    for (i, c) in content.iter().enumerate() {
        if c.strip_suffix(b" \xee\x80\x88").unwrap_or(c) == name {
            return VariantChoice {
                kind: 2,
                index: i as u32,
            };
        }
    }
    bundled
        .iter()
        .position(|v| v == name)
        .map_or_else(VariantChoice::default, |i| VariantChoice {
            kind: 3,
            index: i as u32,
        })
}
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Take {
    pub present: bool,
    pub team_mode: Option<bool>,
}
pub trait LaunchIo {
    fn variant(&self) -> Result<Option<bool>>;
    fn map(&self) -> Result<Vec<u8>>;
    fn prepare_map(&self, map: &[u8]) -> Result<()>;
    fn self_present(&self) -> Result<bool>;
    fn take(&self) -> Result<Take>;
    fn local_counts(&self) -> Result<(u32, u32)>;
    fn local_count(&self, count: u32) -> Result<()>;
    fn admitted_name(&self, index: u32) -> Result<Vec<u8>>;
    fn profile_name(&self, index: u32) -> Result<Vec<u8>>;
    fn append_profile(&self, index: u32, teams: bool) -> Result<()>;
    fn validate_map(&self) -> Result<bool>;
    fn reject_content(&self) -> Result<()>;
    fn publish(&self) -> Result<()>;
    fn discard(&self) -> Result<()>;
    fn failure(&self) -> Result<(bool, Vec<u8>)>;
}
fn require(ok: bool, message: &'static str) -> Result<()> {
    if ok {
        Ok(())
    } else {
        Err(Error::Policy(message))
    }
}
pub fn launch(io: &impl LaunchIo) -> Result<bool> {
    let teams = io.variant()?.ok_or(Error::Policy(
        "Playlist selected an unavailable game variant.",
    ))?;
    let map = io.map()?;
    require(
        map.iter()
            .all(|c| c.is_ascii_lowercase() || c.is_ascii_digit() || *c == b'_'),
        "Invalid matchmaking map.",
    )?;
    let mut path = map;
    path.extend_from_slice(b".map");
    io.prepare_map(&path)?;
    require(io.self_present()?, "Matchmaking omitted the local player.")?;
    let connection = io.take()?;
    if connection.present {
        require(
            connection.team_mode.is_none_or(|value| value == teams),
            "Matchmaking team mode differs from the selected variant",
        )?;
        let (profiles, slots) = io.local_counts()?;
        require(
            profiles == slots,
            "Couch profiles changed before match launch.",
        )?;
        io.local_count(profiles)?;
        for i in 0..profiles {
            let admitted = io.admitted_name(i)?;
            let profile = io.profile_name(i)?;
            require(
                profile == admitted,
                "Couch profile differs from admitted player.",
            )?;
            io.append_profile(i, teams)?;
        }
        if !io.validate_map()? {
            io.reject_content()?;
            return Ok(false);
        }
        io.publish()?;
    }
    Ok(true)
}
impl Controller {
    fn edit<T>(&self, f: impl FnOnce(&mut State) -> T) -> T {
        f(&mut self
            .0
            .lock()
            .expect("Native frontend search state poisoned"))
    }
    pub fn state(&self) -> State {
        self.edit(|s| s.clone())
    }
    pub fn copy(&self) -> Self {
        Self(Mutex::new(self.state()))
    }
    pub fn scalar(&self, field: u32) -> Result<i64> {
        self.edit(|s| {
            Ok(match field {
                0 => s.active as i64,
                1 => s.quick as i64,
                2 => s.lobby_pending as i64,
                3 => i64::from(s.countdown),
                4 => s.have_fun as i64,
                5 => i64::from(s.pane),
                6 => i64::from(s.title),
                7 => i64::from(s.pane_clock),
                _ => return Err(Error::Policy("Invalid native frontend search field")),
            })
        })
    }
    pub fn launch(&self, io: &(impl Io + LaunchIo)) -> Result<bool> {
        let failed = match launch(io) {
            Ok(value) => return Ok(value),
            Err(Error::Policy(s)) => s.as_bytes().to_vec(),
            Err(Error::Io) => {
                let (format, bytes) = io.failure()?;
                if !format {
                    return Err(Error::Io);
                }
                bytes
            }
        };
        // This is the old try-scope unwind before its FormatError handler. Native
        // ownership is released before queue cancellation or reentrant UI work.
        io.discard()?;
        io.cancel()?;
        self.reset_progress(io.context()?.clock);
        io.ui(Ui::Unavailable(failed))?;
        Ok(false)
    }
    pub fn stop(&self) {
        self.edit(|s| {
            s.active = false;
            s.quick = false
        })
    }
    pub fn quick(&self, value: bool) {
        self.edit(|s| s.quick = value)
    }
    pub fn left(&self) {
        self.edit(|s| {
            s.active = false;
            s.quick = false;
            s.selected.clear();
            s.lobby_pending = false
        })
    }
    pub fn reset_progress(&self, clock: u32) {
        self.edit(|s| {
            s.consent = false;
            s.countdown = -1;
            s.have_fun = false;
            s.pane = 0;
            s.title = 0;
            s.pane_clock = clock;
            s.party.clear()
        })
    }
    pub fn client_reset(&self, io: &impl Io) -> Result<()> {
        self.edit(|s| {
            s.active = false;
            s.quick = false;
            s.lobby_pending = false;
            s.name.clear()
        });
        io.ui(Ui::ClearSnapshot)?;
        self.reset_progress(io.context()?.clock);
        Ok(())
    }
    pub fn prepare(&self, quick: bool, io: &impl Io) -> Result<()> {
        io.ui(Ui::ModeOnline)?;
        self.edit(|s| {
            s.quick = false;
            s.lobby_pending = quick || !s.selected.is_empty();
            s.create_submitted = false
        });
        io.ui(Ui::PopLobbyOrOnline)?;
        if io.context()?.top() != 14 {
            io.ui(Ui::OpenLobby)?
        }
        io.ui(Ui::EnterLobby)?;
        if !self.state().active {
            self.refresh(io)?
        }
        self.update(io)
    }
    pub fn cancel_and_return(&self, io: &impl Io) -> Result<()> {
        self.quick(false);
        if io.context()?.queue {
            io.cancel()?;
        }
        let mut context = io.context()?;
        self.reset_progress(context.clock);
        // TU1.5's confirm callback (0x25847E) marks cancellation before the
        // network requests. Trim to ONLINE or keep the root in a preview.
        while context.frames.len() > 1 && context.top() != 186 {
            io.ui(Ui::PopTop)?;
            context = io.context()?;
        }
        if context.mode == 3 {
            io.ui(Ui::OpenLobby)?;
            io.ui(Ui::EnterLobby)
        } else {
            io.ui(Ui::Revision)
        }
    }

    pub fn select(&self, index: u32, io: &impl Io) -> Result<()> {
        let view = io.view()?;
        if let Some(p) = view.playlists.get(index as usize) {
            if !p.eligible {
                io.ui(Ui::PlaylistError(
                    concat!(
                        "This playlist is unavailable for your ONLINE client, party, or installed maps. ",
                        "Use an updated ONLINE build and check the playlist's party and map requirements."
                    )
                    .as_bytes()
                    .to_vec(),
                ))?;
                return Ok(());
            }
            self.edit(|s| s.selected = p.id.clone());
            self.prepare(false, io)
        } else {
            self.refresh(io)
        }
    }
    pub fn opened(&self, search: bool, io: &impl Io) -> Result<()> {
        if search {
            self.quick(true)
        } else {
            let c = io.context()?;
            if c.account && c.party.is_none() && !io.account_snapshot()?.busy {
                io.command("create", "", "")?;
            }
        }
        if !self.state().active {
            self.refresh(io)?
        }
        if search {
            self.update(io)?
        }
        Ok(())
    }
    pub fn refresh(&self, io: &impl Io) -> Result<()> {
        if !io.identity()? {
            return Ok(());
        }
        self.reset_progress(io.context()?.clock);
        self.edit(|s| s.active = true);
        let name = io.profile()?;
        self.edit(|s| s.name = name.clone());
        let couch = io.context()?.couch_revision;
        self.edit(|s| s.couch_revision = couch);
        if io.context()?.queue {
            io.refresh(&name)?
        }
        self.update(io)
    }
    pub fn party_search(&self, io: &impl Io) -> Result<bool> {
        let c = io.context()?;
        let v = io.view()?;
        let shown = c.shown(210) || c.shown(14);
        let searching = matches!(
            v.status,
            Status::Searching | Status::Checking | Status::Awaiting
        );
        if searching && shown && c.party.as_ref().is_some_and(|p| p.members.len() > 1) {
            if !self.state().party.is_empty() {
                return Ok(false);
            }
            let p = c.party.as_ref().unwrap();
            self.edit(|s| s.party = p.members.iter().map(|m| m.handle.clone()).collect());
            if p.leader || c.mode != 3 || c.shown(210) {
                return Ok(false);
            }
            io.ui(Ui::PopLobby)?;
            io.ui(Ui::OpenSearch)?;
            self.quick(false);
            return Ok(true);
        }
        let searched = self.edit(|s| std::mem::take(&mut s.party));
        if searched.is_empty() {
            return Ok(false);
        }
        if v.status != Status::Ready || !shown || c.party.is_none() {
            return Ok(false);
        }
        let departed = searched.iter().any(|h| {
            !c.party
                .as_ref()
                .unwrap()
                .members
                .iter()
                .any(|m| m.handle == *h)
        });
        self.quick(false);
        if c.queue {
            io.cancel()?
        }
        self.reset_progress(io.context()?.clock);
        io.ui(Ui::PopLobbyOrOnline)?;
        if c.mode == 3 {
            if io.context()?.top() != 14 {
                io.ui(Ui::OpenLobby)?
            }
            io.ui(Ui::EnterLobby)?
        }
        io.ui(Ui::Dialog(if departed { 0xae } else { 0xad }))?;
        io.ui(Ui::Revision)?;
        Ok(true)
    }
    pub fn update(&self, io: &impl Io) -> Result<()> {
        let v = io.view()?;
        let s = self.state();
        let previous = pane(ui_state(&v, s.have_fun), s.countdown);
        let c = io.context()?;
        let retained = s.active && c.mode == 3 && c.party.is_some();
        if !(io.identity()? && (retained || c.frames.iter().any(|f| screen(f) || f.party_screen))) {
            if self.state().active && c.queue {
                io.sign_out()?
            }
            self.edit(|s| {
                s.active = false;
                s.quick = false;
                s.lobby_pending = false;
                s.name.clear()
            });
            io.ui(Ui::ClearSnapshot)?;
            self.reset_progress(io.context()?.clock);
            return Ok(());
        }
        if io.lobby_countdown()? {
            return Ok(());
        }
        let s = self.state();
        if s.active {
            let changed = s.name != io.profile()?;
            if changed || s.couch_revision != io.context()?.couch_revision {
                self.quick(io.context()?.top() == 210);
                self.refresh(io)?;
                return Ok(());
            }
        }
        let mut changed = false;
        let c = io.context()?;
        if c.queue {
            let revision = io.view()?.revision;
            changed = io.pull_after(revision)?
        } else if io.view()?.message_empty {
            io.ui(Ui::NoService(
                b"No matchmaking service configured.".to_vec(),
            ))?;
            changed = true
        }
        let v = io.view()?;
        let shown = io.context()?.shown(210);
        if let Some(failure) = v.failure.filter(|_| shown) {
            let error = crate::matchmaking_readiness::error_dialog(failure, false);
            io.ui(Ui::ClearFailure)?;
            self.quick(false);
            if io.context()?.queue {
                io.cancel()?
            }
            self.reset_progress(io.context()?.clock);
            io.ui(Ui::PopLobbyOrOnline)?;
            if io.context()?.top() != 14 {
                io.ui(Ui::OpenLobby)?
            }
            io.ui(Ui::EnterLobby)?;
            if let Some(error) = error {
                io.ui(Ui::Dialog(error))?
            }
            io.ui(Ui::Revision)?;
            return Ok(());
        }
        if self.state().lobby_pending && io.view()?.status == Status::Ready {
            if io.context()?.account {
                let p = io.account_snapshot()?;
                if p.party.is_none() && !p.busy {
                    if self.state().create_submitted {
                        self.edit(|s| s.lobby_pending = false);
                        io.ui(Ui::Failure(if p.message.is_empty() {
                            b"The party could not be created.".to_vec()
                        } else {
                            p.message
                        }))?;
                    } else if io.command("create", "", "")? {
                        self.edit(|s| s.create_submitted = true)
                    }
                }
            }
            if self.state().lobby_pending {
                let s = self.state();
                let p = choose(io, &io.view()?, &s.selected, false)?;
                if let Some(p) = p {
                    self.edit(|s| s.selected = p.id.clone());
                    let c = io.context()?;
                    if !c.account
                        || (c.party.is_some() && io.command("ready", &p.id, &p.revision)?)
                    {
                        self.edit(|s| s.lobby_pending = false)
                    }
                } else {
                    self.edit(|s| s.lobby_pending = false)
                }
            }
            io.ui(Ui::RepopulateLobby)?;
            changed = true;
        }
        if self.party_search(io)? {
            return Ok(());
        }
        if self.state().quick && io.view()?.status == Status::Ready {
            let c = io.context()?;
            let p = choose(io, &io.view()?, &self.state().selected, true)?;
            if p.as_ref().is_some_and(|p| p.eligible) && c.queue {
                let p = p.unwrap();
                self.edit(|s| s.selected = p.id.clone());
                if io.context()?.party.is_some() {
                    let account = if io.context()?.account {
                        io.account_snapshot()?
                    } else {
                        Account::default()
                    };
                    let consent = account.party.as_ref().is_some_and(|party| {
                        party.members.iter().any(|m| m.is_self && m.ready == p.id)
                    });
                    if !io.context()?.account || account.party.is_none() {
                        self.edit(|s| {
                            s.quick = false;
                            s.consent = false
                        });
                        io.ui(Ui::Failure(if account.message.is_empty() {
                            b"The accepted party is no longer available.".to_vec()
                        } else {
                            account.message
                        }))?;
                    } else if account.busy {
                    } else if consent {
                        self.edit(|s| {
                            s.quick = false;
                            s.consent = false
                        });
                        if account.party.unwrap().leader {
                            io.join(&p.id)?;
                            io.pull()?
                        }
                    } else if self.state().consent {
                        self.edit(|s| {
                            s.quick = false;
                            s.consent = false
                        });
                        io.ui(Ui::Failure(if account.message.is_empty() {
                            b"Party playlist selection was not accepted.".to_vec()
                        } else {
                            account.message
                        }))?;
                    } else if io.command("ready", &p.id, &p.revision)? {
                        self.edit(|s| s.consent = true)
                    }
                } else {
                    self.edit(|s| {
                        s.quick = false;
                        s.consent = false
                    });
                    io.join(&p.id)?;
                    io.pull()?
                }
            } else {
                self.edit(|s| {
                    s.quick = false;
                    s.consent = false
                });
                io.ui(Ui::Failure(if p.is_some() {
                    b"This playlist is unavailable for this party or installed maps.".to_vec()
                } else {
                    b"Choose an available playlist in Optimatch first.".to_vec()
                }))?;
            }
            changed = true;
        }
        let v = io.view()?;
        let start = v.status == Status::Loading && v.matched && io.context()?.queue;
        if start && !self.state().have_fun {
            let remaining = v
                .countdown_ms
                .filter(|n| v.countdown_supported && *n <= 10000);
            let (seconds, ready) = crate::matchmaking_frontend_progress::published_countdown(remaining);
            if seconds != self.state().countdown {
                self.edit(|s| s.countdown = seconds);
                changed = true
            }
            if ready {
                if !io.launch()? {
                    return Ok(());
                }
                if io.has_launch()? {
                    self.edit(|s| s.have_fun = true);
                    changed = true
                }
            }
        } else if !start && (self.state().countdown != -1 || self.state().have_fun) {
            self.edit(|s| {
                s.countdown = -1;
                s.have_fun = false
            });
            changed = true
        }
        let v = io.view()?;
        let s = self.state();
        let ui = ui_state(&v, s.have_fun);
        let next = pane(ui, s.countdown);
        if next != s.pane {
            let clock = io.context()?.clock;
            self.edit(|s| {
                s.title = ui;
                s.pane = next;
                s.pane_clock = clock
            });
            changed = true;
            loop {
                let c = io.context()?;
                let above = c
                    .frames
                    .iter()
                    .rposition(|f| f.screen == 210)
                    .is_some_and(|i| i + 1 < c.frames.len());
                if !above {
                    break;
                }
                let top = c.frames.last().unwrap();
                let leave = top.screen == 7 && top.first_destination == Some(-16);
                let y = matches!(top.screen, 26 | 31 | 216 | 217 | 211);
                if !((leave && next >= 2) || (y && next >= 1)) {
                    break;
                }
                io.ui(Ui::PopTop)?;
            }
            if next == 3 && io.context()?.top() == 210 {
                io.ui(Ui::Sound(12))?
            }
        }
        if changed {
            io.ui(Ui::Render(previous))?;
            io.ui(Ui::Revision)?
        }
        Ok(())
    }
}
