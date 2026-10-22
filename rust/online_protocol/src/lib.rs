//! Owned parsing for the native ONLINE protocol.
//!
//! Parsed data is not authenticated authority. The transport/session must
//! separately admit the sender, its seats, message role, epoch and sequence.

pub mod authority_archive;
pub mod eos_wire;
pub mod native_requests;

const MAGIC: &[u8; 4] = b"H2N2";
const VOICE_KIND: u8 = 6;
const CAPACITY: u8 = 16;
pub const MAX_OPUS_BYTES: usize = 256;
pub const MAX_VOICE_PACKET_BYTES: usize = 22 + MAX_OPUS_BYTES;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DecodeError {
    TooShort,
    TooLarge,
    WrongMagic,
    WrongProtocol,
    WrongKind,
    InvalidSubtype,
    InvalidLength,
    InvalidSource,
    InvalidBoolean,
    InvalidGain,
    InvalidCount,
    InvalidControl,
    InvalidIntent,
}

/// Syntax-checked wire data. Nothing here grants sender or speaker authority.
#[derive(Clone, Debug, PartialEq)]
pub enum ParsedVoice {
    /// The source is the authenticated transport peer, never a wire field.
    Preferences { enabled: bool, muted: u16 },
    /// Requires the current host as transport sender before it can be used.
    ForwardedPreferences {
        source: u8,
        enabled: bool,
        muted: u16,
    },
    Audio {
        source: u8,
        sequence: u32,
        radio: bool,
        left: f32,
        right: f32,
        opus: Vec<u8>,
    },
}

fn word(packet: &[u8], at: usize) -> Result<u32, DecodeError> {
    let end = at.checked_add(4).ok_or(DecodeError::InvalidLength)?;
    let bytes = packet.get(at..end).ok_or(DecodeError::InvalidLength)?;
    Ok(u32::from_le_bytes(
        bytes.try_into().map_err(|_| DecodeError::InvalidLength)?,
    ))
}

fn boolean(value: u8) -> Result<bool, DecodeError> {
    match value {
        0 => Ok(false),
        1 => Ok(true),
        _ => Err(DecodeError::InvalidBoolean),
    }
}

fn source(value: u8) -> Result<u8, DecodeError> {
    if value < CAPACITY {
        Ok(value)
    } else {
        Err(DecodeError::InvalidSource)
    }
}

fn gain(value: u32) -> Result<f32, DecodeError> {
    let value = f32::from_bits(value);
    if value.is_finite() && (0.0..=1.0).contains(&value) {
        Ok(value)
    } else {
        Err(DecodeError::InvalidGain)
    }
}

/// Parse the application payload after transport decryption/epoch framing.
///
/// `expected_protocol` comes from local protocol/admission policy. A successful
/// return establishes syntax only; the packet's own source is still a claim.
pub fn decode_voice(packet: &[u8], expected_protocol: u16) -> Result<ParsedVoice, DecodeError> {
    if packet.len() < 8 {
        return Err(DecodeError::TooShort);
    }
    if packet.len() > MAX_VOICE_PACKET_BYTES {
        return Err(DecodeError::TooLarge);
    }
    if packet.get(..4) != Some(MAGIC.as_slice()) {
        return Err(DecodeError::WrongMagic);
    }
    if u16::from_le_bytes([packet[4], packet[5]]) != expected_protocol {
        return Err(DecodeError::WrongProtocol);
    }
    if packet[6] != VOICE_KIND {
        return Err(DecodeError::WrongKind);
    }
    match packet[7] {
        subtype @ (1 | 2) => {
            if packet.len() != if subtype == 1 { 11 } else { 12 } {
                return Err(DecodeError::InvalidLength);
            }
            let enabled = boolean(packet[8])?;
            let muted = u16::from_le_bytes([packet[9], packet[10]]);
            if subtype == 1 {
                Ok(ParsedVoice::Preferences { enabled, muted })
            } else {
                Ok(ParsedVoice::ForwardedPreferences {
                    source: source(packet[11])?,
                    enabled,
                    muted,
                })
            }
        }
        0 => {
            if packet.len() <= 22 {
                return Err(DecodeError::InvalidLength);
            }
            let source = source(packet[8])?;
            let sequence = word(packet, 9)?;
            let radio = boolean(packet[13])?;
            let left = gain(word(packet, 14)?)?;
            let right = gain(word(packet, 18)?)?;
            Ok(ParsedVoice::Audio {
                source,
                sequence,
                radio,
                left,
                right,
                opus: packet[22..].to_vec(),
            })
        }
        _ => Err(DecodeError::InvalidSubtype),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Independent literal vectors for the native C++ kind-6 wire fields.
    const PREFERENCES: [u8; 11] = [0x48, 0x32, 0x4e, 0x32, 97, 0, 6, 1, 1, 0x01, 0x80];
    const AUDIO: [u8; 24] = [
        0x48, 0x32, 0x4e, 0x32, 97, 0, 6, 0, 15, 0x78, 0x56, 0x34, 0x12, 1, 0, 0, 0, 0x3f, 0, 0,
        0x80, 0x3f, 0x19, 0xa5,
    ];

    #[test]
    fn exact_preferences_and_forwarded_seat_claim() {
        assert_eq!(
            decode_voice(&PREFERENCES, 97),
            Ok(ParsedVoice::Preferences {
                enabled: true,
                muted: 0x8001
            })
        );
        let mut forwarded = PREFERENCES.to_vec();
        forwarded[7] = 2;
        forwarded.push(15);
        assert_eq!(
            decode_voice(&forwarded, 97),
            Ok(ParsedVoice::ForwardedPreferences {
                source: 15,
                enabled: true,
                muted: 0x8001
            })
        );
        forwarded[11] = 16;
        assert_eq!(
            decode_voice(&forwarded, 97),
            Err(DecodeError::InvalidSource)
        );
    }

    #[test]
    fn exact_audio_is_owned_and_preserves_values() {
        let mut packet = AUDIO.to_vec();
        let decoded = decode_voice(&packet, 97).unwrap();
        packet.fill(0);
        assert_eq!(
            decoded,
            ParsedVoice::Audio {
                source: 15,
                sequence: 0x12345678,
                radio: true,
                left: 0.5,
                right: 1.0,
                opus: vec![0x19, 0xa5]
            }
        );
    }

    #[test]
    fn framing_and_version_fail_closed() {
        for end in 0..PREFERENCES.len() {
            assert!(decode_voice(&PREFERENCES[..end], 97).is_err());
        }
        for end in 0..23 {
            assert!(decode_voice(&AUDIO[..end], 97).is_err());
        }
        assert_eq!(decode_voice(&AUDIO, 96), Err(DecodeError::WrongProtocol));
        assert_eq!(decode_voice(&AUDIO, 353), Err(DecodeError::WrongProtocol));
        for (offset, value, error) in [
            (0, 0, DecodeError::WrongMagic),
            (6, 7, DecodeError::WrongKind),
            (7, 3, DecodeError::InvalidSubtype),
            (8, 16, DecodeError::InvalidSource),
            (13, 2, DecodeError::InvalidBoolean),
        ] {
            let mut packet = AUDIO;
            packet[offset] = value;
            assert_eq!(decode_voice(&packet, 97), Err(error));
        }
        let mut extra = PREFERENCES.to_vec();
        extra.push(0);
        assert_eq!(decode_voice(&extra, 97), Err(DecodeError::InvalidLength));
        extra.truncate(11);
        extra[8] = 2;
        assert_eq!(decode_voice(&extra, 97), Err(DecodeError::InvalidBoolean));
    }

    #[test]
    fn payload_bounds_and_all_byte_values() {
        let mut packet = AUDIO[..22].to_vec();
        assert_eq!(decode_voice(&packet, 97), Err(DecodeError::InvalidLength));
        for value in 0..=u8::MAX {
            packet.truncate(22);
            packet.push(value);
            assert!(
                matches!(decode_voice(&packet, 97), Ok(ParsedVoice::Audio { opus, .. }) if opus == [value])
            );
        }
        packet.resize(MAX_VOICE_PACKET_BYTES, 0xa5);
        assert!(
            matches!(decode_voice(&packet, 97), Ok(ParsedVoice::Audio { opus, .. }) if opus.len() == MAX_OPUS_BYTES)
        );
        packet.push(0);
        assert_eq!(decode_voice(&packet, 97), Err(DecodeError::TooLarge));
    }

    #[test]
    fn gain_domains_preserve_negative_zero_and_reject_non_finite() {
        for offset in [14, 18] {
            for value in [f32::NAN, f32::INFINITY, f32::NEG_INFINITY, -0.01, 1.01] {
                let mut packet = AUDIO;
                packet[offset..offset + 4].copy_from_slice(&value.to_bits().to_le_bytes());
                assert_eq!(decode_voice(&packet, 97), Err(DecodeError::InvalidGain));
            }
        }
        let mut packet = AUDIO;
        packet[14..18].copy_from_slice(&(-0.0_f32).to_bits().to_le_bytes());
        match decode_voice(&packet, 97).unwrap() {
            ParsedVoice::Audio { left, .. } => assert_eq!(left.to_bits(), (-0.0_f32).to_bits()),
            _ => panic!("wrong voice subtype"),
        }
    }

    #[test]
    fn framing_does_not_invent_sequence_authority() {
        for sequence in [0_u32, 1, u32::MAX] {
            let mut packet = AUDIO;
            packet[9..13].copy_from_slice(&sequence.to_le_bytes());
            assert!(
                matches!(decode_voice(&packet, 97), Ok(ParsedVoice::Audio { sequence: actual, .. }) if actual == sequence)
            );
        }
    }

    #[test]
    fn arbitrary_short_inputs_are_bounded_errors_or_owned_messages() {
        let mut seed = 0x48153297_u32;
        for length in 0..=512 {
            let mut bytes = vec![0; length];
            for byte in &mut bytes {
                seed = seed.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                *byte = (seed >> 24) as u8;
            }
            let _ = decode_voice(&bytes, 97);
        }
    }
}
