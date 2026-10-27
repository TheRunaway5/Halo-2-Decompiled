//! Original party privacy leaves. The service owns authenticated membership,
//! invitations, actual transport state and capacity; these inputs grant none.
//! Retail19A148/19A161 and6F2B0 correspond to unchanged TU1.5
//! 194936/19494F and72EE0. Source controls: privacy-join-original-controls.json.

pub fn mode(value: i32) -> i32 {
    match value { 0 => 0, 1 => 1, _ => 2 }
}

/// Original5BDE0 requires an established session and its actual party leader;
/// 5DAC0 admits the same leader through an authenticated remote peer channel.
pub fn can_change(established: bool, leader: bool) -> bool {
    established && leader
}

/// Privacy/capacity branch of6F2B0, after version/protocol and actual logon
/// admission and before the independent joining-state override.7 means full,
/// 8 means privacy denied. Closed never has an invitation bypass.
pub fn admission(privacy: i32, invited: bool, count: i32,
                 public_open: i32, private_open: i32) -> i32 {
    let available = match privacy {
        0 => public_open,
        1 if invited => private_open,
        _ => return 8,
    };
    if count > available { 7 } else { 0 }
}
