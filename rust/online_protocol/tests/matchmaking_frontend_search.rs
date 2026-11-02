use halo2_online_protocol::matchmaking_frontend_search::*;
use std::cell::{Cell, RefCell};
struct Native<'a> {
    owner: &'a Controller,
    context: RefCell<Context>,
    view: RefCell<Snapshot>,
    account: RefCell<Account>,
    calls: RefCell<Vec<String>>,
    profile: RefCell<Vec<u8>>,
    launch_case: Cell<u8>,
    accepted: bool,
    launch: bool,
}
impl<'a> Native<'a> {
    fn new(owner: &'a Controller) -> Self {
        Self {
            owner,
            context: RefCell::new(Context {
                mode: 3,
                clock: 5,
                queue: true,
                account: true,
                frames: vec![Frame {
                    screen: 210,
                    ..Frame::default()
                }],
                ..Context::default()
            }),
            view: RefCell::new(Snapshot {
                status: Status::Ready,
                message_empty: true,
                playlists: vec![Playlist {
                    id: "p".into(),
                    revision: "r".into(),
                    eligible: true,
                    quickmatch: true,
                    clan_match: false,
                }],
                ..Snapshot::default()
            }),
            account: RefCell::new(Account::default()),
            calls: RefCell::new(vec![]),
            profile: RefCell::new(b"Pilot".to_vec()),
            launch_case: Cell::new(0),
            accepted: true,
            launch: false,
        }
    }
    fn call(&self, s: &str) {
        let _ = self.owner.state();
        self.calls.borrow_mut().push(s.into());
    }
}
impl Io for Native<'_> {
    fn context(&self) -> Result<Context> {
        let _ = self.owner.state();
        Ok(self.context.borrow().clone())
    }
    fn view(&self) -> Result<Snapshot> {
        let _ = self.owner.state();
        Ok(self.view.borrow().clone())
    }
    fn identity(&self) -> Result<bool> {
        self.call("identity");
        Ok(true)
    }
    fn profile(&self) -> Result<Vec<u8>> {
        self.call("profile");
        Ok(self.profile.borrow().clone())
    }
    fn local_count(&self) -> Result<u32> {
        self.call("locals");
        Ok(1)
    }
    fn pull_after(&self, _: u64) -> Result<bool> {
        self.call("pull_after");
        Ok(false)
    }
    fn pull(&self) -> Result<()> {
        self.call("pull");
        Ok(())
    }
    fn account_snapshot(&self) -> Result<Account> {
        self.call("account");
        Ok(self.account.borrow().clone())
    }
    fn command(&self, action: &str, _: &str, _: &str) -> Result<bool> {
        self.call(action);
        Ok(self.accepted)
    }
    fn sign_out(&self) -> Result<()> {
        self.call("sign_out");
        Ok(())
    }
    fn refresh(&self, _: &[u8]) -> Result<()> {
        self.call("refresh");
        Ok(())
    }
    fn cancel(&self) -> Result<()> {
        self.call("cancel");
        Ok(())
    }
    fn join(&self, _: &str) -> Result<()> {
        self.call("join");
        self.view.borrow_mut().status = Status::Searching;
        Ok(())
    }
    fn lobby_countdown(&self) -> Result<bool> {
        self.call("lobby");
        Ok(false)
    }
    fn launch(&self) -> Result<bool> {
        self.call("launch");
        Ok(true)
    }
    fn has_launch(&self) -> Result<bool> {
        Ok(self.launch)
    }
    fn ui(&self, e: Ui) -> Result<()> {
        self.call(&format!("{e:?}"));
        match e {
            Ui::Failure(_) => self.view.borrow_mut().status = Status::Failed,
            Ui::ClearSnapshot => *self.view.borrow_mut() = Snapshot::default(),
            Ui::ClearFailure => self.view.borrow_mut().failure = None,
            Ui::PopTop => {
                self.context.borrow_mut().frames.pop();
            }
            _ => {}
        }
        Ok(())
    }
}
fn party(ready: &str, leader: bool) -> Party {
    Party {
        leader,
        members: vec![Member {
            handle: "self".into(),
            is_self: true,
            ready: ready.into(),
        }],
    }
}
impl LaunchIo for Native<'_> {
    fn variant(&self) -> Result<Option<bool>> {
        self.call("variant");
        if self.launch_case.get() == 10 {
            return Err(Error::Io);
        }
        Ok((self.launch_case.get() != 1).then_some(false))
    }
    fn map(&self) -> Result<Vec<u8>> {
        self.call("map");
        Ok(if self.launch_case.get() == 2 {
            b"Bad.map".to_vec()
        } else {
            b"lockout".to_vec()
        })
    }
    fn prepare_map(&self, m: &[u8]) -> Result<()> {
        assert_eq!(m, b"lockout.map");
        self.call("prepare_map");
        Ok(())
    }
    fn self_present(&self) -> Result<bool> {
        self.call("self");
        Ok(self.launch_case.get() != 3)
    }
    fn take(&self) -> Result<Take> {
        self.call("take");
        Ok(Take {
            present: self.launch_case.get() != 4,
            team_mode: (self.launch_case.get() == 5).then_some(true),
        })
    }
    fn local_counts(&self) -> Result<(u32, u32)> {
        self.call("counts");
        Ok((1, if self.launch_case.get() == 6 { 2 } else { 1 }))
    }
    fn local_count(&self, n: u32) -> Result<()> {
        assert_eq!(n, 1);
        self.call("count");
        Ok(())
    }
    fn admitted_name(&self, i: u32) -> Result<Vec<u8>> {
        assert_eq!(i, 0);
        self.call("admitted");
        Ok(if self.launch_case.get() == 7 {
            b"Other".to_vec()
        } else {
            b"Pilot".to_vec()
        })
    }
    fn profile_name(&self, i: u32) -> Result<Vec<u8>> {
        assert_eq!(i, 0);
        self.call("local_name");
        Ok(b"Pilot".to_vec())
    }
    fn append_profile(&self, i: u32, t: bool) -> Result<()> {
        assert_eq!(i, 0);
        assert!(!t);
        self.call("append");
        Ok(())
    }
    fn validate_map(&self) -> Result<bool> {
        self.call("validate");
        if self.launch_case.get() == 9 {
            Err(Error::Io)
        } else {
            Ok(self.launch_case.get() != 8)
        }
    }
    fn reject_content(&self) -> Result<()> {
        self.call("reject");
        Ok(())
    }
    fn publish(&self) -> Result<()> {
        self.call("publish");
        Ok(())
    }
    fn discard(&self) -> Result<()> {
        self.call("discard");
        Ok(())
    }
    fn failure(&self) -> Result<(bool, Vec<u8>)> {
        self.call("failure");
        Ok((self.launch_case.get() == 10, vec![0xff, b'x']))
    }
}
#[test]
fn launch_uses_real_admitted_roster_then_native_content_validation() {
    let c = Controller::default();
    let io = Native::new(&c);
    assert!(c.launch(&io).unwrap());
    assert_eq!(
        &*io.calls.borrow(),
        &[
            "variant",
            "map",
            "prepare_map",
            "self",
            "take",
            "counts",
            "count",
            "admitted",
            "local_name",
            "append",
            "validate",
            "publish"
        ]
    );
}
#[test]
fn launch_policy_refusals_discard_before_cancellation_and_ui() {
    for case in [1, 2, 3, 5, 6, 7] {
        let c = Controller::default();
        let io = Native::new(&c);
        io.launch_case.set(case);
        assert!(!c.launch(&io).unwrap());
        let trace = io.calls.borrow();
        let discard = trace.iter().position(|s| s == "discard").unwrap();
        assert_eq!(trace[discard + 1], "cancel");
        assert!(trace[discard + 2].starts_with("Unavailable("));
        assert!(!trace.iter().any(|s| s == "publish"));
    }
}
#[test]
fn native_content_refusal_notifies_owner_without_relabeling_other_errors() {
    let c = Controller::default();
    let io = Native::new(&c);
    io.launch_case.set(8);
    assert!(!c.launch(&io).unwrap());
    assert!(io.calls.borrow().contains(&"reject".into()));
    assert!(!io.calls.borrow().contains(&"cancel".into()));
    io.calls.borrow_mut().clear();
    io.launch_case.set(9);
    assert_eq!(c.launch(&io), Err(Error::Io));
    assert!(!io.calls.borrow().contains(&"reject".into()));
    assert!(!io.calls.borrow().contains(&"cancel".into()));
}
#[test]
fn missing_transport_never_fabricates_a_publication() {
    let c = Controller::default();
    let io = Native::new(&c);
    io.launch_case.set(4);
    assert!(c.launch(&io).unwrap());
    assert!(!io.calls.borrow().contains(&"publish".into()));
    assert!(!io.calls.borrow().contains(&"counts".into()));
}
#[test]
fn native_format_error_bytes_survive_recovery_without_utf8_conversion() {
    let c = Controller::default();
    let io = Native::new(&c);
    io.launch_case.set(10);
    assert!(!c.launch(&io).unwrap());
    assert_eq!(
        &*io.calls.borrow(),
        &[
            "variant",
            "failure",
            "discard",
            "cancel",
            "Unavailable([255, 120])"
        ]
    );
}
#[test]
fn io_reenters_state_without_any_rust_guard() {
    let c = Controller::default();
    let io = Native::new(&c);
    c.refresh(&io).unwrap();
    assert!(c.state().active);
    assert!(io.calls.borrow().contains(&"refresh".into()));
}
#[test]
fn solo_search_enqueues_once() {
    let c = Controller::default();
    let io = Native::new(&c);
    c.refresh(&io).unwrap();
    c.quick(true);
    c.update(&io).unwrap();
    c.update(&io).unwrap();
    assert_eq!(io.calls.borrow().iter().filter(|s| *s == "join").count(), 1);
}
#[test]
fn busy_party_waits_then_requires_actual_consent() {
    let c = Controller::default();
    let io = Native::new(&c);
    c.refresh(&io).unwrap();
    io.context.borrow_mut().party = Some(party("", true));
    *io.account.borrow_mut() = Account {
        party: Some(party("", true)),
        busy: true,
        ..Account::default()
    };
    c.quick(true);
    c.update(&io).unwrap();
    assert!(c.state().quick);
    io.account.borrow_mut().busy = false;
    c.update(&io).unwrap();
    assert!(c.state().consent);
    io.account.borrow_mut().party = Some(party("p", true));
    c.update(&io).unwrap();
    assert!(!c.state().quick);
    assert!(io.calls.borrow().contains(&"join".into()));
}
#[test]
fn completed_consent_refusal_does_not_enqueue() {
    let c = Controller::default();
    let io = Native::new(&c);
    c.refresh(&io).unwrap();
    io.context.borrow_mut().party = Some(party("", true));
    io.account.borrow_mut().party = Some(party("", true));
    c.quick(true);
    c.update(&io).unwrap();
    c.update(&io).unwrap();
    assert_eq!(io.view.borrow().status, Status::Failed);
    assert!(!io.calls.borrow().contains(&"join".into()));
}
#[test]
fn follower_consent_does_not_enqueue_its_own_ticket() {
    let c = Controller::default();
    let io = Native::new(&c);
    c.refresh(&io).unwrap();
    io.context.borrow_mut().party = Some(party("p", false));
    io.account.borrow_mut().party = Some(party("p", false));
    c.quick(true);
    c.update(&io).unwrap();
    assert!(!c.state().quick);
    assert!(!io.calls.borrow().contains(&"join".into()));
}
#[test]
fn loading_requires_published_zero_and_real_launch() {
    for ms in [None, Some(10001), Some(1), Some(0)] {
        let c = Controller::default();
        let mut io = Native::new(&c);
        c.refresh(&io).unwrap();
        {
            let mut v = io.view.borrow_mut();
            v.status = Status::Loading;
            v.matched = true;
            v.countdown_supported = true;
            v.countdown_ms = ms;
        }
        io.launch = true;
        c.update(&io).unwrap();
        assert_eq!(c.state().have_fun, ms == Some(0));
        assert_eq!(io.calls.borrow().contains(&"launch".into()), ms == Some(0));
    }
}
#[test]
fn zero_retries_when_actual_transport_is_not_transferred() {
    let c = Controller::default();
    let io = Native::new(&c);
    c.refresh(&io).unwrap();
    {
        let mut v = io.view.borrow_mut();
        v.status = Status::Loading;
        v.matched = true;
        v.countdown_supported = true;
        v.countdown_ms = Some(0);
    }
    c.update(&io).unwrap();
    c.update(&io).unwrap();
    assert!(!c.state().have_fun);
    assert_eq!(
        io.calls.borrow().iter().filter(|s| *s == "launch").count(),
        2
    );
}
#[test]
fn profile_bytes_preserve_invalid_utf8_without_native_string_conversion() {
    let c = Controller::default();
    let io = Native::new(&c);
    *io.profile.borrow_mut() = vec![0xff, 0xfe, b'a'];
    c.refresh(&io).unwrap();
    assert_eq!(c.state().name, vec![0xff, 0xfe, b'a']);
    let copy = c.copy();
    c.stop();
    assert!(copy.state().active);
    assert!(!c.state().active);
}
#[test]
fn exact_legacy_variant_choice_retains_authored_suffix_and_order() {
    let content = vec![b"Slayer \xee\x80\x88".to_vec(), b"Slayer".to_vec()];
    assert_eq!(
        variant_choice(b"Slayer", None, &content, &[]),
        VariantChoice { kind: 2, index: 0 }
    );
    assert_eq!(variant_choice(b"Other", None, &content, &[]).kind, 0);
    assert_eq!(
        variant_choice(b"Oddball", None, &[], &[b"Oddball".to_vec()]),
        VariantChoice { kind: 3, index: 0 }
    );
}
