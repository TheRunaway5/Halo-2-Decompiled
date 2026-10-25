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
            NativeRequest::Input {
                nonce,
                cookie,
                sequence,
                controls,
                ack,
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
        } = decode_request(&input(), 97).unwrap()
        else {
            panic!("wrong kind")
        };
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
        let mut flags = valid.clone();
        *flags.last_mut().unwrap() = 2;
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
}
