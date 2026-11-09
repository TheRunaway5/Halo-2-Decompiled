use halo2_online_protocol::matchmaking_frontend_postgame::*;
use std::cell::{Cell, RefCell};
struct Device {
    f: RefCell<Facts>,
    calls: RefCell<Vec<String>>,
    callbacks: Vec<bool>,
    held: Cell<Option<u32>>,
    prior_generation: Cell<u32>,
    replace_on_select: Cell<bool>,
    fail: Cell<Option<usize>>,
}
fn report(online: bool, count: Option<u32>) -> Report {
    Report {
        online,
        player_count: count,
        select: true,
    }
}
fn frame(screen: u32, r: Option<Report>) -> Frame {
    Frame { screen, report: r }
}
fn party() -> Party {
    Party {
        leader: false,
        members: vec![
            Member {
                is_self: true,
                postgame: Some(true),
                ..Member::default()
            },
            Member {
                leader: true,
                postgame: Some(false),
                ..Member::default()
            },
        ],
    }
}
impl Default for Device {
    fn default() -> Self {
        Self {
            f: RefCell::new(Facts {
                authenticated: true,
                party: Some(party()),
                mode: 3,
                frames: vec![frame(6, None), frame(14, None)],
                prior: Some(report(true, Some(2))),
                view: Some(report(true, Some(2))),
            }),
            calls: RefCell::new(vec![]),
            callbacks: vec![false, true, true],
            held: Cell::new(None),
            prior_generation: Cell::new(1),
            replace_on_select: Cell::new(false),
            fail: Cell::new(None),
        }
    }
}
impl Device {
    fn observe(&self, name: String) -> Result<()> {
        let _ = host(&self.f.borrow());
        let i = self.calls.borrow().len();
        self.calls.borrow_mut().push(name);
        if self.fail.get() == Some(i) {
            Err("device fault".into())
        } else {
            Ok(())
        }
    }
}
impl Io for Device {
    fn facts(&self) -> Result<Facts> {
        Ok(self.f.borrow().clone())
    }
    fn hold(&self, i: u32) -> Result<bool> {
        self.held.set(Some(i));
        Ok(*self
            .callbacks
            .get(i as usize)
            .ok_or("invalid native report")?)
    }
    fn continue_held(&self) -> Result<()> {
        self.observe(format!("continue:{}", self.held.get().unwrap()))
    }
    fn effect(&self, e: Effect) -> Result<()> {
        self.observe(format!("{e:?}:{}", self.prior_generation.get()))?;
        match e {
            Effect::SelectPrior => {
                if self.replace_on_select.get() {
                    self.prior_generation.set(9);
                }
            }
            Effect::Pop => {
                self.f.borrow_mut().frames.pop();
            }
            Effect::ClearNavigation => self.f.borrow_mut().frames.clear(),
            _ => {}
        }
        Ok(())
    }
}
#[test]
fn unauthenticated_follower_is_still_postgame_host() {
    let mut f = Device::default().f.into_inner();
    f.authenticated = false;
    assert!(host(&f));
}
#[test]
fn one_member_nonleader_is_not_a_follower() {
    let mut f = Device::default().f.into_inner();
    f.party.as_mut().unwrap().members.truncate(1);
    assert!(host(&f));
}
#[test]
fn absent_party_is_host() {
    let f = Facts {
        authenticated: true,
        ..Facts::default()
    };
    assert!(host(&f));
}
#[test]
fn signed_follower_needs_actual_leader_departure() {
    let d = Device::default();
    d.f.borrow_mut().frames[1].report = Some(report(true, Some(2)));
    d.f.borrow_mut().party.as_mut().unwrap().members[1].postgame = None;
    update(&d).unwrap();
    assert!(d.held.get().is_none());
}
#[test]
fn last_duplicate_self_and_leader_observations_are_preserved() {
    let d = Device::default();
    {
        let mut f = d.f.borrow_mut();
        let p = f.party.as_mut().unwrap();
        p.members.push(Member {
            is_self: true,
            postgame: Some(false),
            ..Member::default()
        });
        f.frames[1].report = Some(report(true, Some(2)));
    }
    update(&d).unwrap();
    assert!(d.held.get().is_none());
}
#[test]
fn first_online_report_owns_continuation() {
    let d = Device::default();
    {
        let mut f = d.f.borrow_mut();
        f.frames[0].report = Some(report(false, Some(2)));
        f.frames[1].report = Some(report(true, Some(2)));
        f.frames
            .push(frame(REVIEW_SCREEN, Some(report(true, Some(3)))));
    }
    update(&d).unwrap();
    assert_eq!(d.held.get(), Some(1));
    assert_eq!(&*d.calls.borrow(), &["continue:1"]);
}
#[test]
fn first_online_report_without_callback_does_not_fall_through() {
    let d = Device::default();
    {
        let mut f = d.f.borrow_mut();
        f.frames[0].report = Some(report(true, Some(2)));
        f.frames[1].report = Some(report(true, Some(2)));
    }
    update(&d).unwrap();
    assert_eq!(d.held.get(), Some(0));
    assert!(d.calls.borrow().is_empty());
}
#[test]
fn empty_stack_update_has_no_frame_requirement() {
    let d = Device::default();
    d.f.borrow_mut().frames.clear();
    update(&d).unwrap();
    assert!(d.held.get().is_none());
}
#[test]
fn no_result_bank_or_zero_players_cannot_supply_prior_stats() {
    let mut f = Facts::default();
    for p in [None, Some(report(true, None)), Some(report(true, Some(0)))] {
        f.prior = p;
        assert!(!has_prior(&f));
    }
}
#[test]
fn stats_predicate_preserves_raw_count_and_does_not_invent_online_guard() {
    let f = Facts {
        prior: Some(report(false, Some(u32::MAX))),
        ..Facts::default()
    };
    assert!(has_prior(&f));
}
#[test]
fn non_online_mode_short_circuits_empty_stack() {
    let d = Device::default();
    {
        let mut f = d.f.borrow_mut();
        f.mode = 2;
        f.frames.clear();
    }
    assert!(!review(&d).unwrap());
    assert!(d.calls.borrow().is_empty());
}
#[test]
fn online_empty_stack_reports_missing_owner() {
    let d = Device::default();
    d.f.borrow_mut().frames.clear();
    assert!(review(&d).is_err());
}
#[test]
fn prior_selection_can_replace_owner_before_open() {
    let d = Device::default();
    d.replace_on_select.set(true);
    assert!(review(&d).unwrap());
    assert_eq!(&*d.calls.borrow(), &["SelectPrior:1", "OpenPrior:9"]);
}
#[test]
fn missing_selection_callback_does_not_prevent_actual_prior_open() {
    let d = Device::default();
    d.f.borrow_mut().prior.as_mut().unwrap().select = false;
    assert!(review(&d).unwrap());
    assert_eq!(&*d.calls.borrow(), &["OpenPrior:1"]);
}
#[test]
fn selection_failure_never_opens_another_report() {
    let d = Device::default();
    d.fail.set(Some(0));
    assert_eq!(review(&d).unwrap_err(), "device fault");
    assert_eq!(d.calls.borrow().len(), 1);
}
#[test]
fn review_close_preserves_its_parent_and_advances_revision() {
    let d = Device::default();
    d.f.borrow_mut().frames[1] = frame(REVIEW_SCREEN, Some(report(true, None)));
    close(&d).unwrap();
    assert_eq!(d.f.borrow().frames.len(), 1);
    assert_eq!(&*d.calls.borrow(), &["Pop:1", "Revision:1"]);
}
#[test]
fn single_root_review_cannot_pop_the_last_frame() {
    let d = Device::default();
    d.f.borrow_mut().frames = vec![frame(REVIEW_SCREEN, Some(report(true, Some(1))))];
    close(&d).unwrap();
    assert_eq!(d.f.borrow().frames.len(), 1);
    assert!(d.calls.borrow().is_empty());
}
#[test]
fn offline_review_cannot_close_online_history() {
    let d = Device::default();
    d.f.borrow_mut().frames[1] = frame(REVIEW_SCREEN, Some(report(false, Some(1))));
    close(&d).unwrap();
    assert_eq!(d.f.borrow().frames.len(), 2);
}
#[test]
fn only_online_completed_match_captures_prior() {
    let d = Device::default();
    capture(&d, 185).unwrap();
    assert!(d.calls.borrow().is_empty());
    capture(&d, 16).unwrap();
    assert_eq!(&*d.calls.borrow(), &["CapturePrior:1"]);
}
#[test]
fn capture_nonmatch_id_short_circuits_missing_view() {
    let d = Device::default();
    d.f.borrow_mut().view = None;
    capture(&d, 185).unwrap();
    assert!(capture(&d, 16).is_err());
}
#[test]
fn online_review_keeps_navigation_but_match_replaces_it() {
    let d = Device::default();
    navigation(&d, 185).unwrap();
    assert_eq!(d.f.borrow().frames.len(), 2);
    navigation(&d, 16).unwrap();
    assert!(d.f.borrow().frames.is_empty());
}
#[test]
fn offline_review_replaces_navigation() {
    let d = Device::default();
    d.f.borrow_mut().view = Some(report(false, Some(1)));
    navigation(&d, 185).unwrap();
    assert!(d.f.borrow().frames.is_empty());
}
#[test]
fn continuation_failure_preserves_the_held_selection() {
    let d = Device::default();
    d.f.borrow_mut().frames[1].report = Some(report(true, Some(2)));
    d.fail.set(Some(0));
    assert_eq!(update(&d).unwrap_err(), "device fault");
    assert_eq!(d.held.get(), Some(1));
}
