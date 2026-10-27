//! Original party-A countdown leaves, with authentication and session states
//! owned by the caller. Unchanged TU1.5 controls are retained in
//! tests/fixtures/tu15_party_countdown_original.json (40 controls).

/// 19A78E / TU1.5 194FBD: ready state9 (-1) starts10; active state10
/// subtracts one only above3. The caller proves the original status/host gate.
pub fn press(seconds: i32) -> i32 {
    if seconds == -1 { 10 } else if seconds > 3 { seconds - 1 } else { seconds }
}

/// 6E360 / TU1.5 71D70 uses a signed elapsed comparison and publishes at most
/// one lower value. Publication stores the current clock; no catch-up occurs.
pub fn tick(seconds: i32, elapsed_ms: u32) -> i32 {
    if seconds > 0 && (elapsed_ms as i32) >= 1000 { seconds - 1 } else { seconds }
}
