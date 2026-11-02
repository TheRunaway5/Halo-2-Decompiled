use halo2_online_protocol::matchmaking_frontend_lobby::*;
fn facts() -> Facts {
    Facts {
        mode: 3,
        screens: vec![6, 186, 14],
        account_client: true,
        authenticated: true,
        queue_client: true,
        selected_playlist: "ranked".into(),
        playlists: vec![Playlist {
            id: "ranked".into(),
            revision: "v7".into(),
            eligible: true,
        }],
        party: Some(Party {
            leader: true,
            countdown_supported: true,
            members: vec![
                Member {
                    handle: "self".into(),
                    ready_playlist: "ranked".into(),
                },
                Member {
                    handle: "peer".into(),
                    ready_playlist: "ranked".into(),
                },
            ],
            ..Party::default()
        }),
        ..Facts::default()
    }
}
fn publish(f: &mut Facts, serial: u64, seconds: Option<i32>, complete: bool) {
    f.party.as_mut().unwrap().countdown = Some(Countdown {
        serial,
        seconds,
        complete,
    });
}
fn lobby() -> Lobby {
    let mut l = Lobby::default();
    l.enter(&facts());
    l
}
#[test]
fn start_requires_every_real_accepted_party_fact_in_original_priority() {
    let l = lobby();
    let good = facts();
    assert_eq!(l.start_error(&good), "");
    let mut f = good.clone();
    f.account_client = false;
    assert!(l.start_error(&f).starts_with("Sign in"));
    f = good.clone();
    f.authenticated = false;
    f.party = None;
    assert!(l.start_error(&f).starts_with("Sign in"));
    f = good.clone();
    f.party = None;
    assert!(l.start_error(&f).starts_with("Create or join"));
    f = good.clone();
    f.party.as_mut().unwrap().leader = false;
    f.selected_playlist.clear();
    assert!(l.start_error(&f).starts_with("Only the party"));
    f = good.clone();
    f.selected_playlist.clear();
    f.queue_client = false;
    assert!(l.start_error(&f).starts_with("Choose a matchmaking"));
    f = good.clone();
    f.queue_client = false;
    f.busy = true;
    assert!(l.start_error(&f).starts_with("No matchmaking"));
    f = good.clone();
    f.party.as_mut().unwrap().countdown_supported = false;
    f.busy = true;
    assert!(l.start_error(&f).starts_with("This ONLINE service"));
    f = good.clone();
    f.busy = true;
    f.playlists.clear();
    assert!(l.start_error(&f).starts_with("Wait for"));
    f = good.clone();
    f.playlists[0].eligible = false;
    assert!(l.start_error(&f).starts_with("The selected playlist"));
    f = good.clone();
    f.party.as_mut().unwrap().members[1].ready_playlist = "other".into();
    assert!(l.start_error(&f).starts_with("Every party"));
    let l = Lobby::default();
    assert!(l.start_error(&good).starts_with("Choose a matchmaking"));
}
#[test]
fn actual_start_submission_must_succeed_before_refresh() {
    let mut l = lobby();
    let f = facts();
    let p = l.plan(Input::Accept(-17), &f);
    assert_eq!(
        (p.command, p.playlist.as_str(), p.revision.as_str()),
        (Command::Start, "ranked", "v7")
    );
    let refused = l.finish_input(p.clone(), false);
    assert!(refused.sound && !refused.bounded_sound && !refused.refresh_account);
    assert!(refused.error.starts_with("The ONLINE operation changed"));
    let ok = l.finish_input(p, true);
    assert!(ok.refresh_account && !ok.sound);
    assert_eq!(l.seconds(), -1);
}
#[test]
fn active_countdown_intercepts_accept_even_on_game_setup_focus() {
    let mut l = lobby();
    let mut f = facts();
    assert!(!l.plan(Input::Accept(23), &f).handled);
    publish(&mut f, 1, Some(8), false);
    l.observe(&f);
    assert_eq!(l.plan(Input::Accept(23), &f).command, Command::Start);
    f.screens.push(23);
    assert!(!l.plan(Input::Accept(-17), &f).handled);
}
#[test]
fn countdown_observes_service_seconds_without_a_local_timer() {
    let mut l = lobby();
    let mut f = facts();
    publish(&mut f, 7, Some(10), false);
    assert!(l.observe(&f).revision);
    for _ in 0..100 {
        assert_eq!(l.observe(&f), Observation::default());
    }
    assert_eq!(l.seconds(), 10);
    publish(&mut f, 7, None, false);
    assert!(l.observe(&f).revision);
    assert_eq!(l.seconds(), -1);
}
#[test]
fn each_new_stage_beeps_once_and_zero_consumes_a_serial_once() {
    let mut l = lobby();
    let mut f = facts();
    for s in [10, 3, 2, 1] {
        publish(&mut f, 9, Some(s), false);
        let o = l.observe(&f);
        assert_eq!(o.beep, s <= 3);
        assert!(!o.search);
        assert_eq!(l.observe(&f), Observation::default());
    }
    publish(&mut f, 9, Some(0), true);
    let o = l.observe(&f);
    assert!(o.beep && o.search && o.revision);
    assert_eq!(l.seconds(), -1);
    assert_eq!(l.observe(&f), Observation::default());
    publish(&mut f, 10, Some(0), true);
    assert!(l.observe(&f).search);
}
#[test]
fn entering_a_fresh_lobby_consumes_an_already_completed_publication() {
    let mut f = facts();
    publish(&mut f, 88, Some(0), true);
    let mut l = Lobby::default();
    l.enter(&f);
    assert!(!l.observe(&f).search);
    assert_eq!(l.seconds(), -1);
    publish(&mut f, 89, None, true);
    assert!(l.observe(&f).search);
}
#[test]
fn actual_departure_has_one_notice_but_parameter_cancel_does_not() {
    let mut l = lobby();
    let mut f = facts();
    publish(&mut f, 2, Some(7), false);
    l.observe(&f);
    f.party.as_mut().unwrap().countdown = None;
    assert!(!l.observe(&f).departure);
    assert_eq!(l.seconds(), -1);
    publish(&mut f, 3, Some(7), false);
    l.observe(&f);
    f.party.as_mut().unwrap().countdown = None;
    f.party.as_mut().unwrap().members.pop();
    let o = l.observe(&f);
    assert!(o.departure && o.revision && !o.search);
    assert!(!l.observe(&f).departure);
}
#[test]
fn unsupported_service_lost_party_and_leaving_lobby_clear_countdown() {
    for which in 0..3 {
        let mut l = lobby();
        let mut f = facts();
        publish(&mut f, 3, Some(2), false);
        l.observe(&f);
        match which {
            0 => f.party = None,
            1 => f.party.as_mut().unwrap().countdown_supported = false,
            _ => f.mode = 2,
        }
        let o = l.observe(&f);
        assert!(o.revision && !o.departure && !o.search);
        assert_eq!(l.seconds(), -1);
    }
}
#[test]
fn leader_back_cancels_and_follower_back_requests_before_confirmation() {
    let mut l = lobby();
    let mut f = facts();
    publish(&mut f, 2, Some(3), false);
    l.observe(&f);
    let p = l.plan(Input::Back, &f);
    assert_eq!(p.command, Command::Cancel);
    let e = l.finish_input(p, false);
    assert!(e.sound && !e.confirm_leave && e.error.is_empty());
    f.party.as_mut().unwrap().leader = false;
    let p = l.plan(Input::Back, &f);
    assert_eq!(p.command, Command::Request);
    let e = l.finish_input(p, false);
    assert!(e.confirm_leave && !e.sound);
    f.account_client = false;
    let p = l.plan(Input::Back, &f);
    assert_eq!(p.command, Command::None);
    assert!(l.finish_input(p, true).confirm_leave);
}
#[test]
fn auxiliary_routes_to_friends_or_actual_cancel_request() {
    let mut l = lobby();
    let mut f = facts();
    let p = l.plan(Input::Auxiliary, &f);
    assert!(l.finish_input(p, true).friends);
    publish(&mut f, 2, Some(3), false);
    l.observe(&f);
    assert_eq!(l.plan(Input::Auxiliary, &f).command, Command::Cancel);
    f.party.as_mut().unwrap().leader = false;
    assert_eq!(l.plan(Input::Auxiliary, &f).command, Command::Request);
    f.account_client = false;
    let p = l.plan(Input::Auxiliary, &f);
    let e = l.finish_input(p, true);
    assert!(e.sound && e.bounded_sound && !e.friends);
}
#[test]
fn leave_refusal_retains_actual_party_and_confirmation() {
    let mut l = lobby();
    let mut f = facts();
    let p = l.plan(Input::BeginLeave, &f);
    assert_eq!(p.command, Command::Leave);
    let e = l.finish_input(p.clone(), false);
    assert!(e.sound && !e.begin_leave && !l.leave_pending());
    assert!(l.finish_input(p, true).begin_leave);
    assert!(l.leave_pending());
    assert_eq!(l.plan(Input::BeginLeave, &f).command, Command::None);
    f.busy = true;
    f.party = None;
    assert!(!l.observe_leave(&f));
    f.busy = false;
    assert!(l.observe_leave(&f));
    assert!(!l.leave_pending());
    // Native sign-out is performed before this teardown commit.
    assert!(l.matchmaking());
    l.left();
    assert!(!l.matchmaking());
}
#[test]
fn completed_leave_rejection_clears_only_pending_intent() {
    let mut l = lobby();
    let mut f = facts();
    let p = l.plan(Input::BeginLeave, &f);
    l.finish_input(p, true);
    assert!(!l.observe_leave(&f) && l.leave_pending());
    f.party_message_present = true;
    assert!(!l.observe_leave(&f));
    assert!(!l.leave_pending() && l.matchmaking());
}
#[test]
fn pending_leave_fences_countdown_and_clone_has_independent_state() {
    let mut l = lobby();
    let mut f = facts();
    publish(&mut f, 3, Some(2), false);
    l.observe(&f);
    let p = l.plan(Input::BeginLeave, &f);
    l.finish_input(p, true);
    publish(&mut f, 3, Some(0), true);
    assert_eq!(l.observe(&f), Observation::default());
    assert_eq!(l.seconds(), 2);
    let mut copy = l.clone();
    copy.left();
    assert_eq!(l.seconds(), 2);
    assert!(l.matchmaking());
    assert_eq!(copy.seconds(), -1);
}
