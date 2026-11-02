//! Original error controls and declared native transport observations.
use halo2_online_protocol::matchmaking_readiness::{
    NativeLaunchObservation, Seat, error_clear, error_dialog, native_launch_ready,
};
use serde_json::Value;

#[test]
fn unchanged_tu15_error_selector_and_state_clear() {
    let fixture: Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/tu15_matchmaking_errors_original.json"
    ))
    .unwrap();
    assert_eq!(
        fixture["source_sha256"],
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
    );
    assert_eq!(fixture["all_executed_instruction_bytes_verified"], true);
    let rows = fixture["selector_controls"].as_array().unwrap();
    assert_eq!(rows.len(), 50);
    for row in rows {
        let expected = row["dialog"].as_u64().map(|v| v as u32);
        assert_eq!(
            error_dialog(
                row["state"].as_i64().unwrap() as i32,
                row["local_cancel"].as_bool().unwrap()
            ),
            expected
        );
    }
    let rows = fixture["state_clear_controls"].as_array().unwrap();
    assert_eq!(rows.len(), 5);
    for row in rows {
        let state = row["input_state"].as_i64().unwrap() as i32;
        let clear = error_clear(
            row["local_host"].as_bool().unwrap(),
            state,
            row["shown"].as_u64().unwrap() as u32,
            row["now"].as_u64().unwrap() as u32,
        );
        assert_eq!(
            if clear { 0 } else { state },
            row["result_state"].as_i64().unwrap() as i32
        );
    }
}

fn ready() -> NativeLaunchObservation {
    NativeLaunchObservation {
        session_present: true,
        transferred: false,
        loading: true,
        start_supported: true,
        start_ms: Some(0),
        team_mode: Some(true),
        current_peer: 0,
    }
}

#[test]
fn native_handoff_requires_transport_and_observed_countdown_zero() {
    let seats = [
        Seat {
            occupied: true,
            peer: 0,
            team: 0,
        },
        Seat {
            occupied: true,
            peer: 1,
            team: 1,
        },
    ];
    assert!(native_launch_ready(ready(), &seats));
    for observation in [
        NativeLaunchObservation {
            session_present: false,
            ..ready()
        },
        NativeLaunchObservation {
            transferred: true,
            ..ready()
        },
        NativeLaunchObservation {
            loading: false,
            ..ready()
        },
        NativeLaunchObservation {
            start_supported: false,
            ..ready()
        },
        NativeLaunchObservation {
            start_ms: None,
            ..ready()
        },
        NativeLaunchObservation {
            start_ms: Some(1),
            ..ready()
        },
        NativeLaunchObservation {
            start_ms: Some(10_000),
            ..ready()
        },
        NativeLaunchObservation {
            start_ms: Some(u32::MAX),
            ..ready()
        },
    ] {
        assert!(!native_launch_ready(observation, &seats), "{observation:?}");
    }
}

#[test]
fn native_handoff_preserves_seat_ownership_and_legacy_ffa() {
    let mut seats = vec![
        Seat {
            occupied: true,
            peer: 0,
            team: 0,
        },
        Seat {
            occupied: true,
            peer: 0,
            team: 0,
        },
        Seat {
            occupied: true,
            peer: 2,
            team: 1,
        },
    ];
    assert!(native_launch_ready(ready(), &seats));
    seats[2].occupied = false;
    assert!(!native_launch_ready(ready(), &seats));
    seats[2].occupied = true;
    seats[2].team = 0;
    assert!(!native_launch_ready(ready(), &seats));
    for team_mode in [None, Some(false)] {
        assert!(native_launch_ready(
            NativeLaunchObservation {
                team_mode,
                ..ready()
            },
            &seats
        ));
    }
    seats[2].team = -1;
    assert!(!native_launch_ready(ready(), &seats));
    seats[2].team = 1;
    assert!(!native_launch_ready(
        NativeLaunchObservation {
            current_peer: u32::MAX,
            ..ready()
        },
        &seats
    ));
    seats[2].peer = u32::MAX;
    assert!(!native_launch_ready(ready(), &seats));
    assert!(!native_launch_ready(ready(), &[]));
    assert!(!native_launch_ready(
        ready(),
        &[Seat {
            occupied: true,
            peer: 0,
            team: 0
        }; 17]
    ));
}

#[test]
fn native_error_expiry_retains_unsigned_clock_boundaries() {
    assert!(!error_clear(true, 1, 0, u32::MAX));
    assert!(!error_clear(true, 0, 100, 3100));
    assert!(!error_clear(false, 1, 100, 3100));
    for shown in [1u32, 100, 0xffff_fff0] {
        for delta in [0u32, 2999, 3000, 3001, u32::MAX] {
            assert_eq!(
                error_clear(true, 1, shown, shown.wrapping_add(delta)),
                delta >= 3000
            );
        }
    }
}
