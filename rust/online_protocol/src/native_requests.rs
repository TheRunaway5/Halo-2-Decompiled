//! Complete native host-bound request decoding, shared by matched and raw ingress.
//! Profiles and object datums are owned data, never native pointer authority.

use crate::DecodeError;

pub const MAX_NATIVE_PACKET_BYTES: usize = 1400;
pub const PROFILE_BYTES: usize = 0x90;
pub const MAX_LOCAL_PLAYERS: usize = 4;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct LocalPlayer {
    pub team: i8,
    pub profile: Option<[u8; PROFILE_BYTES]>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct VehicleIntent {
    pub action: i16,
    pub index: i16,
    pub object: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub struct InputControl {
    pub forward: f32,
    pub right: f32,
    pub yaw: f32,
    pub pitch: f32,
    pub flags: u16,
    pub zoom: i16,
    pub zoom_reset_serial: u32,
    pub pickup_milliseconds: u16,
    pub dual_wield_milliseconds: u16,
    pub completed: [u16; 2],
    pub presses: [u32; 10],
    pub vehicle: Option<VehicleIntent>,
    pub completed_vehicle: Option<VehicleIntent>,
}

/// Snapshot stream acknowledgement carried by every input packet: the newest
/// decoded snapshot sequence, the 32 before it as a bit mask, and flags
/// (bit 0 requests a self-contained reset).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct SnapshotAck {
    pub latest: u32,
    pub mask: u32,
    pub flags: u8,
}

/// Displayed snapshot time of a non-host input (protocol 113): the low 16 bits
/// of the older interpolation endpoint's host tick and the blend toward the
/// next tick in 1/255 steps (0..=254). Lag-compensated hosts rewind hitscan
/// targets to it. On the wire, fraction 255 with tick 0 means no view.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct SnapshotView {
    pub tick: u16,
    pub fraction: u8,
}

/// Wire fraction byte that marks an input without a displayed snapshot.
pub const VIEW_ABSENT_FRACTION: u8 = 255;

/// Predicted motion of a non-host's own biped after the input it travels with
/// (protocol 113, NETCODE_ENHANCED_GOAL item 1). Position is absolute in
/// 1/1024 world units (signed 24-bit), velocity 1/256 world units per second.
/// `baseline_age` is the input sequence minus the acknowledged input whose
/// authoritative motion the prediction restarted from (255: unknown).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct MotionReport {
    pub grounded: bool,
    pub crouching: bool,
    pub baseline_age: u8,
    pub position: [i32; 3],
    pub velocity: [i16; 3],
}

/// Motion block flags: bit 0 present, bit 1 grounded, bit 2 crouching. An
/// absent block is exactly one zero byte.
pub const MOTION_PRESENT: u8 = 1;
pub const MOTION_GROUNDED: u8 = 2;
pub const MOTION_CROUCHING: u8 = 4;
/// Signed 24-bit range of motion positions.
pub const MOTION_POSITION_LIMIT: i32 = (1 << 23) - 1;

/// Held per-view fields of an earlier input, repeated for loss tolerance
/// (item 5). Press serials, completions, intents and the motion report are
/// cumulative or latest state and travel only with the newest input (the
/// host adopts only the newest report).
#[derive(Clone, Debug, PartialEq)]
pub struct RedundantControl {
    pub forward: f32,
    pub right: f32,
    pub yaw: f32,
    pub pitch: f32,
    pub flags: u16,
    pub zoom: i16,
}

/// One earlier input: `sequence` is the newest input's sequence minus 1, 2...
#[derive(Clone, Debug, PartialEq)]
pub struct RedundantInput {
    pub sequence: u32,
    pub controls: Vec<RedundantControl>,
}

/// Earlier inputs a packet may repeat.
pub const MAX_REDUNDANT_INPUTS: usize = 2;
/// Redundant field mask: forward, right, yaw, pitch, flags, zoom. A set bit
/// carries a value that differs (bitwise) from the next newer input's; bits 6
/// and 7 are reserved and must be zero.
pub const REDUNDANT_FIELDS: u8 = 0x3f;

#[derive(Clone, Debug, PartialEq)]
pub enum NativeRequest {
    Hello {
        nonce: u64,
        fingerprint: u64,
        players: Vec<LocalPlayer>,
    },
    Input {
        nonce: u64,
        cookie: u64,
        sequence: u32,
        controls: Vec<InputControl>,
        ack: SnapshotAck,
        view: Option<SnapshotView>,
        /// One entry per control, in the same order.
        motion: Vec<Option<MotionReport>>,
        /// Newest first: sequence - 1, sequence - 2.
        redundant: Vec<RedundantInput>,
    },
    Leave {
        nonce: u64,
        cookie: u64,
    },
    SessionQuery {
        nonce: u64,
    },
}

struct Reader<'a> {
    packet: &'a [u8],
    at: usize,
}

impl Reader<'_> {
    fn take<const N: usize>(&mut self) -> Result<[u8; N], DecodeError> {
        let end = self.at.checked_add(N).ok_or(DecodeError::InvalidLength)?;
        let bytes = self.packet.get(self.at..end).ok_or(DecodeError::TooShort)?;
        let value = bytes.try_into().map_err(|_| DecodeError::InvalidLength)?;
        self.at = end;
        Ok(value)
    }
    fn u8(&mut self) -> Result<u8, DecodeError> {
        Ok(self.take::<1>()?[0])
    }
    fn u16(&mut self) -> Result<u16, DecodeError> {
        Ok(u16::from_le_bytes(self.take()?))
    }
    fn u32(&mut self) -> Result<u32, DecodeError> {
        Ok(u32::from_le_bytes(self.take()?))
    }
    fn u64(&mut self) -> Result<u64, DecodeError> {
        Ok(u64::from_le_bytes(self.take()?))
    }
    fn boolean(&mut self) -> Result<bool, DecodeError> {
        crate::boolean(self.u8()?)
    }
    fn count(&mut self) -> Result<usize, DecodeError> {
        let value = usize::from(self.u8()?);
        if !(1..=MAX_LOCAL_PLAYERS).contains(&value) {
            return Err(DecodeError::InvalidCount);
        }
        Ok(value)
    }
    fn real(&mut self, limit: f32) -> Result<f32, DecodeError> {
        let value = f32::from_bits(self.u32()?);
        if !value.is_finite() || value.abs() > limit {
            return Err(DecodeError::InvalidControl);
        }
        Ok(value)
    }
    fn intent(&mut self) -> Result<Option<VehicleIntent>, DecodeError> {
        if !self.boolean()? {
            return Ok(None);
        }
        let value = VehicleIntent {
            action: i16::from_le_bytes(self.take()?),
            index: i16::from_le_bytes(self.take()?),
            object: self.u32()?,
        };
        if if value.action == 0 {
            value.index != 0 || value.object != u32::MAX
        } else {
            !(4..=6).contains(&value.action)
                || !(0..64).contains(&value.index)
                || value.object == u32::MAX
        } {
            return Err(DecodeError::InvalidIntent);
        }
        Ok(Some(value))
    }
    fn motion(&mut self) -> Result<Option<MotionReport>, DecodeError> {
        let flags = self.u8()?;
        if flags & !(MOTION_PRESENT | MOTION_GROUNDED | MOTION_CROUCHING) != 0 {
            return Err(DecodeError::InvalidControl);
        }
        if flags & MOTION_PRESENT == 0 {
            // Absent has exactly one encoding.
            if flags != 0 {
                return Err(DecodeError::InvalidControl);
            }
            return Ok(None);
        }
        let baseline_age = self.u8()?;
        let mut position = [0i32; 3];
        for value in &mut position {
            let bytes = self.take::<3>()?;
            *value = i32::from_le_bytes([bytes[0], bytes[1], bytes[2], 0]) << 8 >> 8;
        }
        let mut velocity = [0i16; 3];
        for value in &mut velocity {
            *value = i16::from_le_bytes(self.take()?);
        }
        Ok(Some(MotionReport {
            grounded: flags & MOTION_GROUNDED != 0,
            crouching: flags & MOTION_CROUCHING != 0,
            baseline_age,
            position,
            velocity,
        }))
    }
    // One redundant view: masked fields over the next newer input's values.
    fn redundant(&mut self, newer: &RedundantControl) -> Result<RedundantControl, DecodeError> {
        let mask = self.u8()?;
        if mask & !REDUNDANT_FIELDS != 0 {
            return Err(DecodeError::InvalidControl);
        }
        let mut value = newer.clone();
        let changed = |differs: bool| {
            if differs {
                Ok(())
            } else {
                Err(DecodeError::InvalidControl)
            }
        };
        if mask & 1 != 0 {
            value.forward = self.real(1.0)?;
            changed(value.forward.to_bits() != newer.forward.to_bits())?;
        }
        if mask & 2 != 0 {
            value.right = self.real(1.0)?;
            changed(value.right.to_bits() != newer.right.to_bits())?;
        }
        if mask & 4 != 0 {
            value.yaw = self.real(10000.0)?;
            changed(value.yaw.to_bits() != newer.yaw.to_bits())?;
        }
        if mask & 8 != 0 {
            value.pitch = self.real(10000.0)?;
            changed(value.pitch.to_bits() != newer.pitch.to_bits())?;
        }
        if mask & 16 != 0 {
            value.flags = self.u16()?;
            if value.flags > 2047 {
                return Err(DecodeError::InvalidControl);
            }
            changed(value.flags != newer.flags)?;
        }
        if mask & 32 != 0 {
            value.zoom = i16::from_le_bytes(self.take()?);
            changed(value.zoom != newer.zoom)?;
        }
        Ok(value)
    }
    fn control(&mut self) -> Result<InputControl, DecodeError> {
        let forward = self.real(1.0)?;
        let right = self.real(1.0)?;
        let yaw = self.real(10000.0)?;
        let pitch = self.real(10000.0)?;
        let flags = self.u16()?;
        if flags > 2047 {
            return Err(DecodeError::InvalidControl);
        }
        let zoom = i16::from_le_bytes(self.take()?);
        let zoom_reset_serial = self.u32()?;
        let pickup_milliseconds = self.u16()?;
        let dual_wield_milliseconds = self.u16()?;
        let completed = [self.u16()?, self.u16()?];
        let mut presses = [0; 10];
        for value in &mut presses {
            *value = self.u32()?;
        }
        Ok(InputControl {
            forward,
            right,
            yaw,
            pitch,
            flags,
            zoom,
            zoom_reset_serial,
            pickup_milliseconds,
            dual_wield_milliseconds,
            completed,
            presses,
            vehicle: self.intent()?,
            completed_vehicle: self.intent()?,
        })
    }
}

/// Decodes only native host-bound requests, not kind-6 ONLINE voice or snapshots.
/// The caller selects this format from its actual receive route. All bytes are
/// consumed before a result is published; local policy supplies the version.
pub fn decode_request(packet: &[u8], protocol: u16) -> Result<NativeRequest, DecodeError> {
    if packet.len() > MAX_NATIVE_PACKET_BYTES {
        return Err(DecodeError::TooLarge);
    }
    let mut reader = Reader { packet, at: 0 };
    if &reader.take::<4>()? != crate::MAGIC {
        return Err(DecodeError::WrongMagic);
    }
    if reader.u16()? != protocol {
        return Err(DecodeError::WrongProtocol);
    }
    let kind = reader.u8()?;
    if !matches!(kind, 1 | 2 | 4 | 6) {
        return Err(DecodeError::WrongKind);
    }
    let nonce = reader.u64()?;
    let value = match kind {
        1 => {
            let fingerprint = reader.u64()?;
            let count = reader.count()?;
            let mut players = Vec::with_capacity(count);
            for _ in 0..count {
                players.push(LocalPlayer {
                    team: i8::from_le_bytes(reader.take()?),
                    profile: None,
                });
            }
            for player in &mut players {
                if reader.boolean()? {
                    player.profile = Some(reader.take()?);
                }
            }
            NativeRequest::Hello {
                nonce,
                fingerprint,
                players,
            }
        }
        2 => {
            let cookie = reader.u64()?;
            let sequence = reader.u32()?;
            let count = reader.count()?;
            let mut controls = Vec::with_capacity(count);
            for _ in 0..count {
                controls.push(reader.control()?);
            }
            let ack = SnapshotAck {
                latest: reader.u32()?,
                mask: reader.u32()?,
                flags: reader.u8()?,
            };
            if ack.flags > 1 {
                return Err(DecodeError::InvalidControl);
            }
            let view = SnapshotView {
                tick: reader.u16()?,
                fraction: reader.u8()?,
            };
            let view = if view.fraction == VIEW_ABSENT_FRACTION {
                // The absent marker has exactly one encoding.
                if view.tick != 0 {
                    return Err(DecodeError::InvalidControl);
                }
                None
            } else {
                Some(view)
            };
            let mut motion = Vec::with_capacity(count);
            for _ in 0..count {
                motion.push(reader.motion()?);
            }
            let repeated = usize::from(reader.u8()?);
            // Repeated sequences stay positive: no wrap through zero.
            if repeated > MAX_REDUNDANT_INPUTS || u64::from(sequence) <= repeated as u64 {
                return Err(DecodeError::InvalidCount);
            }
            let mut newer: Vec<RedundantControl> = controls
                .iter()
                .map(|c| RedundantControl {
                    forward: c.forward,
                    right: c.right,
                    yaw: c.yaw,
                    pitch: c.pitch,
                    flags: c.flags,
                    zoom: c.zoom,
                })
                .collect();
            let mut redundant = Vec::with_capacity(repeated);
            for n in 0..repeated {
                let mut entry = Vec::with_capacity(count);
                for view in &newer {
                    entry.push(reader.redundant(view)?);
                }
                redundant.push(RedundantInput {
                    sequence: sequence - 1 - n as u32,
                    controls: entry.clone(),
                });
                newer = entry;
            }
            NativeRequest::Input {
                nonce,
                cookie,
                sequence,
                controls,
                ack,
                view,
                motion,
                redundant,
            }
        }
        4 => NativeRequest::Leave {
            nonce,
            cookie: reader.u64()?,
        },
        6 => NativeRequest::SessionQuery { nonce },
        _ => unreachable!(), // Exhaustive checked wire kind above.
    };
    if reader.at != packet.len() {
        return Err(DecodeError::InvalidLength);
    }
    Ok(value)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn header(kind: u8) -> Vec<u8> {
        let mut value = b"H2N2\x61\x00".to_vec();
        value.push(kind);
        value.extend_from_slice(&0x123456789abcdef0u64.to_le_bytes());
        value
    }
    fn input() -> Vec<u8> {
        let mut value = header(2);
        value.extend_from_slice(&u64::MAX.to_le_bytes());
        value.extend_from_slice(&u32::MAX.to_le_bytes());
        value.push(1);
        for real in [-0.0f32, 1.0, -10000.0, 10000.0] {
            value.extend_from_slice(&real.to_bits().to_le_bytes());
        }
        value.extend_from_slice(&2047u16.to_le_bytes());
        value.extend_from_slice(&(-1i16).to_le_bytes());
        value.extend_from_slice(&u32::MAX.to_le_bytes());
        for _ in 0..4 {
            value.extend_from_slice(&u16::MAX.to_le_bytes());
        }
        for n in 0..10u32 {
            value.extend_from_slice(&(u32::MAX - n).to_le_bytes());
        }
        value.extend_from_slice(&[1, 4, 0, 63, 0, 0x34, 0x12, 1, 0x80]);
        value.extend_from_slice(&[1, 0, 0, 0, 0, 255, 255, 255, 255]);
        value.extend_from_slice(&0x0102_0304u32.to_le_bytes());
        value.extend_from_slice(&0x8000_0001u32.to_le_bytes());
        value.push(1);
        value.extend_from_slice(&0xfedcu16.to_le_bytes());
        value.push(254);
        value.push(0); // no motion block
        value.push(0); // no repeated inputs
        value
    }
    // Same packet with a full motion block and two repeated inputs.
    fn predicted_input() -> Vec<u8> {
        let mut value = input();
        value.truncate(value.len() - 2);
        value.extend_from_slice(&[MOTION_PRESENT | MOTION_GROUNDED, 3]);
        for coordinate in [-(1i32 << 23), (1 << 23) - 1, 0x12_3456] {
            value.extend_from_slice(&coordinate.to_le_bytes()[..3]);
        }
        for velocity in [i16::MIN, i16::MAX, -2] {
            value.extend_from_slice(&velocity.to_le_bytes());
        }
        value.push(2);
        // sequence - 1: forward changes.
        value.push(1);
        value.extend_from_slice(&0.5f32.to_bits().to_le_bytes());
        // sequence - 2: yaw and flags change.
        value.push(4 | 16);
        value.extend_from_slice(&1.25f32.to_bits().to_le_bytes());
        value.extend_from_slice(&3u16.to_le_bytes());
        value
    }

    #[test]
    fn hello_owns_every_profile_and_signed_team() {
        let mut packet = header(1);
        packet.extend_from_slice(&0x1122334455667788u64.to_le_bytes());
        packet.extend_from_slice(&[4, 255, 0, 7, 128]);
        for seat in 0..4 {
            packet.push(1);
            packet.extend_from_slice(&[seat; PROFILE_BYTES]);
        }
        let decoded = decode_request(&packet, 97).unwrap();
        packet.fill(0);
        let NativeRequest::Hello {
            nonce,
            fingerprint,
            players,
        } = decoded
        else {
            panic!("wrong kind")
        };
        assert_eq!(nonce, 0x123456789abcdef0);
        assert_eq!(fingerprint, 0x1122334455667788);
        assert_eq!(
            players.iter().map(|p| p.team).collect::<Vec<_>>(),
            [-1, 0, 7, -128]
        );
        for (seat, player) in players.iter().enumerate() {
            assert_eq!(player.profile, Some([seat as u8; PROFILE_BYTES]));
        }
    }

    #[test]
    fn controls_preserve_bits_serials_and_vehicle_claims() {
        let NativeRequest::Input {
            nonce,
            cookie,
            sequence,
            controls,
            ack,
            view,
            motion,
            redundant,
        } = decode_request(&input(), 97).unwrap()
        else {
            panic!("wrong kind")
        };
        assert_eq!(motion, vec![None]);
        assert!(redundant.is_empty());
        assert_eq!(
            view,
            Some(SnapshotView {
                tick: 0xfedc,
                fraction: 254
            })
        );
        assert_eq!(
            ack,
            SnapshotAck {
                latest: 0x0102_0304,
                mask: 0x8000_0001,
                flags: 1
            }
        );
        assert_eq!(nonce, 0x123456789abcdef0);
        assert_eq!(cookie, u64::MAX);
        assert_eq!(sequence, u32::MAX);
        let c = &controls[0];
        assert_eq!(c.forward.to_bits(), (-0.0f32).to_bits());
        assert_eq!((c.right, c.yaw, c.pitch), (1.0, -10000.0, 10000.0));
        assert_eq!((c.flags, c.zoom, c.zoom_reset_serial), (2047, -1, u32::MAX));
        assert_eq!(c.completed, [u16::MAX; 2]);
        assert_eq!(c.presses[9], u32::MAX - 9);
        assert_eq!(
            c.vehicle,
            Some(VehicleIntent {
                action: 4,
                index: 63,
                object: 0x80011234
            })
        );
        assert_eq!(
            c.completed_vehicle,
            Some(VehicleIntent {
                action: 0,
                index: 0,
                object: u32::MAX
            })
        );
    }

    #[test]
    fn malformed_native_requests_fail_before_publication() {
        let valid = input();
        for n in 0..valid.len() {
            assert!(decode_request(&valid[..n], 97).is_err());
        }
        for count in [0, 5, 255] {
            let mut packet = valid.clone();
            packet[27] = count;
            assert_eq!(decode_request(&packet, 97), Err(DecodeError::InvalidCount));
        }
        for bits in [0x7fc00000u32, 0x7f800000, 0xff800000, 0x3f800001] {
            let mut packet = valid.clone();
            packet[28..32].copy_from_slice(&bits.to_le_bytes());
            assert_eq!(
                decode_request(&packet, 97),
                Err(DecodeError::InvalidControl)
            );
        }
        let ack_flags = valid.len() - 6;
        let mut flags = valid.clone();
        flags[ack_flags] = 2;
        assert_eq!(decode_request(&flags, 97), Err(DecodeError::InvalidControl));
        let mut trailing = valid.clone();
        trailing.push(0);
        assert_eq!(
            decode_request(&trailing, 97),
            Err(DecodeError::InvalidLength)
        );
        assert_eq!(decode_request(&valid, 96), Err(DecodeError::WrongProtocol));
        assert_eq!(
            decode_request(&vec![0; 1401], 97),
            Err(DecodeError::TooLarge)
        );
        for kind in [3, 5, 7, 255] {
            assert_eq!(
                decode_request(&header(kind), 97),
                Err(DecodeError::WrongKind)
            );
        }
    }

    fn motion_of(
        packet: &[u8],
    ) -> Result<(Vec<Option<MotionReport>>, Vec<RedundantInput>), DecodeError> {
        match decode_request(packet, 97)? {
            NativeRequest::Input {
                motion, redundant, ..
            } => Ok((motion, redundant)),
            _ => panic!("wrong kind"),
        }
    }

    #[test]
    fn motion_blocks_and_repeated_inputs_are_strict_and_reconstructed() {
        let valid = predicted_input();
        let (motion, redundant) = motion_of(&valid).unwrap();
        let report = MotionReport {
            grounded: true,
            crouching: false,
            baseline_age: 3,
            position: [-(1 << 23), (1 << 23) - 1, 0x12_3456],
            velocity: [i16::MIN, i16::MAX, -2],
        };
        assert_eq!(motion, vec![Some(report)]);
        assert_eq!(redundant.len(), 2);
        assert_eq!(redundant[0].sequence, u32::MAX - 1);
        assert_eq!(redundant[1].sequence, u32::MAX - 2);
        let older = &redundant[0].controls[0];
        assert_eq!((older.forward, older.right, older.yaw), (0.5, 1.0, -10000.0));
        assert_eq!((older.flags, older.zoom), (2047, -1));
        let oldest = &redundant[1].controls[0];
        assert_eq!((oldest.forward, oldest.yaw, oldest.flags), (0.5, 1.25, 3));
        // Every truncation fails; trailing bytes fail.
        for n in 0..valid.len() {
            assert!(decode_request(&valid[..n], 97).is_err());
        }
        let mut trailing = valid.clone();
        trailing.push(0);
        assert_eq!(motion_of(&trailing), Err(DecodeError::InvalidLength));
        let block = input().len() - 2;
        // Reserved motion flags and a second absent encoding are refused.
        for flags in [8u8, 0x80, MOTION_GROUNDED, MOTION_CROUCHING] {
            let mut packet = valid.clone();
            packet[block] = flags;
            assert!(decode_request(&packet, 97).is_err());
        }
        let count = block + 17;
        // Too many repeats, and repeats that would wrap below sequence 1.
        let mut many = valid.clone();
        many[count] = 3;
        assert_eq!(motion_of(&many), Err(DecodeError::InvalidCount));
        let mut wrapping = input();
        let sequence = 23;
        wrapping[sequence..sequence + 4].copy_from_slice(&1u32.to_le_bytes());
        let at = wrapping.len() - 1;
        wrapping[at] = 1;
        wrapping.push(0);
        assert_eq!(motion_of(&wrapping), Err(DecodeError::InvalidCount));
        // A repeated field must differ from the newer input's value.
        let mut same = valid.clone();
        same[count + 2..count + 6].copy_from_slice(&(-0.0f32).to_bits().to_le_bytes());
        assert_eq!(motion_of(&same), Err(DecodeError::InvalidControl));
        // Bits 6 (formerly motion) and 7 are reserved.
        for bit in [0x40u8, 0x80] {
            let mut reserved = valid.clone();
            reserved[count + 1] |= bit;
            assert_eq!(motion_of(&reserved), Err(DecodeError::InvalidControl));
        }
        // Non-finite or out-of-range repeated values are refused.
        let mut nan = valid.clone();
        nan[count + 2..count + 6].copy_from_slice(&f32::NAN.to_bits().to_le_bytes());
        assert_eq!(motion_of(&nan), Err(DecodeError::InvalidControl));
        let flags_at = valid.len() - 2;
        let mut flags = valid.clone();
        flags[flags_at..].copy_from_slice(&2048u16.to_le_bytes());
        assert_eq!(motion_of(&flags), Err(DecodeError::InvalidControl));
    }

    fn view_of(packet: &[u8]) -> Result<Option<SnapshotView>, DecodeError> {
        match decode_request(packet, 97)? {
            NativeRequest::Input { view, .. } => Ok(view),
            _ => panic!("wrong kind"),
        }
    }

    #[test]
    fn displayed_snapshot_view_is_three_strict_bytes() {
        let valid = input();
        let at = valid.len() - 5;
        // Every tick and every fraction below the absent marker round-trips.
        for (tick, fraction) in [(0u16, 0u8), (1, 127), (0xffff, 254), (0x8000, 1)] {
            let mut packet = valid.clone();
            packet[at..at + 2].copy_from_slice(&tick.to_le_bytes());
            packet[at + 2] = fraction;
            assert_eq!(
                view_of(&packet),
                Ok(Some(SnapshotView { tick, fraction }))
            );
        }
        // No view has exactly one encoding: tick 0 with fraction 255.
        let mut absent = valid.clone();
        absent[at..at + 2].copy_from_slice(&0u16.to_le_bytes());
        absent[at + 2] = VIEW_ABSENT_FRACTION;
        assert_eq!(view_of(&absent), Ok(None));
        for tick in [1u16, 0x00ff, 0xff00, 0xffff] {
            let mut packet = absent.clone();
            packet[at..at + 2].copy_from_slice(&tick.to_le_bytes());
            assert_eq!(view_of(&packet), Err(DecodeError::InvalidControl));
        }
        // The view is mandatory: a protocol-112 style packet without it is short.
        assert_eq!(view_of(&valid[..at]), Err(DecodeError::TooShort));
        assert_eq!(view_of(&valid[..at + 2]), Err(DecodeError::TooShort));
        assert_eq!(view_of(&valid[..at + 3]), Err(DecodeError::TooShort));
        let mut trailing = absent;
        trailing.push(0);
        assert_eq!(view_of(&trailing), Err(DecodeError::InvalidLength));
    }
}
