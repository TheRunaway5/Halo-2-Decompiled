use halo2_online_protocol::matchmaking_frontend_account_flow::*;
use halo2_online_protocol::matchmaking_frontend_party::{Membership, Notification};
use halo2_online_protocol::matchmaking_frontend_party_flow::{Controller, State};
use std::cell::RefCell;

fn account(revision: u64, authenticated: bool, party: bool) -> Account {
    Account {
        revision,
        notification: Notification {
            authenticated,
            party: party.then(Vec::new),
            message_empty: true,
            ..Notification::default()
        },
        ..Account::default()
    }
}
fn frame(screen: u32, context: u32) -> Frame {
    Frame {
        screen,
        context,
        ..Frame::default()
    }
}
fn row(label: &[u8], destination: i32, value: i32) -> Entry {
    Entry {
        label: label.to_vec(),
        destination,
        value,
    }
}
struct Native<'a> {
    flow: &'a Controller,
    f: RefCell<Facts>,
    next: Account,
    identity: Option<Vec<u8>>,
    voice_revision: u64,
    native: bool,
    rows: Vec<Entry>,
    calls: RefCell<Vec<String>>,
    effects: RefCell<Vec<Effect>>,
    profiles: RefCell<Vec<(Vec<u8>, Vec<u8>)>>,
    fail: Option<String>,
    partial_population: bool,
}
impl<'a> Native<'a> {
    fn new(flow: &'a Controller) -> Self {
        Self {
            flow,
            f: RefCell::new(Facts {
                account_client: true,
                account: account(1, true, false),
                frames: vec![frame(6, 0), frame(186, 0)],
                mode: 3,
                ..Facts::default()
            }),
            next: account(2, true, false),
            identity: None,
            voice_revision: 0,
            native: false,
            rows: vec![],
            calls: RefCell::new(vec![]),
            effects: RefCell::new(vec![]),
            profiles: RefCell::new(vec![]),
            fail: None,
            partial_population: false,
        }
    }
    fn call(&self, name: &str) -> Result<()> {
        let _reentry = self.flow.state();
        self.calls.borrow_mut().push(name.into());
        if self.fail.as_deref() == Some(name) {
            Err(name.into())
        } else {
            Ok(())
        }
    }
}
impl Io for Native<'_> {
    fn facts(&self) -> Result<Facts> {
        Ok(self.f.borrow().clone())
    }
    fn identity(&self) -> Result<Option<Vec<u8>>> {
        self.call("identity")?;
        Ok(self.identity.clone())
    }
    fn profile(&self, n: &[u8], t: &[u8]) -> Result<()> {
        self.call("profile")?;
        self.profiles.borrow_mut().push((n.to_vec(), t.to_vec()));
        Ok(())
    }
    fn connect(&self, n: &[u8]) -> Result<()> {
        self.call("connect")?;
        assert!(!n.is_empty());
        Ok(())
    }
    fn sign_out(&self) -> Result<()> {
        self.call("signout")
    }
    fn snapshot(&self) -> Result<Account> {
        self.call("snapshot")?;
        Ok(self.next.clone())
    }
    fn stage_voice(&self, read: bool) -> Result<u64> {
        if read {
            self.call("voice")?;
            Ok(self.voice_revision)
        } else {
            Ok(0)
        }
    }
    fn native_friends(&self) -> Result<bool> {
        self.call("native-friends")?;
        Ok(self.native)
    }
    fn effect(&self, e: Effect) -> Result<()> {
        self.call(match &e {
            Effect::VoiceLeave => "leave",
            Effect::VoiceCreateParty => "create",
            Effect::VoiceJoin => "join",
            Effect::VoiceRefresh => "refresh",
            Effect::Unavailable(_) => "unavailable",
            Effect::Populate(_, _) => "populate",
            Effect::CommitVoice => "commit-voice",
            _ => "effect",
        })?;
        self.effects.borrow_mut().push(e.clone());
        let mut f = self.f.borrow_mut();
        match e {
            Effect::Sound(_) => f.sounds += 1,
            Effect::CommitAccount => f.account = self.next.clone(),
            Effect::CommitVoice => f.voice_revision = self.voice_revision,
            Effect::ReturnOnline => {
                f.frames = vec![frame(6, 0), frame(186, 0)];
                f.launch = false;
            }
            Effect::Open(s, c) => f.frames.push(frame(s, c)),
            Effect::VoiceDrop => f.voice_client = false,
            Effect::VoiceCreateParty => f.voice_client = true,
            Effect::VoiceRefresh => f.voice_revision = 91,
            Effect::EraseFrom(i) => {
                f.frames.truncate(i);
            }
            Effect::ClearEntries(i) => f.frames[i].entries.clear(),
            Effect::Populate(i, _) => {
                f.frames[i].entries = self.rows.clone();
                if self.partial_population {
                    return Err("partial population".into());
                }
            }
            Effect::Select(i, n) => f.frames[i].selected = n,
            Effect::Unavailable(_) => f.frames.push(frame(8, 0)),
            _ => (),
        }
        Ok(())
    }
}
#[test]
fn absent_account_and_equal_revisions_leave_panels_untouched() {
    let c = Controller::default();
    c.set_account_name(b"old".to_vec());
    let mut io = Native::new(&c);
    io.f.borrow_mut().account_client = false;
    update(&c, &io).unwrap();
    assert!(io.calls.borrow().is_empty());
    assert_eq!(c.state().account_name, b"old");
    io.f.borrow_mut().account_client = true;
    io.next.revision = 1;
    update(&c, &io).unwrap();
    assert!(io.effects.borrow().is_empty());
    assert_eq!(*io.calls.borrow(), ["signout", "snapshot"]);
}
#[test]
fn identity_name_fallback_and_profile_tag_preserve_opaque_bytes() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    io.f.borrow_mut().login_client = true;
    io.identity = Some(vec![0xff, 0, 0x80]);
    update(&c, &io).unwrap();
    assert_eq!(c.state().account_name, [0xff, 0, 0x80]);
    assert!(io.profiles.borrow().is_empty());
    let name = vec![0xfe, 0];
    io.f.borrow_mut().signed_in = name.clone();
    io.f.borrow_mut().tags.push((name.clone(), vec![0x80, 0]));
    update(&c, &io).unwrap();
    assert_eq!(c.state().account_name, name);
    assert_eq!(io.profiles.borrow().last().unwrap().1, [0x80, 0]);
}
#[test]
fn account_name_changes_only_after_worker_call_returns() {
    let c = Controller::default();
    c.set_account_name(b"before".to_vec());
    let mut io = Native::new(&c);
    io.f.borrow_mut().login_client = true;
    io.identity = Some(b"after".to_vec());
    io.fail = Some("connect".into());
    assert_eq!(update(&c, &io).unwrap_err(), "connect");
    assert_eq!(c.state().account_name, b"before");
    io.identity = None;
    io.fail = Some("signout".into());
    assert!(update(&c, &io).is_err());
    assert_eq!(c.state().account_name, b"before");
}
#[test]
fn retained_party_on_initial_signin_does_not_redirect_online() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    io.f.borrow_mut().account.notification.authenticated = false;
    io.next = account(2, true, true);
    update(&c, &io).unwrap();
    assert_eq!(io.f.borrow().frames.last().unwrap().screen, 186);
    assert!(!io.effects.borrow().contains(&Effect::ReturnOnline));
}
#[test]
fn authoritative_new_membership_enters_lobby_only_when_allowed() {
    for guard in 0..4 {
        let c = Controller::default();
        let mut io = Native::new(&c);
        io.next = account(2, true, true);
        io.f.borrow_mut().queue_client = true;
        if guard == 1 {
            io.f.borrow_mut().launch = true;
        }
        if guard == 2 {
            io.f.borrow_mut().frames.last_mut().unwrap().pcr = true;
        }
        if guard == 3 {
            io.f.borrow_mut().frames.insert(0, frame(14, 0));
        }
        update(&c, &io).unwrap();
        assert_eq!(
            io.effects.borrow().contains(&Effect::ReturnOnline),
            guard == 0
        );
        assert_eq!(
            io.effects.borrow().contains(&Effect::CancelQueue),
            guard == 0
        );
    }
}
#[test]
fn departure_closes_from_first_lobby_and_keeps_group_identity() {
    let c = Controller::default();
    c.set_voice_party(true);
    c.set_voice_wanted(true);
    let io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.account = account(1, true, true);
        f.frames.extend([frame(14, 0), frame(185, 0)]);
        f.voice_client = true;
    }
    update(&c, &io).unwrap();
    assert!(c.state().voice_party);
    assert!(!c.state().voice_wanted);
    assert_eq!(
        io.f.borrow()
            .frames
            .iter()
            .map(|f| f.screen)
            .collect::<Vec<_>>(),
        [6, 186, 26]
    );
    assert!(io.effects.borrow().contains(&Effect::VoiceLeave));
}
#[test]
fn voice_group_is_published_after_creation_and_before_join() {
    let c = Controller::default();
    c.set_voice_wanted(true);
    let mut io = Native::new(&c);
    io.next = account(2, true, true);
    io.fail = Some("create".into());
    assert!(update(&c, &io).is_err());
    assert!(!c.state().voice_party);
    assert!(c.state().voice_wanted);
    io.fail = Some("join".into());
    // A committed revision is not processed again until the worker advances.
    assert!(update(&c, &io).is_ok());
    io.next.revision = 3;
    assert!(update(&c, &io).is_err());
    assert!(c.state().voice_party);
}
#[test]
fn notification_budget_retains_first_ordered_cue() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.sounds = 15;
        f.account = account(1, true, true);
        f.account.notification.busy = true;
        f.account.notification.party = Some(vec![Membership {
            handle: b"old".to_vec(),
            local_count: 1,
        }]);
    }
    io.next = account(2, true, true);
    io.next.notification.party = Some(vec![Membership {
        handle: b"new".to_vec(),
        local_count: 1,
    }]);
    io.next.notification.invitations = vec![b"invite".to_vec()];
    io.next.notification.message_empty = false;
    update(&c, &io).unwrap();
    assert_eq!(
        io.effects
            .borrow()
            .iter()
            .filter_map(|e| if let Effect::Sound(n) = e {
                Some(*n)
            } else {
                None
            })
            .collect::<Vec<_>>(),
        [4]
    );
    assert_eq!(io.f.borrow().sounds, 16);
}
#[test]
fn account_commit_and_invite_pruning_precede_voice_commit() {
    let c = Controller::from_state(State {
        dismissed: vec![b"old".to_vec(), vec![0xff, 0]],
        ..State::default()
    });
    let mut io = Native::new(&c);
    io.next.notification.invitations = vec![vec![0xff, 0]];
    io.fail = Some("commit-voice".into());
    assert!(update(&c, &io).is_err());
    assert_eq!(io.f.borrow().account.revision, 2);
    assert_eq!(c.state().dismissed, [vec![0xff, 0]]);
}
#[test]
fn friend_focus_uses_identity_and_destination_before_duplicate_labels() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    io.native = true;
    {
        let mut f = io.f.borrow_mut();
        f.account.friends = vec![vec![0xff, 0]];
        f.frames = vec![Frame {
            entries: vec![row(b"same", 31, 0)],
            ..frame(26, 0)
        }];
    }
    io.next.friends = vec![vec![0xff, 0], b"other".to_vec()];
    io.rows = vec![
        row(b"same", 31, 1),
        row(b"same", 12, 0),
        row(b"same", 31, 0),
    ];
    update(&c, &io).unwrap();
    assert_eq!(io.f.borrow().frames[0].selected, 2);
}
#[test]
fn clan_focus_preserves_raw_indices_and_does_not_require_friend_destination() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.account.clan = Some(vec![b"target".to_vec()]);
        f.frames = vec![Frame {
            entries: vec![row(b"same", -1, 0)],
            ..frame(26, 1)
        }];
    }
    io.next.clan = Some(vec![b"target".to_vec()]);
    io.rows = vec![row(b"same", 31, -1), row(b"same", -82, 0)];
    update(&c, &io).unwrap();
    assert_eq!(io.f.borrow().frames[0].selected, 1);
}
#[test]
fn party_focus_retains_position_and_new_search_resets_zero() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    io.f.borrow_mut().frames = vec![Frame {
        selected: 2,
        entries: vec![row(b"x", 31, 0), row(b"y", 31, 1), row(b"same", 31, 2)],
        ..frame(26, 2)
    }];
    io.rows = vec![row(b"same", 31, 1), row(b"z", 31, 0)];
    update(&c, &io).unwrap();
    assert_eq!(io.f.borrow().frames[0].selected, 1);
    io.f.borrow_mut().frames = vec![Frame {
        selected: 2,
        entries: io.rows.clone(),
        ..frame(31, 46)
    }];
    io.f.borrow_mut().account.friend_search = false;
    io.next.friend_search = true;
    io.next.revision = 3;
    io.native = true;
    update(&c, &io).unwrap();
    assert_eq!(io.f.borrow().frames[0].selected, 0);
}
#[test]
fn partial_population_error_keeps_clear_and_append_without_reselection() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    io.f.borrow_mut().frames = vec![Frame {
        selected: 99,
        entries: vec![row(b"old", 31, 0)],
        ..frame(26, 2)
    }];
    io.rows = vec![row(b"partial", 31, 0)];
    io.partial_population = true;
    assert_eq!(update(&c, &io).unwrap_err(), "partial population");
    let f = io.f.borrow();
    assert_eq!(f.frames[0].selected, 99);
    assert_eq!(f.frames[0].entries[0].label, b"partial");
}
#[test]
fn friend_join_serial_is_full_width_and_cleared_before_warning() {
    let c = Controller::default();
    c.set_friend_join_serial(u64::MAX);
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frames = vec![frame(31, 43)];
        f.account.command_serial = u64::MAX;
        f.account.successful = Some(false);
        f.account.party_message = vec![0xff, 0];
    }
    io.fail = Some("unavailable".into());
    assert!(friend_join(&c, &io).is_err());
    assert_eq!(c.state().friend_join_serial, 0);
    c.set_friend_join_serial(u64::MAX);
    io.fail = None;
    io.f.borrow_mut().account.command_serial = u64::MAX - 1;
    friend_join(&c, &io).unwrap();
    assert_eq!(c.state().friend_join_serial, 0);
}
#[test]
fn pending_join_is_retained_and_success_requires_actual_membership() {
    let c = Controller::default();
    c.set_friend_join_serial(7);
    let io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frames = vec![frame(31, 43)];
        f.account.command_serial = 7;
        f.account.notification.busy = true;
    }
    friend_join(&c, &io).unwrap();
    assert_eq!(c.state().friend_join_serial, 7);
    {
        let mut f = io.f.borrow_mut();
        f.account.notification.busy = false;
        f.account.successful = Some(true);
        f.account.notification.party = Some(vec![]);
    }
    friend_join(&c, &io).unwrap();
    assert_eq!(c.state().friend_join_serial, 0);
    assert!(io.effects.borrow().is_empty());
}
#[test]
fn copied_state_is_independent_and_voice_match_uses_intent_group_and_join() {
    for bits in 0..16 {
        let c = Controller::default();
        c.set_voice_wanted(bits & 1 != 0);
        c.set_voice_party(bits & 2 != 0);
        c.set_friend_join_serial(u64::MAX);
        c.set_account_name(vec![0xff, 0]);
        let copied = Controller::from_state(c.state());
        copied.set_voice_wanted(false);
        copied.set_account_name(vec![]);
        assert_eq!(c.state().account_name, [0xff, 0]);
        assert_eq!(c.state().friend_join_serial, u64::MAX);
        assert_eq!(
            c.match_voice(bits & 4 != 0, bits & 8 != 0),
            bits & 4 != 0 || (bits & 1 != 0 && bits & 2 != 0 && bits & 8 != 0)
        );
    }
}
