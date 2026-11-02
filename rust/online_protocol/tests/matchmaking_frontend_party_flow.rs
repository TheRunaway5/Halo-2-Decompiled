use halo2_online_protocol::matchmaking_frontend_party::Member;
use halo2_online_protocol::matchmaking_frontend_party_flow::*;
use std::cell::RefCell;

struct Native<'a> {
    controller: &'a Controller,
    facts: RefCell<Facts>,
    rows: Vec<Member>,
    commands: RefCell<Vec<Command>>,
    effects: RefCell<Vec<Ui>>,
    calls: RefCell<Vec<&'static str>>,
    accept: bool,
    failure: Option<&'static str>,
    worker_serial: u64,
    withdraw_queue_on_command: bool,
}
fn frame(screen: u32, context: u32, destination: i32, value: i32) -> Frame {
    Frame {
        screen,
        context,
        instance: 0xffff_ffff_0000_0042,
        entries: vec![Entry { destination, value }],
        ..Frame::default()
    }
}
impl<'a> Native<'a> {
    fn new(
        controller: &'a Controller,
        screen: u32,
        context: u32,
        destination: i32,
        value: i32,
    ) -> Self {
        Self {
            controller,
            facts: RefCell::new(Facts {
                mode: 3,
                frames: vec![
                    frame(6, 0, 0, 0),
                    frame(186, 0, 0, 0),
                    frame(screen, context, destination, value),
                ],
                account_client: true,
                queue_client: true,
                authenticated: true,
                signed_in: vec![b'p', 0, 255],
                party: Some(Party {
                    leader: true,
                    version: u64::MAX - 9,
                    privacy: Some(Privacy {
                        mode: 0,
                        revision: u64::MAX - 10,
                        context: vec![255, 0, b'c'],
                    }),
                    ..Party::default()
                }),
                ..Facts::default()
            }),
            rows: vec![Member {
                handle: vec![255, 0, b'm'],
                ..Member::default()
            }],
            commands: RefCell::new(Vec::new()),
            effects: RefCell::new(Vec::new()),
            calls: RefCell::new(Vec::new()),
            accept: true,
            failure: None,
            worker_serial: u64::MAX - 3,
            withdraw_queue_on_command: false,
        }
    }
    fn call(&self, name: &'static str) -> Result<()> {
        // Every native callback can inspect the same Rust owner immediately.
        let _ = self.controller.state();
        self.calls.borrow_mut().push(name);
        if self.failure == Some(name) {
            Err(name.into())
        } else {
            Ok(())
        }
    }
}
impl Io for Native<'_> {
    fn facts(&self) -> Result<Facts> {
        let _ = self.controller.state();
        Ok(self.facts.borrow().clone())
    }
    fn members(&self) -> Result<Vec<Member>> {
        self.call("members")?;
        Ok(self.rows.clone())
    }
    fn command(&self, command: &Command) -> Result<bool> {
        self.call("command")?;
        self.commands.borrow_mut().push(command.clone());
        if self.withdraw_queue_on_command {
            self.facts.borrow_mut().queue_client = false;
        }
        Ok(self.accept)
    }
    fn command_serial(&self) -> Result<u64> {
        self.call("serial")?;
        Ok(self.worker_serial)
    }
    fn update_account(&self) -> Result<()> {
        self.call("update")
    }
    fn cancel_queue(&self) -> Result<()> {
        self.call("cancel")
    }
    fn lobby_input(&self, _: i32) -> Result<bool> {
        self.call("lobby")?;
        Ok(false)
    }
    fn ui(&self, effect: Ui) -> Result<()> {
        self.call("ui")?;
        self.effects.borrow_mut().push(effect.clone());
        match effect {
            Ui::Pop | Ui::Back => {
                self.facts.borrow_mut().frames.pop();
            }
            Ui::Open(screen, context) => self
                .facts
                .borrow_mut()
                .frames
                .push(frame(screen, context, 0, 0)),
            Ui::FormatError(message) => return Err(String::from_utf8(message).unwrap()),
            _ => (),
        }
        Ok(())
    }
}
#[test]
fn classifier_keeps_mode_context_and_full_width_values() {
    for mode in [-1, 0, 1, 2, 3, 4, i32::MAX] {
        assert_eq!(screen(mode, 14, 45), mode == 3);
        for context in [0, 1, 2, 42, 45, 0x10002, u32::MAX] {
            assert_eq!(screen(mode, 26, context), context == 2);
            assert_eq!(screen(mode, 216, context), context == 0);
            assert_eq!(screen(mode, 31, context), matches!(context, 42 | 45));
            assert_eq!(picker(31, context), context == 45);
        }
        assert!(!screen(mode, 0x1000e, 0));
    }
}
#[test]
fn picker_tab_changes_before_rebuild_without_holding_state_across_io() {
    let c = Controller::default();
    let io = Native::new(&c, 31, 45, 0, 0);
    for direction in [0, 1, 0, -1, i32::MIN] {
        assert!(c.switch_tab(direction, &io).unwrap());
    }
    assert_eq!(c.state().tab, 1);
    assert_eq!(
        io.effects
            .borrow()
            .iter()
            .filter(|v| matches!(v, Ui::RebuildPicker))
            .count(),
        5
    );
    io.facts.borrow_mut().frames.last_mut().unwrap().context = 42;
    assert!(!c.switch_tab(1, &io).unwrap());
    assert_eq!(c.state().tab, 1);
}
#[test]
fn notice_dismissal_pruning_and_acceptance_keep_opaque_identity_and_version() {
    let c = Controller::default();
    let io = Native::new(&c, 186, 0, 0, 0);
    {
        let mut f = io.facts.borrow_mut();
        f.party = None;
        f.invitations = vec![
            Invite {
                id: vec![0, 255, b'a'],
                name: vec![254],
                version: u64::MAX,
            },
            Invite {
                id: b"next".to_vec(),
                ..Invite::default()
            },
        ];
    }
    assert!(c.notice(false, &io).unwrap());
    assert_eq!(c.state().dismissed, vec![vec![0, 255, b'a']]);
    c.prune_dismissed(&[vec![0, 255, b'a']]);
    assert_eq!(c.state().dismissed.len(), 1);
    c.prune_dismissed(&[]);
    assert!(c.state().dismissed.is_empty());
    assert!(c.notice(true, &io).unwrap());
    assert_eq!(
        &*io.calls.borrow(),
        &["ui", "command", "cancel", "update", "ui"]
    );
    assert_eq!(
        io.commands.borrow()[0],
        Command {
            action: b"accept".to_vec(),
            value: vec![0, 255, b'a'],
            version: u64::MAX,
            ..Command::default()
        }
    );
    c.notice(false, &io).unwrap();
    c.clear_dismissed();
    assert!(c.state().dismissed.is_empty());
}
#[test]
fn notice_refusals_do_not_submit_or_navigate() {
    for kind in 0..8 {
        let c = Controller::default();
        let mut io = Native::new(&c, 186, 0, 0, 0);
        {
            let mut f = io.facts.borrow_mut();
            f.party = None;
            f.invitations.push(Invite {
                id: b"invite".to_vec(),
                ..Invite::default()
            });
            match kind {
                0 => f.authenticated = false,
                1 => f.signed_in.clear(),
                2 => f.busy = true,
                3 => f.launch = true,
                4 => f.status = 6,
                5 => f.status = 7,
                6 => f.status = 8,
                _ => f.account_client = false,
            }
        }
        assert!(!c.notice(true, &io).unwrap());
        assert!(io.calls.borrow().is_empty());
        io.accept = false;
        assert!(!c.notice(true, &io).unwrap());
    }
}

#[test]
fn notice_observes_the_queue_resource_after_the_accepted_worker_callback() {
    let c = Controller::default();
    let mut io = Native::new(&c, 186, 0, 0, 0);
    io.withdraw_queue_on_command = true;
    {
        let mut facts = io.facts.borrow_mut();
        facts.party = None;
        facts.invitations.push(Invite {
            id: b"accepted".to_vec(),
            ..Invite::default()
        });
    }
    assert!(c.notice(true, &io).unwrap());
    assert_eq!(&*io.calls.borrow(), &["command", "update", "ui"]);
}
#[test]
fn command_navigation_observes_actual_updated_stack_and_preserves_all_fields() {
    for action in [b"create".to_vec(), b"accept".to_vec(), vec![255, 0, b'x']] {
        let c = Controller::default();
        let io = Native::new(&c, 31, 42, 0, 0);
        io.facts.borrow_mut().frames.insert(2, frame(14, 0, 0, 0));
        let command = Command {
            action: action.clone(),
            value: vec![0, 255],
            playlist: vec![254],
            revision: vec![253],
            version: u64::MAX,
            privacy: Some(u32::MAX),
            privacy_revision: u64::MAX - 1,
            privacy_context: vec![0, 252],
            join_context: vec![251, 0],
        };
        c.command(command.clone(), &io).unwrap();
        assert_eq!(&*io.commands.borrow(), &[command]);
        let f = io.facts.borrow();
        let top = f.frames.last().unwrap();
        if action == b"create" || action == b"accept" {
            assert_eq!(top.screen, 14);
            assert_eq!(f.frames.len(), 3);
        } else {
            assert_eq!((top.screen, top.context), (26, 2));
            assert_eq!(f.frames[f.frames.len() - 2].screen, 14);
        }
    }
}
#[test]
fn rejected_command_or_resource_absence_has_no_navigation_or_update() {
    for resource in [false, true] {
        let c = Controller::default();
        let mut io = Native::new(&c, 31, 42, 0, 0);
        io.accept = false;
        io.facts.borrow_mut().account_client = resource;
        c.command(Command::default(), &io).unwrap();
        assert_eq!(io.facts.borrow().frames.len(), 3);
        assert_eq!(
            &*io.calls.borrow(),
            if resource {
                &["command", "ui"][..]
            } else {
                &["ui"][..]
            }
        );
    }
}
#[test]
fn selected_member_reads_twice_and_remote_primary_or_signed_local_routes_match() {
    for controller in [-2, -1, 0, 1, 3] {
        let c = Controller::from_state(State {
            target: b"previous".to_vec(),
            kind: 1,
            version: 99,
            ..State::default()
        });
        let mut io = Native::new(&c, 26, 2, 31, 0);
        io.rows[0].controller = controller;
        assert!(c.accept(&io).unwrap());
        assert_eq!(&io.calls.borrow()[..3], &["lobby", "members", "members"]);
        assert_eq!(c.state().version, 0);
        if controller > 0 {
            assert_eq!(c.state().target, b"previous");
            assert_eq!(
                *io.effects.borrow(),
                vec![Ui::LocalPlayer(controller as u32)]
            );
        } else {
            assert_eq!(c.state().target, vec![255, 0, b'm']);
            assert_eq!(c.state().kind, 0);
            assert_eq!(*io.effects.borrow(), vec![Ui::Open(31, 42)]);
        }
    }
}
#[test]
fn incoming_outgoing_target_precedence_and_version_binding_survive_duplicate_names() {
    for outgoing in [false, true] {
        let c = Controller::default();
        let io = Native::new(&c, 26, 2, 31, if outgoing { 2000 } else { 1000 });
        {
            let mut f = io.facts.borrow_mut();
            f.invitations = vec![Invite {
                id: vec![255, 0],
                name: b"same".to_vec(),
                version: u64::MAX,
            }];
            f.party.as_mut().unwrap().outgoing = vec![vec![254, 0]];
        }
        assert!(c.accept(&io).unwrap());
        let state = c.state();
        assert_eq!(state.kind, if outgoing { 2 } else { 1 });
        assert_eq!(state.version, if outgoing { 0 } else { u64::MAX });
        assert_eq!(
            state.target,
            if outgoing { vec![254, 0] } else { vec![255, 0] }
        );
        assert!(!io.calls.borrow().contains(&"members"));
    }
}
#[test]
fn all_ten_party_actions_bind_only_accept_version_and_cancel_before_unready() {
    for value in -1..11 {
        let c = Controller::from_state(State {
            target: vec![255, 0],
            version: u64::MAX,
            ..State::default()
        });
        let io = Native::new(&c, 31, 42, -33, value);
        assert!(c.accept(&io).unwrap());
        if !(0..10).contains(&value) {
            assert!(io.commands.borrow().is_empty());
            continue;
        }
        let commands = io.commands.borrow();
        assert_eq!(commands.len(), 1);
        assert_eq!(commands[0].value, vec![255, 0]);
        assert_eq!(commands[0].version, if value == 2 { u64::MAX } else { 0 });
        let calls = io.calls.borrow();
        if value == 6 {
            assert!(
                calls.iter().position(|v| *v == "cancel").unwrap()
                    < calls.iter().position(|v| *v == "command").unwrap()
            );
        }
    }
}
#[test]
fn voice_actions_select_resources_and_target_without_replacing_identity() {
    for value in [0, 1, 2, 99] {
        for active in [false, true] {
            let c = Controller::from_state(State {
                target: vec![255, 0, b'v'],
                ..State::default()
            });
            let io = Native::new(&c, 216, 0, -34, value);
            {
                let mut f = io.facts.borrow_mut();
                f.voice_client = true;
                f.voice_party = active;
                f.voice_joined = active;
                f.muted = vec![vec![255, 0, b'v']];
            }
            c.accept(&io).unwrap();
            let effects = io.effects.borrow();
            match value {
                0 => assert_eq!(
                    &effects[..if active { 2 } else { 3 }],
                    if active {
                        &[Ui::VoiceLeave, Ui::VoiceWanted(false)][..]
                    } else {
                        &[Ui::VoiceLeave, Ui::VoiceRecreate, Ui::VoiceJoin][..]
                    }
                ),
                1 => assert_eq!(effects[0], Ui::VoiceMicrophone(true)),
                2 => assert_eq!(effects[0], Ui::VoiceMute(vec![255, 0, b'v'], false)),
                _ => (),
            }
            assert_eq!(
                &effects[effects.len() - 2..],
                &[Ui::VoiceSnapshot, Ui::Back]
            );
            assert_eq!(io.calls.borrow().last(), Some(&"update"));
        }
    }
}
#[test]
fn privacy_command_publishes_exact_pending_binding_before_reentrant_account_update() {
    let c = Controller::default();
    let io = Native::new(&c, 25, 0, -2, 1);
    c.opened_privacy(&io).unwrap();
    assert_eq!(c.state().pending.as_ref().unwrap().command_serial, 0);
    assert!(c.select_privacy(1, &io).unwrap());
    assert_eq!(&*io.calls.borrow(), &["command", "serial", "update"]);
    let pending = c.state().pending.unwrap();
    assert_eq!(pending.command_serial, u64::MAX - 3);
    assert_eq!(pending.mode, 1);
    assert_eq!(
        io.commands.borrow()[0],
        Command {
            action: b"privacy".to_vec(),
            version: u64::MAX - 9,
            privacy: Some(1),
            privacy_revision: u64::MAX - 10,
            privacy_context: vec![255, 0, b'c'],
            ..Command::default()
        }
    );
    {
        let mut f = io.facts.borrow_mut();
        f.command_serial = pending.command_serial;
        f.successful = Some(true);
        let privacy = f.party.as_mut().unwrap().privacy.as_mut().unwrap();
        privacy.mode = 1;
        privacy.revision += 1;
    }
    c.observe_privacy(&io).unwrap();
    assert!(c.state().pending.is_none());
    assert_eq!(io.effects.borrow().last(), Some(&Ui::Back));
}
#[test]
fn privacy_absent_ack_waits_but_explicit_false_rejects_and_clears() {
    for successful in [None, Some(false), Some(true)] {
        let c = Controller::default();
        let io = Native::new(&c, 25, 0, -2, 0);
        c.opened_privacy(&io).unwrap();
        c.select_privacy(0, &io).unwrap();
        {
            let mut f = io.facts.borrow_mut();
            f.command_serial = io.worker_serial;
            f.successful = successful;
            f.party.as_mut().unwrap().privacy.as_mut().unwrap().revision += 1;
        }
        c.observe_privacy(&io).unwrap();
        if successful.is_none() {
            assert!(c.state().pending.is_some());
            assert!(io.effects.borrow().is_empty());
        } else {
            assert!(c.state().pending.is_none());
            assert_eq!(
                matches!(io.effects.borrow().last(), Some(Ui::Back)),
                successful == Some(true)
            );
        }
    }
}
#[test]
fn changed_party_and_stale_frame_do_not_acknowledge_an_unrelated_privacy_editor() {
    for current in [false, true] {
        let c = Controller::default();
        let io = Native::new(&c, 25, 0, -2, 0);
        c.opened_privacy(&io).unwrap();
        {
            let mut f = io.facts.borrow_mut();
            f.party
                .as_mut()
                .unwrap()
                .privacy
                .as_mut()
                .unwrap()
                .context
                .push(7);
            if !current {
                f.frames.last_mut().unwrap().instance += 1;
            }
        }
        c.observe_privacy(&io).unwrap();
        assert!(c.state().pending.is_none());
        assert_eq!(io.effects.borrow().len(), usize::from(current));
    }
}
#[test]
fn privacy_error_precedence_and_getter_failure_preserve_the_existing_pending_state() {
    let c = Controller::default();
    let mut io = Native::new(&c, 25, 0, -2, -1);
    c.opened_privacy(&io).unwrap();
    io.facts.borrow_mut().busy = true;
    assert!(c.select_privacy(-1, &io).unwrap());
    assert!(matches!(
        io.effects.borrow().last(),
        Some(Ui::Unavailable(_))
    ));
    io.facts.borrow_mut().busy = false;
    assert!(c.select_privacy(-1, &io).is_err());
    let before = c.state();
    io.failure = Some("serial");
    assert_eq!(c.select_privacy(2, &io), Err("serial".into()));
    assert_eq!(c.state(), before);
    assert_eq!(io.calls.borrow().last(), Some(&"serial"));
}
