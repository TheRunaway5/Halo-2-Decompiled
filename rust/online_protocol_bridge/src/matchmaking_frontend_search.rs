use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_search as core;
pub struct FrontendSearch(core::Controller);
fn io<T>(value: Result<T, cxx::Exception>) -> core::Result<T> {
    value.map_err(|_| core::Error::Io)
}
fn party(present: bool, leader: bool, members: Vec<ffi::SearchMember>) -> Option<core::Party> {
    present.then(|| core::Party {
        leader,
        members: members
            .into_iter()
            .map(|m| core::Member {
                handle: m.handle,
                is_self: m.is_self,
                ready: m.ready,
            })
            .collect(),
    })
}
fn snapshot(v: ffi::SearchSnapshot) -> core::Result<core::Snapshot> {
    let status = match v.status {
        0 => core::Status::Unconfigured,
        1 => core::Status::Idle,
        2 => core::Status::Connecting,
        3 => core::Status::Ready,
        4 => core::Status::Joining,
        5 => core::Status::Searching,
        6 => core::Status::Checking,
        7 => core::Status::Awaiting,
        8 => core::Status::Loading,
        9 => core::Status::Failed,
        _ => return Err(core::Error::Policy("Invalid native frontend search ABI")),
    };
    Ok(core::Snapshot {
        status,
        playlists: v
            .playlists
            .into_iter()
            .map(|p| core::Playlist {
                id: p.id,
                revision: p.revision,
                eligible: p.eligible,
                quickmatch: p.quickmatch,
                clan_match: p.clan_match,
            })
            .collect(),
        matched: v.matched,
        gathering: v.gathering,
        revision: v.revision,
        message_empty: v.message_empty,
        countdown_supported: v.countdown_supported,
        countdown_ms: v.countdown_present.then_some(v.countdown_ms),
        failure: v.failure_present.then_some(v.failure),
    })
}
struct Io<'a>(&'a ffi::NativeFrontendSearchIo);
impl core::Io for Io<'_> {
    fn context(&self) -> core::Result<core::Context> {
        let c = io(self.0.search_context())?;
        Ok(core::Context {
            mode: c.mode,
            clock: c.clock,
            couch_revision: c.couch_revision,
            queue: c.queue,
            account: c.account,
            party: party(c.party_present, c.leader, c.members),
            frames: c
                .frames
                .into_iter()
                .map(|f| core::Frame {
                    screen: f.screen,
                    context: f.context,
                    party_screen: f.party_screen,
                    first_destination: f.first_present.then_some(f.first_destination),
                })
                .collect(),
        })
    }
    fn view(&self) -> core::Result<core::Snapshot> {
        snapshot(io(self.0.search_view())?)
    }
    fn identity(&self) -> core::Result<bool> {
        io(self.0.search_identity())
    }
    fn profile(&self) -> core::Result<Vec<u8>> {
        io(self.0.search_profile())
    }
    fn local_count(&self) -> core::Result<u32> {
        io(self.0.search_local_count())
    }
    fn pull_after(&self, v: u64) -> core::Result<bool> {
        io(self.0.search_pull_after(v))
    }
    fn pull(&self) -> core::Result<()> {
        io(self.0.search_pull())
    }
    fn account_snapshot(&self) -> core::Result<core::Account> {
        let p = io(self.0.search_account())?;
        Ok(core::Account {
            party: party(p.present, p.leader, p.members),
            busy: p.busy,
            message: p.message,
        })
    }
    fn command(&self, a: &str, p: &str, r: &str) -> core::Result<bool> {
        io(self.0.search_command(a, p, r))
    }
    fn sign_out(&self) -> core::Result<()> {
        io(self.0.search_sign_out())
    }
    fn refresh(&self, n: &[u8]) -> core::Result<()> {
        io(self.0.search_refresh(n))
    }
    fn cancel(&self) -> core::Result<()> {
        io(self.0.search_cancel())
    }
    fn join(&self, p: &str) -> core::Result<()> {
        io(self.0.search_join(p))
    }
    fn lobby_countdown(&self) -> core::Result<bool> {
        io(self.0.search_lobby_countdown())
    }
    fn launch(&self) -> core::Result<bool> {
        io(self.0.search_launch())
    }
    fn has_launch(&self) -> core::Result<bool> {
        io(self.0.search_has_launch())
    }
    fn ui(&self, e: core::Ui) -> core::Result<()> {
        use core::Ui as U;
        use ffi::SearchUiKind as K;
        let (kind, number, text) = match e {
            U::ClearSnapshot => (K::ClearSnapshot, 0, vec![]),
            U::NoService => (K::NoService, 0, vec![]),
            U::Failure(v) => (K::Failure, 0, v),
            U::ClearFailure => (K::ClearFailure, 0, vec![]),
            U::ModeOnline => (K::ModeOnline, 0, vec![]),
            U::PopLobbyOrOnline => (K::PopLobbyOrOnline, 0, vec![]),
            U::PopLobby => (K::PopLobby, 0, vec![]),
            U::OpenLobby => (K::OpenLobby, 0, vec![]),
            U::EnterLobby => (K::EnterLobby, 0, vec![]),
            U::OpenSearch => (K::OpenSearch, 0, vec![]),
            U::Dialog(n) => (K::Dialog, n, vec![]),
            U::Revision => (K::Revision, 0, vec![]),
            U::RepopulateLobby => (K::RepopulateLobby, 0, vec![]),
            U::PopTop => (K::PopTop, 0, vec![]),
            U::Sound(n) => (K::Sound, n, vec![]),
            U::Render(n) => (K::Render, n, vec![]),
            U::Unavailable(v) => (K::Unavailable, 0, v),
            U::PlaylistError => (K::PlaylistError, 0, vec![]),
        };
        io(self.0.search_ui(&ffi::SearchUi { kind, number, text }))
    }
}
impl core::LaunchIo for Io<'_> {
    fn variant(&self) -> core::Result<Option<bool>> {
        let v = io(self.0.search_variant())?;
        Ok(v.present.then_some(v.teams))
    }
    fn map(&self) -> core::Result<Vec<u8>> {
        io(self.0.search_map())
    }
    fn prepare_map(&self, m: &[u8]) -> core::Result<()> {
        io(self.0.search_prepare_map(m))
    }
    fn self_present(&self) -> core::Result<bool> {
        io(self.0.search_self_present())
    }
    fn take(&self) -> core::Result<core::Take> {
        let t = io(self.0.search_take())?;
        Ok(core::Take {
            present: t.present,
            team_mode: t.teams_present.then_some(t.teams),
        })
    }
    fn local_counts(&self) -> core::Result<(u32, u32)> {
        let c = io(self.0.search_local_counts())?;
        Ok((c.profiles, c.slots))
    }
    fn local_count(&self, n: u32) -> core::Result<()> {
        io(self.0.search_set_local_count(n))
    }
    fn admitted_name(&self, i: u32) -> core::Result<Vec<u8>> {
        io(self.0.search_admitted_name(i))
    }
    fn profile_name(&self, i: u32) -> core::Result<Vec<u8>> {
        io(self.0.search_profile_name(i))
    }
    fn append_profile(&self, i: u32, t: bool) -> core::Result<()> {
        io(self.0.search_append_profile(i, t))
    }
    fn validate_map(&self) -> core::Result<bool> {
        io(self.0.search_validate_map())
    }
    fn reject_content(&self) -> core::Result<()> {
        io(self.0.search_reject_content())
    }
    fn publish(&self) -> core::Result<()> {
        io(self.0.search_publish())
    }
    fn discard(&self) -> core::Result<()> {
        io(self.0.search_discard())
    }
    fn failure(&self) -> core::Result<(bool, Vec<u8>)> {
        let f = io(self.0.search_failure())?;
        Ok((f.format, f.text))
    }
}
fn protect<T>(f: impl FnOnce() -> core::Result<T>) -> Result<T, String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(f))
        .map_err(|_| "Invalid native frontend search state".to_string())?
        .map_err(|e| e.to_string())
}
pub fn new_frontend_search() -> Box<FrontendSearch> {
    Box::new(FrontendSearch(core::Controller::default()))
}
pub fn clone_frontend_search(c: &FrontendSearch) -> Box<FrontendSearch> {
    Box::new(FrontendSearch(c.0.copy()))
}
pub fn frontend_search_scalar(c: &FrontendSearch, field: u32) -> Result<i64, String> {
    protect(|| c.0.scalar(field))
}
pub fn frontend_search_selected(c: &FrontendSearch) -> String {
    c.0.state().selected
}
pub fn frontend_search_stop(c: &FrontendSearch) {
    c.0.stop()
}
pub fn frontend_search_quick(c: &FrontendSearch, value: bool) {
    c.0.quick(value)
}
pub fn frontend_search_left(c: &FrontendSearch) {
    c.0.left()
}
pub fn frontend_search_reset(c: &FrontendSearch, clock: u32) {
    c.0.reset_progress(clock)
}
pub fn frontend_search_run(
    c: &FrontendSearch,
    operation: u32,
    index: u32,
    native: &ffi::NativeFrontendSearchIo,
) -> Result<bool, String> {
    protect(|| {
        let io = Io(native);
        match operation {
            0 => c.0.client_reset(&io)?,
            1 => c.0.prepare(false, &io)?,
            2 => c.0.prepare(true, &io)?,
            3 => c.0.refresh(&io)?,
            4 => c.0.update(&io)?,
            5 => return c.0.party_search(&io),
            6 => c.0.opened(false, &io)?,
            7 => c.0.opened(true, &io)?,
            8 => c.0.select(index, &io)?,
            9 => return c.0.launch(&io),
            _ => {
                return Err(core::Error::Policy(
                    "Invalid native frontend search operation",
                ));
            }
        }
        Ok(true)
    })
}
pub fn frontend_search_ui(c: &FrontendSearch, v: ffi::SearchSnapshot) -> Result<i32, String> {
    protect(|| Ok(core::ui_state(&snapshot(v)?, c.0.state().have_fun)))
}
pub fn frontend_search_pane(c: &FrontendSearch, v: ffi::SearchSnapshot) -> Result<u32, String> {
    protect(|| {
        let s = c.0.state();
        Ok(core::pane(
            core::ui_state(&snapshot(v)?, s.have_fun),
            s.countdown,
        ))
    })
}
pub fn frontend_search_state(c: &FrontendSearch) -> ffi::SearchState {
    let s = c.0.state();
    ffi::SearchState {
        active: s.active,
        quick: s.quick,
        consent: s.consent,
        selected: s.selected,
        name: s.name,
        couch_revision: s.couch_revision,
        lobby_pending: s.lobby_pending,
        create_submitted: s.create_submitted,
        countdown: s.countdown,
        have_fun: s.have_fun,
        pane: s.pane,
        title: s.title,
        pane_clock: s.pane_clock,
        party: s.party,
    }
}
pub fn frontend_search_variant(
    name: &[u8],
    present: bool,
    raw: &[u8],
    content: &[ffi::SearchName],
    bundled: &[ffi::SearchName],
) -> ffi::SearchChoice {
    let c = core::variant_choice(
        name,
        present.then_some(raw),
        &content.iter().map(|n| n.bytes.clone()).collect::<Vec<_>>(),
        &bundled.iter().map(|n| n.bytes.clone()).collect::<Vec<_>>(),
    );
    ffi::SearchChoice {
        kind: c.kind,
        index: c.index,
    }
}
