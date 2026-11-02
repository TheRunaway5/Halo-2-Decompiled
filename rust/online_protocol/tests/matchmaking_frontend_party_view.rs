use halo2_online_protocol::matchmaking_frontend_party::Member;
use halo2_online_protocol::matchmaking_frontend_party_flow::{Controller, State};
use halo2_online_protocol::matchmaking_frontend_party_view::*;
use std::cell::RefCell;
#[derive(Default)]
struct Frame {
    title: Vec<u8>,
    description: Vec<u8>,
    entries: Vec<Entry>,
}
struct Probe {
    f: Facts,
    layout: Layout,
    view: Option<View>,
    members: Vec<Member>,
    seconds: i32,
    matching: bool,
    selected: Vec<u8>,
    stats: bool,
    reads: Vec<(Read, u32, Vec<u8>)>,
    frame: RefCell<Frame>,
    calls: RefCell<Vec<String>>,
    fail: Option<usize>,
}
impl Default for Probe {
    fn default() -> Self {
        Self {
            f: Facts::default(),
            layout: Layout {
                original: false,
                buttons: 2,
            },
            view: None,
            members: vec![],
            seconds: -1,
            matching: false,
            selected: vec![],
            stats: false,
            reads: vec![],
            frame: RefCell::new(Frame::default()),
            calls: RefCell::new(vec![]),
            fail: None,
        }
    }
}
impl Probe {
    fn observe(&self, s: String) -> Result<()> {
        let mut c = self.calls.borrow_mut();
        c.push(s);
        if self.fail == Some(c.len() - 1) {
            Err("native read failed".into())
        } else {
            Ok(())
        }
    }
}
impl Io for Probe {
    fn facts(&self) -> Result<Facts> {
        Ok(self.f.clone())
    }
    fn layout(&self, s: u32, p: u32, d: bool, t: bool) -> Result<Layout> {
        self.observe(format!("layout:{s}:{p}:{d}:{t}"))?;
        Ok(self.layout.clone())
    }
    fn read(&self, o: bool, k: Read, i: u32) -> Result<Vec<u8>> {
        self.observe(format!("read:{o}:{k:?}:{i}"))?;
        Ok(self
            .reads
            .iter()
            .find(|p| p.0 == k && p.1 == i)
            .map_or_else(|| format!("{k:?}:{i}").into_bytes(), |p| p.2.clone()))
    }
    fn members(&self) -> Result<Vec<Member>> {
        self.observe("members".into())?;
        Ok(self.members.clone())
    }
    fn seconds(&self) -> Result<i32> {
        self.observe("seconds".into())?;
        Ok(self.seconds)
    }
    fn matchmaking(&self) -> Result<bool> {
        self.observe("matchmaking".into())?;
        Ok(self.matching)
    }
    fn selected(&self) -> Result<Vec<u8>> {
        self.observe("selected".into())?;
        Ok(self.selected.clone())
    }
    fn view(&self) -> Result<Option<View>> {
        self.observe("view".into())?;
        Ok(self.view.clone())
    }
    fn has_stats(&self) -> Result<bool> {
        self.observe("stats".into())?;
        Ok(self.stats)
    }
    fn start_error(&self) -> Result<Vec<u8>> {
        self.observe("start-error".into())?;
        Ok(vec![0xff, 0, 0x80])
    }
    fn expand(&self, t: &[u8]) -> Result<Vec<u8>> {
        self.observe("expand".into())?;
        Ok([b"expanded:".as_slice(), t].concat())
    }
    fn entries_empty(&self) -> Result<bool> {
        self.observe("empty".into())?;
        Ok(self.frame.borrow().entries.is_empty())
    }
    fn effect(&self, e: Effect) -> Result<()> {
        let name = match &e {
            Effect::ClearDescription => "clear",
            Effect::Title(_) => "title",
            Effect::Description(_) => "description",
            Effect::Append(_) => "append",
            Effect::Replace(_) => "replace",
            Effect::FormatError(_) => "format-error",
        };
        self.observe(name.into())?;
        let mut f = self.frame.borrow_mut();
        match e {
            Effect::ClearDescription => f.description.clear(),
            Effect::Title(t) => f.title = t,
            Effect::Description(t) => f.description = t,
            Effect::Append(e) => f.entries.push(e),
            Effect::Replace(e) => f.entries = e,
            Effect::FormatError(_) => return Err("format-error".into()),
        }
        Ok(())
    }
}
fn old_entry() -> Entry {
    Entry {
        label: b"old".to_vec(),
        destination: 99,
        value: -5,
    }
}
#[test]
fn unknown_screens_clear_description_and_retain_every_other_field() {
    let p = Probe::default();
    *p.frame.borrow_mut() = Frame {
        title: b"retained".to_vec(),
        description: b"clear".to_vec(),
        entries: vec![old_entry()],
    };
    populate(&Controller::default(), &p).unwrap();
    let f = p.frame.borrow();
    assert!(f.description.is_empty());
    assert_eq!(f.title, b"retained");
    assert_eq!(f.entries, vec![old_entry()]);
}
#[test]
fn picker_placeholders_observe_existing_entries_and_keep_source_indices() {
    let mut p = Probe::default();
    p.f.screen = 31;
    p.f.context = 45;
    p.f.friends = vec![
        Friend {
            name: b"skip".to_vec(),
            state: b"Friend".to_vec(),
            ..Friend::default()
        },
        Friend {
            name: vec![0xff, 0],
            state: b"friend".to_vec(),
            presence: b"playing".to_vec(),
        },
    ];
    p.frame.borrow_mut().entries.push(old_entry());
    populate(&Controller::default(), &p).unwrap();
    let f = p.frame.borrow();
    assert_eq!(f.entries.len(), 4);
    assert_eq!(f.entries[1].value, 1);
    assert_eq!(
        f.entries[1].label,
        [vec![0xff, 0], b" - In match".to_vec()].concat()
    );
}
#[test]
fn clan_presence_words_and_roles_do_not_alias_low_bytes() {
    let mut p = Probe::default();
    p.f.screen = 31;
    p.f.context = 45;
    p.f.clan = Some(Clan {
        role: u32::MAX,
        members: [0, 1, 2, 3, 258, u32::MAX]
            .into_iter()
            .map(|presence| ClanMember {
                name: presence.to_string().into_bytes(),
                presence,
                is_self: false,
            })
            .collect(),
    });
    populate(
        &Controller::from_state(State {
            tab: 0x100,
            ..State::default()
        }),
        &p,
    )
    .unwrap();
    let f = p.frame.borrow();
    assert_eq!(f.entries.len(), 4);
    assert_eq!(f.entries[0].value, 2);
    assert_eq!(f.entries[1].value, 3);
}
#[test]
fn lobby_header_commits_before_button_validation_and_replacement() {
    let mut p = Probe::default();
    p.f.screen = 14;
    p.f.party = Some(Party::default());
    p.layout.buttons = 1;
    p.frame.borrow_mut().entries.push(old_entry());
    assert_eq!(
        populate(&Controller::default(), &p),
        Err("format-error".into())
    );
    assert_eq!(p.frame.borrow().title, b"Header:0");
    assert_eq!(p.frame.borrow().entries, vec![old_entry()]);
    assert_eq!(
        *p.calls.borrow(),
        [
            "clear",
            "layout:14:2:false:true",
            "read:false:Header:0",
            "title",
            "format-error"
        ]
    );
}
#[test]
fn failed_second_button_read_retains_the_old_entries() {
    let mut p = Probe::default();
    p.f.screen = 14;
    p.f.party = Some(Party::default());
    p.fail = Some(5);
    p.frame.borrow_mut().entries.push(old_entry());
    assert!(populate(&Controller::default(), &p).is_err());
    assert_eq!(p.frame.borrow().title, b"Header:0");
    assert_eq!(p.frame.borrow().entries, vec![old_entry()]);
    assert_eq!(p.calls.borrow().last().unwrap(), "read:false:Button:1");
}
#[test]
fn roster_uses_actual_expanded_seats_once_and_disables_only_minus_two() {
    let mut p = Probe::default();
    p.f.screen = 26;
    p.f.party = Some(Party::default());
    p.members = [-2, -1, 0, i32::MIN, i32::MAX]
        .into_iter()
        .map(|controller| Member {
            controller,
            name: b"member".to_vec(),
            ..Member::default()
        })
        .collect();
    populate(&Controller::default(), &p).unwrap();
    let f = p.frame.borrow();
    assert_eq!(
        f.entries.iter().map(|e| e.destination).collect::<Vec<_>>(),
        [-82, 31, 31, 31, 31]
    );
    assert_eq!(
        p.calls.borrow().iter().filter(|c| *c == "members").count(),
        1
    );
}
#[test]
fn duplicate_invite_ids_choose_the_first_without_erasing_the_cached_frame_on_miss() {
    let mut p = Probe::default();
    p.f.screen = 31;
    p.f.invitations = vec![
        Named {
            id: vec![0xff, 0],
            name: b"first".to_vec(),
        },
        Named {
            id: vec![0xff, 0],
            name: b"second".to_vec(),
        },
    ];
    let flow = Controller::from_state(State {
        target: vec![0xff, 0],
        kind: 1,
        ..State::default()
    });
    populate(&flow, &p).unwrap();
    assert_eq!(p.frame.borrow().title, b"first");
    p.f.invitations.clear();
    *p.frame.borrow_mut() = Frame {
        entries: vec![old_entry()],
        ..Frame::default()
    };
    populate(&flow, &p).unwrap();
    assert_eq!(p.frame.borrow().entries, vec![old_entry()]);
}
#[test]
fn popup_uses_primary_member_identity_and_preserves_existing_rows_for_nonleaders() {
    let mut p = Probe::default();
    p.f.screen = 31;
    p.f.voice_party = true;
    p.f.voice_joined = true;
    p.f.muted = vec![vec![0xff, 0]];
    p.f.party = Some(Party {
        members: vec![Member {
            handle: vec![0xff, 0],
            name: b"member".to_vec(),
            ..Member::default()
        }],
        ..Party::default()
    });
    p.frame.borrow_mut().entries.push(old_entry());
    populate(
        &Controller::from_state(State {
            target: vec![0xff, 0],
            ..State::default()
        }),
        &p,
    )
    .unwrap();
    assert_eq!(p.frame.borrow().entries[0], old_entry());
    assert_eq!(p.frame.borrow().entries[1].label, b"Unmute Voice");
}
#[test]
fn empty_known_countdown_status_is_localized_and_expanded_before_other_queries() {
    let mut p = Probe::default();
    p.f.screen = 14;
    p.seconds = 0;
    p.view = Some(View::default());
    assert_eq!(text(0, &p).unwrap(), b"expanded:Localized:0");
    assert_eq!(
        *p.calls.borrow(),
        [
            "layout:14:2:false:true",
            "seconds",
            "view",
            "read:false:Localized:0",
            "expand"
        ]
    );
}
#[test]
fn selected_and_error_queries_are_guarded_and_late() {
    let mut p = Probe::default();
    p.f.screen = 14;
    p.matching = true;
    p.f.party = Some(Party {
        countdown_supported: false,
        ..Party::default()
    });
    p.selected = vec![0xff, 0];
    assert_eq!(text(0, &p).unwrap(), [0xff, 0, 0x80]);
    assert_eq!(
        *p.calls.borrow(),
        [
            "layout:14:2:false:true",
            "seconds",
            "matchmaking",
            "selected",
            "start-error"
        ]
    );
    p.f.party = None;
    p.calls.borrow_mut().clear();
    text(0, &p).unwrap();
    assert!(!p.calls.borrow().contains(&"selected".into()));
}
#[test]
fn leader_replacement_uses_first_raw_token_and_never_expands() {
    let mut p = Probe::default();
    p.f.screen = 14;
    p.f.party = Some(Party {
        members: vec![
            Member {
                leader: true,
                name: vec![0xff, 0],
                ..Member::default()
            },
            Member {
                leader: true,
                name: b"second".to_vec(),
                ..Member::default()
            },
        ],
        ..Party::default()
    });
    p.reads.push((
        Read::Authored,
        43,
        vec![0xee, 0x90, 0x96, b'/', 0xee, 0x90, 0x96],
    ));
    assert_eq!(text(43, &p).unwrap(), [0xff, 0, b'/', 0xee, 0x90, 0x96]);
    assert!(!p.calls.borrow().contains(&"expand".into()));
}
#[test]
fn roster_text_prepares_the_direct_owner_even_for_constant_or_unknown_indices() {
    let mut p = Probe::default();
    p.f.screen = 26;
    assert_eq!(text(0, &p).unwrap(), b"PARTY");
    assert_eq!(*p.calls.borrow(), ["layout:26:2:true:false"]);
    p.fail = Some(1);
    assert!(text(u32::MAX, &p).is_err());
}
#[test]
fn prior_statistics_read_is_lazy_and_unexpanded() {
    let mut p = Probe::default();
    p.f.screen = 14;
    p.stats = true;
    assert_eq!(text(45, &p).unwrap(), b"Authored:45");
    assert_eq!(
        *p.calls.borrow(),
        ["layout:14:2:false:true", "stats", "read:false:Authored:45"]
    );
}
#[test]
fn help_uses_matchmaking_session_six_and_privacy_is_not_expanded() {
    let mut p = Probe::default();
    p.f.screen = 14;
    p.view = Some(View {
        protocol: 2,
        machines: i32::MIN,
        ..View::default()
    });
    assert_eq!(
        text(47, &p).unwrap(),
        format!("Localized:{}", 0x16001827u32).into_bytes()
    );
    assert!(!p.calls.borrow().contains(&"expand".into()));
}
#[test]
fn picker_and_fallback_text_retain_opaque_bytes() {
    let mut p = Probe::default();
    p.f.screen = 31;
    p.f.context = 45;
    p.f.description = vec![0xff, 0, 0x80];
    assert_eq!(text(0, &p).unwrap(), p.f.description);
    assert!(p.calls.borrow().is_empty());
    p.f.screen = 9;
    p.f.party_message = vec![0xff, 0];
    p.f.account_message = b"account".to_vec();
    assert_eq!(text(0, &p).unwrap(), [0xff, 0]);
    assert!(text(u32::MAX, &p).unwrap().is_empty());
}
