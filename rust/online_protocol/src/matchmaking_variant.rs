//! Read-only full-buffer admission from original TU1.5 0x1A7140/0x1A7170.
//!
//! Matchmaking rejects invalid settings. It never publishes the original
//! normalizer's localized Slayer reset as a replacement for the supplied game.
//! Reserved bytes and literal UTF16 titles remain part of the admitted buffer.
pub const BYTES: usize = 304;

fn read(raw: &[u8], offset: usize, width: usize) -> u32 {
    raw[offset..offset + width]
        .iter()
        .enumerate()
        .fold(0, |value, (i, byte)| value | (u32::from(*byte) << (i * 8)))
}
fn write(raw: &mut [u8], offset: usize, value: u32, width: usize) {
    raw[offset..offset + width].copy_from_slice(&value.to_le_bytes()[..width]);
}
fn mask(raw: &mut [u8], offset: usize, bits: u32, width: usize) {
    let value = read(raw, offset, width) & bits;
    write(raw, offset, value, width);
}
fn clamp(raw: &mut [u8], offset: usize, low: i32, high: i32, width: usize) {
    let value = read(raw, offset, width);
    let signed = match width {
        1 => i32::from(value as u8 as i8),
        2 => i32::from(value as u16 as i16),
        4 => value as i32,
        _ => unreachable!(),
    };
    write(raw, offset, signed.clamp(low, high) as u32, width);
}

pub fn valid(input: &[u8]) -> bool {
    let Ok(mut raw): Result<[u8; BYTES], _> = input.try_into() else {
        return false;
    };
    let original_type = read(input, 0x44, 4) as i32;
    mask(&mut raw, 0, 1, 2);
    write(&mut raw, 0x42, 0, 2);
    mask(&mut raw, 0x48, 0x7fff, 4);
    for offset in [0x50, 0x54, 0x7c, 0x80, 0x84, 0xac] {
        clamp(&mut raw, offset, 0, 0x7fff, 4);
    }
    for offset in [0x58, 0x88, 0xa4, 0xa8] {
        clamp(&mut raw, offset, 0, 2, 4);
    }
    clamp(&mut raw, 0x4c, 0, 6, 4);
    clamp(&mut raw, 0x74, 0, 16, 4);
    clamp(&mut raw, 0x78, 0, 16, 4);
    write(&mut raw, 0xb0, 0, 4);
    clamp(&mut raw, 0xb4, 0, 8, 4);
    for (i, high) in [3, 7, 7, 4, 4, 4, 6, 6, 0x13, 3, 0x14, 0x14]
        .into_iter()
        .enumerate()
    {
        clamp(&mut raw, 0xcc + i, 0, high, 1);
    }
    clamp(&mut raw, 0x44, 1, 9, 4);
    if read(&raw, 0x44, 4) as i32 == original_type {
        match original_type {
            1 | 9 => {
                if original_type == 9 {
                    clamp(&mut raw, 0x108, 0, 15, 2);
                    clamp(&mut raw, 0x10a, 0, 15, 2);
                }
                let flags = read(&raw, 0x48, 4) | 1;
                write(&mut raw, 0x48, flags, 4);
                mask(&mut raw, 0xf0, 0xff, 4);
                clamp(&mut raw, 0xf4, 0, 0x7fff, 4);
                clamp(&mut raw, 0xf8, 0, 2, 4);
                clamp(&mut raw, 0xfc, 0, 1, 4);
                clamp(&mut raw, 0x100, 0, 3, 4);
                clamp(&mut raw, 0x104, 0, 2, 4);
            }
            2 => mask(&mut raw, 0xf0, 7, 4),
            3 => {
                mask(&mut raw, 0xf0, 7, 4);
                for (offset, high) in [(0xf4, 3), (0xf6, 1), (0xf8, 2), (0xfa, 3)] {
                    clamp(&mut raw, offset, 0, high, 2);
                }
            }
            4 => {
                mask(&mut raw, 0xf0, 0x1f, 4);
                clamp(&mut raw, 0xf4, 0, 0x7fff, 2);
            }
            7 => {
                mask(&mut raw, 0x48, 0xffff_fffe, 4);
                mask(&mut raw, 0xf0, 0x7f, 4);
                clamp(&mut raw, 0xf4, 0, 2, 2);
            }
            8 => {
                clamp(&mut raw, 0xf0, 1, 8, 2);
                clamp(&mut raw, 0xf2, 1, 0x7fff, 2);
                clamp(&mut raw, 0xf4, 1, 0x7fff, 2);
            }
            _ => write(&mut raw, 0x44, 10, 4),
        }
    }
    raw.as_slice() == input
}
