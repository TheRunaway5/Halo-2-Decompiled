use halo2_online_protocol::{
    matchmaking_frontend_account_flow::Entry,
    matchmaking_frontend_couch::*,
    matchmaking_frontend_party_flow::{Controller, State as PartyState},
};
use std::cell::{Cell, RefCell};
#[derive(Clone, Default)]
struct Client {
    generation: u32,
    status: i32,
    id: Option<Vec<u8>>,
    authorized: bool,
    steam: bool,
}
struct TestIo<'a> {
    null_factory: Cell<bool>,
    null_worker: Cell<bool>,
    flow: &'a Controller,
    facts: RefCell<Facts>,
    primary: RefCell<Option<Client>>,
    locals: RefCell<[Option<Client>; 4]>,
    held: RefCell<Option<Client>>,
    events: RefCell<Vec<String>>,
    states: RefCell<Vec<State>>,
    fail: RefCell<Option<String>>,
    hook: RefCell<Option<fn(&TestIo<'_>, &str)>>,
    description: RefCell<Vec<u8>>,
    built: RefCell<Frame>,
    players: RefCell<Vec<u32>>,
}
impl<'a> TestIo<'a> {
    fn new(flow: &'a Controller) -> Self {
        let frame = Frame {
            screen: 186,
            description: b"old".to_vec(),
            ..Frame::default()
        };
        Self {
            null_factory: Cell::new(false),
            null_worker: Cell::new(false),
            flow,
            facts: RefCell::new(Facts {
                status: 3,
                mode: 3,
                account_client: true,
                authenticated: true,
                factory: true,
                local_accounts: vec![false; 4],
                profiles: vec![vec![], b"one".to_vec(), b"two".to_vec(), b"three".to_vec()],
                inputs: vec![5, -1, -1, -1],
                profile: 7,
                signed_in: b"host".to_vec(),
                frame: Some(frame.clone()),
                frames: vec![frame],
                ..Facts::default()
            }),
            primary: RefCell::new(Some(Client {
                generation: 1,
                status: 3,
                id: Some(b"primary".to_vec()),
                authorized: true,
                steam: true,
            })),
            locals: RefCell::new(Default::default()),
            held: RefCell::new(None),
            events: RefCell::new(vec![]),
            states: RefCell::new(vec![]),
            fail: RefCell::new(None),
            hook: RefCell::new(None),
            description: RefCell::new(b"new".to_vec()),
            built: RefCell::new(Frame {
                screen: 31,
                context: CONTEXT,
                entries: vec![Entry {
                    label: b"new".to_vec(),
                    destination: -5,
                    value: 0,
                }],
                ..Frame::default()
            }),
            players: RefCell::new(vec![]),
        }
    }
    fn event(&self, s: String) -> Result<()> {
        self.states.borrow_mut().push(self.flow.couch_state());
        self.events.borrow_mut().push(s.clone());
        if self.fail.borrow().as_ref() == Some(&s) {
            return Err(s);
        }
        if let Some(h) = *self.hook.borrow() {
            h(self, &s);
        }
        Ok(())
    }
    fn events(&self) -> Vec<String> {
        self.events.borrow().clone()
    }
    fn client(&self, l: Login) -> Result<Client> {
        match l {
            Login::Primary => self.primary.borrow().clone(),
            Login::Local(i) => self.locals.borrow().get(i as usize).and_then(Clone::clone),
            Login::Held => self.held.borrow().clone(),
        }
        .ok_or("missing fake login owner".into())
    }
    fn set_frame(&self, frame: Frame) {
        let mut f = self.facts.borrow_mut();
        f.frame = Some(frame.clone());
        f.frames = vec![frame];
    }
    fn couch_action(&self, value: i32) {
        self.set_frame(Frame {
            screen: 31,
            context: CONTEXT,
            entries: vec![Entry {
                label: vec![],
                destination: -80,
                value,
            }],
            ..Frame::default()
        });
    }
    fn local(&self, i: usize, status: i32, id: Option<&[u8]>, auth: bool) {
        self.locals.borrow_mut()[i] = Some(Client {
            generation: 10 + i as u32,
            status,
            id: id.map(Vec::from),
            authorized: auth,
            steam: false,
        });
    }
}
impl Io for TestIo<'_> {
    fn facts(&self) -> Result<Facts> {
        let mut f = self.facts.borrow().clone();
        f.primary_login = self.primary.borrow().is_some();
        f.local_logins = self.locals.borrow().iter().map(Option::is_some).collect();
        Ok(f)
    }
    fn effect(&self, e: Effect) -> Result<()> {
        self.event(format!("{e:?}"))?;
        match e {
            Effect::Back => {
                if !back(self.flow, self)? && !back_join(self.flow, self)? {
                    let mut f = self.facts.borrow_mut();
                    if f.frames.len() > 1 {
                        f.frames.pop();
                    }
                    f.frame = f.frames.last().cloned();
                }
            }
            Effect::Open(s, c) => {
                let mut f = self.facts.borrow_mut();
                let frame = Frame {
                    screen: s,
                    context: c,
                    ..Frame::default()
                };
                f.frames.push(frame.clone());
                f.frame = Some(frame);
            }
            Effect::Pop => {
                let mut f = self.facts.borrow_mut();
                f.frames.pop();
                f.frame = f.frames.last().cloned();
            }
            Effect::Description(t) => {
                self.facts.borrow_mut().frame.as_mut().unwrap().description = t
            }
            Effect::Replace(e) => self.facts.borrow_mut().frame.as_mut().unwrap().entries = e,
            Effect::Append(e) => self
                .facts
                .borrow_mut()
                .frame
                .as_mut()
                .unwrap()
                .entries
                .push(e),
            Effect::LocalProfile(i, t) => self.facts.borrow_mut().profiles[i as usize] = t,
            Effect::LocalInput(i, id) => self.facts.borrow_mut().inputs[i as usize] = id,
            Effect::Profile(i) => self.facts.borrow_mut().profile = i,
            Effect::ResetAccount(i) => self.facts.borrow_mut().local_accounts[i as usize] = false,
            Effect::ClearAccounts => self.facts.borrow_mut().local_accounts.fill(false),
            Effect::ResetLogin(i) => self.locals.borrow_mut()[i as usize] = None,
            Effect::CreateLogin(i) => {
                if !self.null_factory.get() {
                    self.local(i as usize, 0, None, false);
                }
            }
            Effect::CreateAccount(i) => {
                if !self.null_worker.get() {
                    self.facts.borrow_mut().local_accounts[i as usize] = true;
                }
            }
            Effect::DescriptionIndex(i, t) => {
                self.facts.borrow_mut().frames[i as usize].description = t
            }
            Effect::CommitBuilt(instance, selected) => {
                let mut frame = self.built.borrow().clone();
                frame.instance = instance;
                frame.selected = selected;
                let mut f = self.facts.borrow_mut();
                *f.frames.last_mut().unwrap() = frame.clone();
                f.frame = Some(frame);
            }
            Effect::ProfileFromCurrent(i) => {
                self.facts.borrow_mut().profiles[i as usize] = b"selected profile".to_vec()
            }
            _ => (),
        }
        Ok(())
    }
    fn hold_login(&self, i: u32) -> Result<bool> {
        *self.held.borrow_mut() = self.locals.borrow().get(i as usize).cloned().flatten();
        Ok(self.held.borrow().is_some())
    }
    fn release_login(&self) -> Result<()> {
        self.held.borrow_mut().take();
        Ok(())
    }
    fn snapshot(&self, l: Login) -> Result<Snapshot> {
        let c = self.client(l)?;
        self.event(format!("snapshot:{l:?}:{}", c.generation))?;
        Ok(Snapshot {
            status: c.status,
            identity: c.id,
        })
    }
    fn authorized(&self, l: Login) -> Result<bool> {
        let c = self.client(l)?;
        self.event(format!("authorized:{l:?}:{}", c.generation))?;
        Ok(c.authorized)
    }
    fn steam(&self, l: Login) -> Result<bool> {
        let c = self.client(l)?;
        self.event(format!("steam:{l:?}:{}", c.generation))?;
        Ok(c.steam)
    }
    fn cancel(&self, l: Login) -> Result<()> {
        let c = self.client(l)?;
        self.event(format!("cancel:{l:?}:{}", c.generation))
    }
    fn identity(&self) -> Result<bool> {
        identity(self)
    }
    fn description(&self) -> Result<Vec<u8>> {
        self.event("description".into())?;
        Ok(self.description.borrow().clone())
    }
    fn build(&self) -> Result<Frame> {
        self.event("build".into())?;
        Ok(self.built.borrow().clone())
    }
    fn players(&self) -> Result<Vec<u32>> {
        self.event("players".into())?;
        Ok(self.players.borrow().clone())
    }
}
fn flow(s: State) -> Controller {
    Controller::from_state(PartyState {
        couch: s,
        ..PartyState::default()
    })
}
#[test]
fn mutable_blocks_exact_search_phases_after_identity_read() {
    for status in [-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 99] {
        let c = Controller::default();
        let io = TestIo::new(&c);
        io.facts.borrow_mut().status = status;
        assert_eq!(mutable(&io).unwrap(), !matches!(status, 4..=8));
        assert_eq!(io.events(), ["snapshot:Primary:1"]);
    }
}
#[test]
fn identity_gate_reads_no_provider_for_unverified_account() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.facts.borrow_mut().authenticated = false;
    assert!(!identity(&io).unwrap());
    assert!(io.events().is_empty());
    io.facts.borrow_mut().account_client = false;
    assert!(identity(&io).unwrap());
}
#[test]
fn empty_provider_identity_still_present() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.primary.borrow_mut().as_mut().unwrap().id = Some(vec![]);
    assert!(identity(&io).unwrap());
    io.primary.borrow_mut().as_mut().unwrap().id = None;
    assert!(!identity(&io).unwrap());
}
#[test]
fn open_commits_options_before_navigation_failure() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    *io.fail.borrow_mut() = Some(format!("Open(31, {CONTEXT})"));
    assert!(open(&c, &io, 2).is_err());
    assert_eq!(c.couch_state().options, Some(2));
    assert_eq!(io.states.borrow().last().unwrap().options, Some(2));
}
#[test]
fn open_guards_slots_and_empty_profiles_before_identity() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    for i in [0, 4, u32::MAX] {
        open(&c, &io, i).unwrap();
    }
    io.facts.borrow_mut().profiles[1].clear();
    open(&c, &io, 1).unwrap();
    assert!(io.events().is_empty());
}
#[test]
fn selected_controller_prioritizes_joining_and_preserves_zero_option() {
    let c = flow(State {
        joining: Some(2),
        options: Some(3),
        ..State::default()
    });
    assert_eq!(selected_controller(&c), Some(2));
    let c = flow(State {
        options: Some(0),
        ..State::default()
    });
    assert_eq!(selected_controller(&c), Some(0));
}
#[test]
fn populate_without_options_preserves_description_then_title_order() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    populate(&c, &io).unwrap();
    let e = io.events();
    assert_eq!(e[0], "Description([])");
    assert!(e[1].starts_with("Title("));
    assert_eq!(
        io.facts.borrow().frame.as_ref().unwrap().entries[0].label,
        b"Back"
    );
}
#[test]
fn populate_held_login_survives_slot_replacement() {
    fn hook(io: &TestIo<'_>, event: &str) {
        if event == "snapshot:Held:11" {
            io.locals.borrow_mut()[1] = Some(Client {
                generation: 99,
                authorized: false,
                ..Client::default()
            });
        }
    }
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.local(1, 3, Some(b"old"), true);
    *io.hook.borrow_mut() = Some(hook);
    populate(&c, &io).unwrap();
    assert!(io.events().contains(&"authorized:Held:11".into()));
    assert!(!io.events().contains(&"authorized:Held:99".into()));
    assert_eq!(
        io.facts.borrow().frame.as_ref().unwrap().entries[1].label,
        b"Continue with Steam"
    );
    assert!(io.held.borrow().is_none());
}
#[test]
fn busy_login_skips_authorization_and_shows_provider_cancel() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.local(1, 2, None, false);
    populate(&c, &io).unwrap();
    let f = io.facts.borrow();
    let rows = &f.frame.as_ref().unwrap().entries;
    assert_eq!(rows.len(), 2);
    assert_eq!(rows[0].label, b"Cancel Steam Sign-In");
    assert!(!io.events().iter().any(|s| s.starts_with("authorized")));
}
#[test]
fn text_raw_name_and_unknown_index_do_not_observe_login() {
    let c = flow(State {
        options: Some(2),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.facts.borrow_mut().profiles[2] = vec![255, 0, 128];
    assert_eq!(text(&c, &io, 0).unwrap(), [255, 0, 128]);
    assert!(text(&c, &io, 2).unwrap().is_empty());
    assert!(io.events().is_empty());
}
#[test]
fn failed_login_checks_authorization_before_failure_text() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.local(1, 4, None, false);
    assert_eq!(text(&c, &io, 1).unwrap(), b"Sign-in failed. Try again.");
    assert_eq!(io.events(), ["snapshot:Held:11", "authorized:Held:11"]);
}
#[test]
fn primary_pending_short_circuits_local_snapshot_scan() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(1);
    io.primary.borrow_mut().as_mut().unwrap().status = 1;
    io.local(3, 2, None, false);
    accept(&c, &io).unwrap();
    assert!(io.events().contains(&"snapshot:Primary:1".into()));
    assert!(!io.events().contains(&"snapshot:Local(3):13".into()));
}
#[test]
fn pending_local_scan_includes_slot_zero_and_stops_in_order() {
    let c = flow(State {
        options: Some(2),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(1);
    io.local(0, 2, None, false);
    io.local(3, 1, None, false);
    accept(&c, &io).unwrap();
    assert!(io.events().contains(&"snapshot:Local(0):10".into()));
    assert!(!io.events().contains(&"snapshot:Local(3):13".into()));
}
#[test]
fn unavailable_factory_precedes_pending_provider_reads() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(1);
    io.facts.borrow_mut().factory = false;
    accept(&c, &io).unwrap();
    assert_eq!(io.events().len(), 1);
    assert!(io.events()[0].starts_with("Unavailable"));
}
#[test]
fn local_signin_creates_binds_starts_then_revises() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(1);
    accept(&c, &io).unwrap();
    let e = io.events();
    assert_eq!(
        &e[e.len() - 4..],
        [
            "CreateLogin(1)",
            "BindLogin(1)",
            "StartLogin(1)",
            "Revision"
        ]
    );
}
#[test]
fn guest_action_clears_current_replacement_after_cancel() {
    fn hook(io: &TestIo<'_>, event: &str) {
        if event == "cancel:Local(1):11" {
            io.local(1, 3, Some(b"replacement"), true);
        }
    }
    let c = flow(State {
        options: Some(1),
        subjects: [vec![], b"old".to_vec(), vec![], vec![]],
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(0);
    io.local(1, 0, None, false);
    *io.hook.borrow_mut() = Some(hook);
    accept(&c, &io).unwrap();
    assert!(io.locals.borrow()[1].is_none());
    assert!(c.couch_state().subjects[1].is_empty());
    assert_eq!(c.couch_state().revision, 2);
    assert!(c.couch_state().options.is_none());
}
#[test]
fn signout_failure_keeps_guest_login_subject_and_revision() {
    let c = flow(State {
        options: Some(1),
        subjects: [vec![], b"old".to_vec(), vec![], vec![]],
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(0);
    io.local(1, 3, Some(b"old"), true);
    io.facts.borrow_mut().local_accounts[1] = true;
    *io.fail.borrow_mut() = Some("SignOutAccount(1)".into());
    assert!(accept(&c, &io).is_err());
    assert!(io.locals.borrow()[1].is_some());
    assert_eq!(c.couch_state().subjects[1], b"old");
    assert_eq!(c.couch_state().revision, 0);
}
#[test]
fn guest_profile_selection_clears_join_before_identity_navigation() {
    let c = flow(State {
        joining: Some(2),
        joining_online: true,
        host_profile: 7,
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.set_frame(Frame {
        screen: 10,
        entries: vec![Entry {
            label: vec![],
            destination: -80,
            value: 0,
        }],
        ..Frame::default()
    });
    accept(&c, &io).unwrap();
    assert_eq!(io.facts.borrow().profiles[2], b"Guest 3");
    assert!(c.couch_state().joining.is_none());
    assert!(!c.couch_state().joining_online);
    assert_eq!(c.couch_state().options, Some(2));
}
#[test]
fn update_duplicate_primary_revokes_worker_but_keeps_observed_subject() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.local(1, 3, Some(b"primary"), true);
    io.facts.borrow_mut().local_accounts[1] = true;
    update(&c, &io).unwrap();
    assert_eq!(c.couch_state().subjects[1], b"primary");
    assert!(!io.facts.borrow().local_accounts[1]);
    assert!(!io.events().contains(&"CreateAccount(1)".into()));
}
#[test]
fn duplicate_slots_use_fresh_prior_observed_subjects() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.local(1, 3, Some(&[255, 0]), true);
    io.local(2, 3, Some(&[255, 0]), true);
    update(&c, &io).unwrap();
    assert!(io.events().contains(&"CreateAccount(1)".into()));
    assert!(!io.events().contains(&"CreateAccount(2)".into()));
    assert_eq!(c.couch_state().subjects[2], [255, 0]);
}
#[test]
fn authorization_precedes_local_identity_and_primary_snapshot() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.local(1, 3, Some(b"one"), true);
    update(&c, &io).unwrap();
    let e = io.events();
    let a = e
        .iter()
        .position(|s| s == "authorized:Local(1):11")
        .unwrap();
    assert_eq!(e[a + 1], "snapshot:Local(1):11");
    assert_eq!(e[a + 3], "snapshot:Primary:1");
}
#[test]
fn stale_retained_online_descriptions_are_updated_individually() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    {
        let mut f = io.facts.borrow_mut();
        f.frames = vec![
            Frame {
                screen: 186,
                description: b"old".to_vec(),
                ..Frame::default()
            },
            Frame {
                screen: 26,
                ..Frame::default()
            },
            Frame {
                screen: 186,
                description: b"new".to_vec(),
                ..Frame::default()
            },
        ];
    }
    update(&c, &io).unwrap();
    assert_eq!(
        io.events().iter().filter(|e| *e == "description").count(),
        2
    );
    assert_eq!(
        io.events()
            .iter()
            .filter(|e| e.starts_with("DescriptionIndex"))
            .count(),
        1
    );
}
#[test]
fn rebuilt_menu_preserves_instance_and_clips_focus_only_on_entry_change() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.set_frame(Frame {
        screen: 31,
        context: CONTEXT,
        instance: u64::MAX,
        selected: 99,
        entries: vec![Entry::default()],
        ..Frame::default()
    });
    update(&c, &io).unwrap();
    assert_eq!(io.facts.borrow().frame.as_ref().unwrap().instance, u64::MAX);
    assert_eq!(io.facts.borrow().frame.as_ref().unwrap().selected, 0);
}
#[test]
fn same_entries_discard_rebuild_without_replacing_other_frame_fields() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.set_frame(Frame {
        screen: 31,
        context: CONTEXT,
        instance: 77,
        description: b"keep".to_vec(),
        entries: io.built.borrow().entries.clone(),
        ..Frame::default()
    });
    update(&c, &io).unwrap();
    assert!(io.events().contains(&"DiscardBuilt".into()));
    assert_eq!(
        io.facts.borrow().frame.as_ref().unwrap().description,
        b"keep"
    );
}
#[test]
fn clear_preserves_join_slot_host_and_failure_prefixes() {
    let c = flow(State {
        options: Some(1),
        joining: Some(2),
        joining_online: true,
        host_profile: 99,
        revision: u64::MAX,
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.local(1, 3, Some(b"one"), true);
    clear(&c, &io).unwrap();
    let s = c.couch_state();
    assert_eq!(s.joining, Some(2));
    assert_eq!(s.host_profile, 99);
    assert_eq!(s.revision, 0);
    assert!(s.options.is_none() && !s.joining_online);
    assert!(io.locals.borrow().iter().all(Option::is_none));
}
#[test]
fn controller_binding_reuses_first_known_input_without_identity() {
    let c = flow(State {
        options: Some(2),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.facts.borrow_mut().inputs = vec![5, 7, 7, -1];
    assert_eq!(controller_slot(&c, &io, 7, true).unwrap(), Some(1));
    assert_eq!(controller_slot(&c, &io, -1, true).unwrap(), None);
    assert!(io.events().is_empty());
}
#[test]
fn new_controller_uses_empty_slot_and_commits_state_before_open_failure() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.facts.borrow_mut().profiles[2].clear();
    *io.fail.borrow_mut() = Some("Open(10, 0)".into());
    assert!(controller_slot(&c, &io, 9, true).is_err());
    assert_eq!(c.couch_state().joining, Some(2));
    assert!(c.couch_state().joining_online);
    assert_eq!(c.couch_state().host_profile, 7);
    assert_eq!(io.facts.borrow().inputs[2], 9);
}
#[test]
fn local_title_rebinds_primary_when_online_join_is_ineligible() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    {
        let mut f = io.facts.borrow_mut();
        f.inputs[0] = -1;
        f.frame.as_mut().unwrap().screen = 9;
    }
    assert_eq!(controller_slot(&c, &io, 8, true).unwrap(), Some(0));
    assert_eq!(io.facts.borrow().inputs[0], 8);
    assert!(c.couch_state().joining.is_none());
}
#[test]
fn finish_local_restores_host_before_revision_and_clears_joining_online_later() {
    let c = flow(State {
        joining: Some(2),
        joining_online: false,
        host_profile: 44,
        ..State::default()
    });
    let io = TestIo::new(&c);
    finish(&c, &io).unwrap();
    assert_eq!(
        io.events(),
        [
            "ProfileFromCurrent(2)",
            "ResetLocalTeam(2)",
            "Profile(44)",
            "Revision"
        ]
    );
    assert!(io.states.borrow().last().unwrap().joining.is_none());
    assert_eq!(io.facts.borrow().profile, 44);
}
#[test]
fn leave_rejects_immutable_online_phase_before_mutating_roster() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.facts.borrow_mut().status = 5;
    leave(&c, &io, 1).unwrap();
    assert_eq!(io.events(), ["snapshot:Primary:1"]);
    assert_eq!(io.facts.borrow().profiles[1], b"one");
}
#[test]
fn back_uses_held_login_but_re_reads_changed_options_for_reset() {
    fn hook(io: &TestIo<'_>, event: &str) {
        if event == "cancel:Held:11" {
            open(io.flow, io, 2).unwrap();
        }
    }
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(0);
    io.local(1, 3, None, false);
    io.local(2, 3, Some(b"two"), true);
    *io.hook.borrow_mut() = Some(hook);
    assert!(back(&c, &io).unwrap());
    assert!(io.locals.borrow()[1].is_some());
    assert!(io.locals.borrow()[2].is_none());
    assert!(c.couch_state().options.is_none());
}
#[test]
fn back_join_resets_controller_and_keeps_couch_revision() {
    let c = flow(State {
        joining: Some(2),
        joining_online: true,
        host_profile: 99,
        revision: 7,
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.set_frame(Frame {
        screen: 10,
        ..Frame::default()
    });
    assert!(back_join(&c, &io).unwrap());
    assert_eq!(io.facts.borrow().inputs[2], -1);
    assert_eq!(io.facts.borrow().profile, 99);
    assert_eq!(c.couch_state().revision, 7);
    assert!(c.couch_state().joining.is_none() && !c.couch_state().joining_online);
}
#[test]
fn profile_entry_filters_primary_and_joined_names_only_during_profile_join() {
    let c = flow(State {
        joining: Some(2),
        joining_online: true,
        ..State::default()
    });
    let io = TestIo::new(&c);
    assert!(!profile_entry(&c, &io, 10, b"host").unwrap());
    assert!(!profile_entry(&c, &io, 10, b"one").unwrap());
    assert!(profile_entry(&c, &io, 193, b"host").unwrap());
    assert!(guest_entry(&c, 10));
    assert!(!guest_entry(&c, 193));
}
#[test]
fn online_players_keep_order_duplicates_and_skip_only_controller_zero() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    *io.players.borrow_mut() = vec![0, 2, 1, 2];
    local_players(&io).unwrap();
    assert_eq!(
        io.events(),
        ["players", "EmitLocal(1)", "EmitLocal(2)", "EmitLocal(3)"]
    );
    *io.players.borrow_mut() = vec![4];
    assert!(local_players(&io).is_err());
}
#[test]
fn invalid_selected_rows_return_safe_errors_instead_of_cpp_undefined_access() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(1);
    io.facts.borrow_mut().frame.as_mut().unwrap().selected = u32::MAX;
    assert!(accept(&c, &io).is_err());
}

#[test]
fn null_factory_returns_error_without_bind_or_start() {
    let c = flow(State {
        options: Some(1),
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.couch_action(1);
    io.null_factory.set(true);
    assert!(
        accept(&c, &io)
            .unwrap_err()
            .contains("factory produced no owner")
    );
    assert!(!io.events().contains(&"BindLogin(1)".into()));
}
#[test]
fn null_profile_worker_keeps_observed_subject_and_returns_error_before_profile() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.local(1, 3, Some(b"one"), true);
    io.null_worker.set(true);
    assert!(
        update(&c, &io)
            .unwrap_err()
            .contains("profile worker produced no owner")
    );
    assert_eq!(c.couch_state().subjects[1], b"one");
    assert!(!io.events().contains(&"ProfileAccount(1)".into()));
}
#[test]
fn primary_rebind_short_circuits_absent_frame_when_online_is_ineligible() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    {
        let mut f = io.facts.borrow_mut();
        f.authenticated = false;
        f.frame = None;
        f.frames.clear();
        f.inputs = vec![-1; 4];
    }
    assert_eq!(controller_slot(&c, &io, 9, true).unwrap(), Some(0));
    assert_eq!(io.facts.borrow().inputs[0], 9);
    assert_eq!(io.events(), ["LocalInput(0, 9)"]);
}
