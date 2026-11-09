use halo2_online_protocol::matchmaking_frontend_account_flow::Entry;
use halo2_online_protocol::{
    matchmaking_frontend_party_flow::{Controller, State as PartyState},
    matchmaking_frontend_signin::*,
};
use std::cell::{Cell, RefCell};
struct Device<'a> {
    c: &'a Controller,
    f: RefCell<Facts>,
    snapshot: Cell<Snapshot>,
    identity: Cell<bool>,
    steam: Cell<bool>,
    authorized: Cell<bool>,
    calls: RefCell<Vec<String>>,
    fail: Cell<Option<usize>>,
    random: RefCell<Vec<u8>>,
    built: RefCell<Frame>,
    random_fail: Cell<bool>,
    reenter: RefCell<Option<Effect>>,
}
fn frame(s: u32) -> Frame {
    Frame {
        screen: s,
        description: b"old".to_vec(),
        entries: vec![Entry {
            label: b"old".to_vec(),
            destination: 0,
            value: 0,
        }],
        ..Frame::default()
    }
}
impl<'a> Device<'a> {
    fn new(c: &'a Controller) -> Self {
        Self {
            c,
            f: RefCell::new(Facts {
                login: true,
                account: true,
                queue: true,
                voice: true,
                social: true,
                directory: true,
                mode: 3,
                signed_in: b"host".to_vec(),
                frames: vec![frame(6), frame(20)],
                ..Facts::default()
            }),
            snapshot: Cell::new(Snapshot {
                status: 3,
                identity: false,
            }),
            identity: Cell::new(false),
            steam: Cell::new(false),
            authorized: Cell::new(true),
            calls: RefCell::new(vec![]),
            fail: Cell::new(None),
            random: RefCell::new((0..16).collect()),
            built: RefCell::new(frame(20)),
            random_fail: Cell::new(false),
            reenter: RefCell::new(None),
        }
    }
    fn observe(&self, name: String) -> Result<()> {
        let _ = self.c.state();
        let n = self.calls.borrow().len();
        self.calls.borrow_mut().push(name);
        if self.fail.get() == Some(n) {
            return Err("device fault".into());
        }
        Ok(())
    }
}
impl Io for Device<'_> {
    fn facts(&self) -> Result<Facts> {
        Ok(self.f.borrow().clone())
    }
    fn random(&self) -> Result<Vec<u8>> {
        self.observe("random".into())?;
        if self.random_fail.get() {
            Err("random fault".into())
        } else {
            Ok(self.random.borrow().clone())
        }
    }
    fn steam(&self) -> Result<bool> {
        self.observe("steam".into())?;
        Ok(self.steam.get())
    }
    fn authorized(&self) -> Result<bool> {
        self.observe("authorized".into())?;
        Ok(self.authorized.get())
    }
    fn snapshot(&self) -> Result<Snapshot> {
        self.observe("snapshot".into())?;
        Ok(self.snapshot.get())
    }
    fn identity(&self) -> Result<bool> {
        self.observe("identity".into())?;
        Ok(self.identity.get())
    }
    fn profile_name(&self) -> Result<Vec<u8>> {
        self.observe("profile".into())?;
        Ok(b"renamed".to_vec())
    }
    fn build(&self, s: u32, c: u32) -> Result<Frame> {
        self.observe(format!("build:{s}:{c}"))?;
        Ok(self.built.borrow().clone())
    }
    fn effect(&self, e: Effect) -> Result<()> {
        self.observe(format!("{e:?}"))?;
        if matches!(e, Effect::Restore) {
            if let Some(Effect::Open(s, c)) = self.reenter.borrow_mut().take() {
                prepare_open(self.c, self, s, c)?;
            }
        }
        let mut f = self.f.borrow_mut();
        match e {
            Effect::ClearNavigation => {
                f.frames.clear();
                f.mode = 3;
            }
            Effect::Open(s, c) => {
                let mut v = frame(s);
                v.context = c;
                f.frames.push(v);
            }
            Effect::Pop => {
                f.frames.pop();
            }
            Effect::KeyboardProfile => f.keyboard = b"host".to_vec(),
            Effect::KeyboardTag => f.keyboard = b"TAG".to_vec(),
            Effect::CommitBuilt {
                index,
                instance,
                pane_clock,
                selected,
            } => {
                let mut b = self.built.borrow().clone();
                b.instance = instance;
                b.pane_clock = pane_clock;
                b.selected = selected;
                f.frames[index as usize] = b;
            }
            _ => {}
        }
        Ok(())
    }
}
fn awaiting() -> Controller {
    let mut s = PartyState::default();
    s.signin.awaiting = true;
    s.signin.pending_screen = 14;
    Controller::from_state(s)
}
#[test]
fn empty_name_does_not_request_entropy() {
    let c = Controller::default();
    let io = Device::new(&c);
    assert!(key(&c, &io, b"").unwrap().is_empty());
    assert!(io.calls.borrow().is_empty());
}
#[test]
fn profile_key_uses_exact_sixteen_bytes_lowercase_hex() {
    let c = Controller::default();
    let io = Device::new(&c);
    assert_eq!(
        key(&c, &io, b"a").unwrap(),
        b"000102030405060708090a0b0c0d0e0f"
    );
    assert_eq!(&*io.calls.borrow(), &["random", "Save"]);
}
#[test]
fn cached_key_never_calls_entropy_or_save() {
    let c = Controller::default();
    load_key(&c, b"a".to_vec(), vec![b'f'; 32]);
    let io = Device::new(&c);
    assert_eq!(key(&c, &io, b"a").unwrap(), vec![b'f'; 32]);
    assert!(io.calls.borrow().is_empty());
}
#[test]
fn malformed_entropy_cannot_create_a_namespace() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.random.borrow_mut().truncate(15);
    assert!(key(&c, &io, b"a").is_err());
    assert!(c.signin_state().keys.is_empty());
}
#[test]
fn entropy_failure_leaves_no_namespace() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.random_fail.set(true);
    assert_eq!(key(&c, &io, b"a").unwrap_err(), "random fault");
    assert!(c.signin_state().keys.is_empty());
}
#[test]
fn save_failure_preserves_already_inserted_key() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.fail.set(Some(1));
    assert!(key(&c, &io, b"a").is_err());
    assert_eq!(c.signin_state().keys.len(), 1);
}
#[test]
fn saved_keys_validate_length_and_alphabet() {
    let c = Controller::default();
    for k in [
        vec![b'a'; 31],
        vec![b'a'; 33],
        vec![b'A'; 32],
        vec![0; 32],
        vec![255; 32],
    ] {
        load_key(&c, b"a".to_vec(), k);
    }
    assert!(c.signin_state().keys.is_empty());
    load_key(&c, b"a".to_vec(), vec![b'9'; 32]);
    assert_eq!(c.signin_state().keys.len(), 1);
}
#[test]
fn namespace_order_is_unsigned_bytes_and_preserves_empty_names() {
    let c = Controller::default();
    for n in [vec![255], vec![0], vec![128], vec![]] {
        load_key(&c, n, vec![b'a'; 32]);
    }
    assert_eq!(
        c.signin_state().keys.keys().cloned().collect::<Vec<_>>(),
        vec![vec![], vec![0], vec![128], vec![255]]
    );
}
#[test]
fn rename_moves_opaque_key_and_reads_destination_once() {
    let c = Controller::default();
    load_key(&c, b"a".to_vec(), vec![b'b'; 32]);
    let key = take_key(&c, b"a");
    assert!(c.signin_state().keys.is_empty());
    let io = Device::new(&c);
    restore_key(&c, &io, key).unwrap();
    assert_eq!(&*io.calls.borrow(), &["profile"]);
    assert_eq!(c.signin_state().keys[b"renamed".as_slice()], vec![b'b'; 32]);
}
#[test]
fn missing_rename_does_not_read_profile() {
    let c = Controller::default();
    let io = Device::new(&c);
    restore_key(&c, &io, take_key(&c, b"missing")).unwrap();
    assert!(io.calls.borrow().is_empty());
}
#[test]
fn clone_keeps_independent_signin_state() {
    let c = Controller::default();
    load_key(&c, b"a".to_vec(), vec![b'a'; 32]);
    let other = Controller::from_state(c.state());
    take_key(&c, b"a");
    assert_eq!(other.signin_state().keys.len(), 1);
}
#[test]
fn non_steam_bind_preserves_existing_account() {
    let c = Controller::default();
    let io = Device::new(&c);
    bind(&c, &io).unwrap();
    assert_eq!(&*io.calls.borrow(), &["steam"]);
}
#[test]
fn bind_empty_profile_is_noop_after_provider_observation() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.steam.set(true);
    io.f.borrow_mut().signed_in.clear();
    bind(&c, &io).unwrap();
    assert_eq!(&*io.calls.borrow(), &["steam"]);
}
#[test]
fn bind_orders_reset_after_provider_and_preserves_failure_prefix() {
    for fail in 0..10 {
        let c = Controller::default();
        let io = Device::new(&c);
        io.steam.set(true);
        io.fail.set(Some(fail));
        let r = bind(&c, &io);
        assert!(r.is_err());
        assert_eq!(io.calls.borrow().len(), fail + 1);
        let bound = c.signin_state().bound;
        if fail <= 3 {
            assert!(bound.is_empty());
        } else {
            assert!(!bound.is_empty());
        }
    }
}
#[test]
fn bind_calls_all_reset_owners_in_order() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.steam.set(true);
    bind(&c, &io).unwrap();
    let calls = io.calls.borrow();
    assert_eq!(
        &calls[4..],
        &[
            "ResetPrior",
            "SignOutAccount",
            "ClearAccount",
            "SignOutQueue",
            "StopSearch",
            "LeaveVoice",
            "DisconnectSocial"
        ]
    );
    assert_eq!(c.signin_state().bound.len(), 32);
}
#[test]
fn open_legacy_password_redirects_to_browser_and_default_target() {
    let c = Controller::default();
    let io = Device::new(&c);
    assert_eq!(prepare_open(&c, &io, 21, 99).unwrap(), (20, 99));
    assert_eq!(c.signin_state().pending_screen, 186);
}
#[test]
fn open_online_guard_preserves_pending_target_and_context() {
    let c = Controller::default();
    let io = Device::new(&c);
    assert_eq!(prepare_open(&c, &io, 14, u32::MAX).unwrap(), (20, 0));
    assert_eq!(
        (
            c.signin_state().pending_screen,
            c.signin_state().pending_context
        ),
        (14, u32::MAX)
    );
}
#[test]
fn offline_lobby_does_not_require_identity() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.f.borrow_mut().mode = 2;
    assert_eq!(prepare_open(&c, &io, 14, 2).unwrap(), (14, 2));
    assert_eq!(&*io.calls.borrow(), &["steam"]);
}
#[test]
fn restore_marks_awaiting_only_after_success() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.steam.set(true);
    io.f.borrow_mut().signed_in.clear();
    io.authorized.set(false);
    prepare_open(&c, &io, 20, 0).unwrap();
    assert!(c.signin_state().awaiting);
}
#[test]
fn login_update_preserves_six_owner_order_when_provider_absent() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.f.borrow_mut().login = false;
    update(&c, &io).unwrap();
    assert_eq!(
        &*io.calls.borrow(),
        &[
            "UpdateCouch",
            "UpdateFriends",
            "UpdateAccount",
            "UpdatePrivacy",
            "UpdateLeave",
            "UpdatePostgame"
        ]
    );
}
#[test]
fn successful_login_consumes_pending_navigation_before_open() {
    let c = awaiting();
    let io = Device::new(&c);
    io.identity.set(true);
    update(&c, &io).unwrap();
    assert!(!c.signin_state().awaiting);
    assert_eq!(io.f.borrow().frames.last().unwrap().screen, 14);
    let calls = io.calls.borrow();
    assert_eq!(&calls[calls.len() - 2..], &["Pop", "Open(14, 0)"]);
}
#[test]
fn failed_login_clears_awaiting_without_redirect() {
    let c = awaiting();
    let io = Device::new(&c);
    io.snapshot.set(Snapshot {
        status: 4,
        identity: false,
    });
    update(&c, &io).unwrap();
    assert!(!c.signin_state().awaiting);
    assert!(!io.calls.borrow().iter().any(|s| s.starts_with("Open")));
}
#[test]
fn unchanged_panel_does_not_replace_native_frame() {
    let c = Controller::default();
    let io = Device::new(&c);
    update(&c, &io).unwrap();
    assert!(io.calls.borrow().iter().any(|s| s == "DiscardBuilt"));
    assert!(!io.calls.borrow().iter().any(|s| s == "Revision"));
}
#[test]
fn changed_panel_preserves_native_identity_clock_and_clamps_selection() {
    let c = Controller::default();
    let io = Device::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frames[1].instance = u64::MAX;
        f.frames[1].pane_clock = u32::MAX;
        f.frames[1].selected = 99;
    }
    io.built.borrow_mut().description = b"new".to_vec();
    update(&c, &io).unwrap();
    let f = io.f.borrow();
    assert_eq!(f.frames[1].instance, u64::MAX);
    assert_eq!(f.frames[1].pane_clock, u32::MAX);
    assert_eq!(f.frames[1].selected, 0);
}
#[test]
fn back_clears_wait_after_cancellation() {
    let c = awaiting();
    let io = Device::new(&c);
    back(&c, &io).unwrap();
    assert_eq!(&*io.calls.borrow(), &["Cancel"]);
    assert!(!c.signin_state().awaiting);
}
#[test]
fn cancellation_failure_keeps_wait_flag() {
    let c = awaiting();
    let io = Device::new(&c);
    io.fail.set(Some(0));
    assert!(back(&c, &io).is_err());
    assert!(c.signin_state().awaiting);
}
#[test]
fn waiting_provider_does_not_start_another_login() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.snapshot.set(Snapshot {
        status: 1,
        identity: false,
    });
    assert!(accept(&c, &io).unwrap());
    assert_eq!(&*io.calls.borrow(), &["identity", "snapshot", "snapshot"]);
}
#[test]
fn keyboard_profile_is_observed_once() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.identity.set(true);
    io.f.borrow_mut().frames.last_mut().unwrap().selected = 1;
    accept(&c, &io).unwrap();
    assert_eq!(
        &*io.calls.borrow(),
        &["identity", "KeyboardProfile", "Open(167, 1)"]
    );
}
#[test]
fn signout_suppresses_restore_before_native_login_signout() {
    let c = Controller::default();
    let io = Device::new(&c);
    io.identity.set(true);
    {
        let mut f = io.f.borrow_mut();
        let top = f.frames.last_mut().unwrap();
        top.selected = 2;
        top.entries.resize(
            3,
            Entry {
                label: vec![],
                destination: 0,
                value: 0,
            },
        );
    }
    accept(&c, &io).unwrap();
    assert!(c.signin_state().suppressed);
    assert!(io.calls.borrow().iter().any(|s| s == "SignOutLogin"));
}
#[test]
fn return_to_lobby_cancels_previous_queue_before_new_navigation() {
    let c = Controller::default();
    let io = Device::new(&c);
    return_lobby(&c, &io, b"done").unwrap();
    assert_eq!(
        &io.calls.borrow()[..3],
        &["CancelQueue", "QuickOff", "ResetProgress"]
    );
    assert_eq!(io.f.borrow().mode, 3);
    assert!(c.signin_state().awaiting);
    assert!(io.calls.borrow().iter().any(|s| s == "EnterLobby"));
}
#[test]
fn screen_classification_preserves_exact_native_set() {
    let expected = [
        21, 26, 27, 31, 34, 35, 36, 182, 186, 210, 211, 215, 216, 217, 219, 236, 237,
    ];
    for s in 0..300 {
        assert_eq!(online_screen(s), expected.contains(&s));
    }
    assert!(!online_screen(u32::MAX));
}
