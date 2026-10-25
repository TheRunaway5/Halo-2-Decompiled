//! CXX ownership for the bandwidth-bounded snapshot stream. Coding, budgets,
//! acknowledgements and pose replication live in the protocol crate.
use crate::ffi;
use halo2_online_protocol::netcode::bandwidth::Profile;
use halo2_online_protocol::netcode::stream::{Frame, Receiver, Sender, SenderConfig};

/// After an unexpected unwind the owner is retired; the native caller falls
/// back to complete snapshots (sender) or waits for a reset (receiver).
pub struct SnapshotSender {
    inner: Sender,
    poisoned: bool,
}

pub struct SnapshotReceiver {
    inner: Receiver,
    chunk: usize,
}

/// `H2_NET_PROFILE`: `original` (default), `broadband`, `unlimited`, or
/// `start[,floor[,ceiling]]` bits per second. Invalid values keep `original`.
/// `complete` disables the stream: every tick sends a complete snapshot.
fn profile() -> Option<Profile> {
    match std::env::var("H2_NET_PROFILE") {
        Ok(text) if text.trim() == "complete" => None,
        Ok(text) => Some(Profile::parse(&text).unwrap_or(Profile::ORIGINAL)),
        Err(_) => Some(Profile::ORIGINAL),
    }
}

pub fn new_snapshot_sender(chunk: usize, overhead: usize) -> Box<SnapshotSender> {
    let profile = profile();
    Box::new(SnapshotSender {
        inner: Sender::new(SenderConfig { profile: profile.unwrap_or(Profile::ORIGINAL), chunk, overhead }),
        poisoned: profile.is_none(),
    })
}

pub fn snapshot_sender_encode(
    sender: &mut SnapshotSender,
    body: &[u8],
    floats: &[u32],
    anchor_offsets: &[u32],
    anchor_keys: &[u64],
    info: &ffi::SnapshotFrameInfo,
) -> Result<ffi::SnapshotEmission, String> {
    if sender.poisoned {
        return Err("Rust snapshot sender is retired".to_owned());
    }
    let viewers: Vec<[f32; 3]> = info.viewers.chunks_exact(3).map(|v| [v[0], v[1], v[2]]).collect();
    let anchors: Vec<(u32, u64)> = anchor_offsets.iter().copied().zip(anchor_keys.iter().copied()).collect();
    let frame = Frame {
        body,
        floats,
        anchors: &anchors,
        poses: info.has_poses.then_some((info.pose_begin, info.pose_end)),
        tick: info.tick,
        sequence: info.sequence,
        now_ms: info.now_ms,
        viewers: &viewers,
        own: &info.own,
        hot: &info.hot,
    };
    let inner = &mut sender.inner;
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| inner.encode(&frame)));
    let emitted = match result {
        Ok(Ok(value)) => value,
        Ok(Err(_)) => return Err("Snapshot exceeds the delta stream bounds".to_owned()),
        Err(_) => {
            sender.poisoned = true;
            return Err("Rust snapshot sender failed".to_owned());
        }
    };
    Ok(match emitted {
        Some(value) => ffi::SnapshotEmission {
            emitted: true,
            keyframe: value.keyframe,
            digest: value.digest,
            view_len: value.view_len,
            packets: value.packets.into_iter().map(|data| ffi::SnapshotPacket { data }).collect(),
        },
        None => ffi::SnapshotEmission { emitted: false, keyframe: false, digest: [0; 32], view_len: 0, packets: Vec::new() },
    })
}

pub fn snapshot_sender_ack(sender: &mut SnapshotSender, latest: u32, mask: u32, flags: u8, now_ms: u64) {
    if sender.poisoned {
        return;
    }
    let inner = &mut sender.inner;
    if std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| inner.ack(latest, mask, flags, now_ms))).is_err() {
        sender.poisoned = true;
    }
}

pub fn snapshot_sender_stats(sender: &SnapshotSender) -> ffi::SnapshotSenderStats {
    let s = sender.inner.stats();
    ffi::SnapshotSenderStats {
        messages: s.messages,
        wire_bytes: s.wire_bytes,
        packets: s.packets,
        keyframes: s.keyframes,
        resets: s.resets,
        over_budget: s.over_budget,
        full_refreshes: s.full_refreshes,
        root_updates: s.root_updates,
        forced_refreshes: s.forced_refreshes,
        held_refreshes: s.held_refreshes,
        lost_messages: s.lost_messages,
        budget_bps: s.budget_bps,
        rtt_ms: s.rtt_ms,
        loss_percent: s.loss_percent,
        retired: sender.poisoned,
    }
}

pub fn new_snapshot_receiver(chunk: usize) -> Box<SnapshotReceiver> {
    Box::new(SnapshotReceiver { inner: Receiver::new(chunk), chunk })
}

fn no_delivery(failed: bool) -> ffi::SnapshotDelivery {
    ffi::SnapshotDelivery { complete: false, failed, sequence: 0, tick: 0, digest: [0; 32], view_len: 0, body: Vec::new() }
}

pub fn snapshot_receiver_push(receiver: &mut SnapshotReceiver, packet: &[u8], now_ms: u64) -> ffi::SnapshotDelivery {
    let inner = &mut receiver.inner;
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| inner.push(packet, now_ms))) {
        Ok(Ok(Some(value))) => ffi::SnapshotDelivery {
            complete: true,
            failed: false,
            sequence: value.sequence,
            tick: value.tick,
            digest: value.digest,
            view_len: value.view_len,
            body: value.body,
        },
        Ok(Ok(None)) => no_delivery(false),
        Ok(Err(_)) => no_delivery(true),
        Err(_) => {
            // Rebuild from scratch; the next ack requests a reset.
            receiver.inner = Receiver::new(receiver.chunk);
            receiver.inner.request_reset();
            no_delivery(true)
        }
    }
}

pub fn snapshot_receiver_commit(receiver: &mut SnapshotReceiver, sequence: u32) -> bool {
    let inner = &mut receiver.inner;
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| inner.commit(sequence))) {
        Ok(committed) => committed,
        Err(_) => {
            receiver.inner = Receiver::new(receiver.chunk);
            receiver.inner.request_reset();
            false
        }
    }
}

pub fn snapshot_receiver_reject(receiver: &mut SnapshotReceiver, sequence: u32) {
    receiver.inner.reject(sequence)
}

pub fn snapshot_receiver_ack(receiver: &SnapshotReceiver) -> ffi::SnapshotAckFields {
    let (latest, mask, flags) = receiver.inner.ack();
    ffi::SnapshotAckFields { latest, mask, flags }
}

pub fn snapshot_receiver_reset(receiver: &mut SnapshotReceiver) {
    receiver.inner.reset();
}

pub fn snapshot_receiver_buffered(receiver: &SnapshotReceiver) -> usize {
    receiver.inner.buffered_bytes()
}

pub fn snapshot_receiver_stats(receiver: &SnapshotReceiver) -> ffi::SnapshotReceiverStats {
    let s = receiver.inner.stats();
    ffi::SnapshotReceiverStats {
        messages: s.messages,
        failures: s.failures,
        held_poses: s.held_poses,
        missing_poses: s.missing_poses,
    }
}
