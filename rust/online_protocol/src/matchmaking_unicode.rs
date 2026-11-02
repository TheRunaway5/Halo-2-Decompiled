//! Native modern public-name comparison, pinned to the previous Unicode16 owner.
//! Matchmaking IDs/authentication never depend on a display name or this fold.
use unicode_normalization::UnicodeNormalization;
#[path = "matchmaking_unicode_fold_table.rs"]
mod data;

pub(crate) fn casefold(value: &str) -> String {
    let mut result = String::new();
    for c in value.chars() {
        match data::MAPPINGS.binary_search_by_key(&(c as u32), |(key, _)| *key) {
            Ok(index) => result.push_str(data::MAPPINGS[index].1),
            Err(_) => result.push(c),
        }
    }
    result
}
pub(crate) fn fold(value: &str) -> String {
    casefold(&value.nfc().collect::<String>())
}

#[cfg(test)]
mod tests {
    use super::*;
    use sha2::{Digest, Sha256};

    #[test]
    fn independent_unicode16_full_mappings_and_normalization() {
        assert_eq!(unicode_normalization::UNICODE_VERSION, (16, 0, 0));
        let raw = include_str!("../data/unicode16/CaseFolding.txt");
        assert_eq!(
            format!("{:x}", Sha256::digest(raw.as_bytes())),
            data::SOURCE_SHA256
        );
        let mut count = 0;
        for line in raw.lines() {
            let fields = line
                .split('#')
                .next()
                .unwrap()
                .split(';')
                .map(str::trim)
                .collect::<Vec<_>>();
            if fields.len() < 3 || !["C", "F"].contains(&fields[1]) {
                continue;
            }
            let code = u32::from_str_radix(fields[0], 16).unwrap();
            let expected = fields[2]
                .split_whitespace()
                .map(|c| char::from_u32(u32::from_str_radix(c, 16).unwrap()).unwrap())
                .collect::<String>();
            let index = data::MAPPINGS
                .binary_search_by_key(&code, |(key, _)| *key)
                .unwrap();
            assert_eq!(data::MAPPINGS[index].1, expected);
            count += 1;
        }
        assert_eq!(count, data::MAPPINGS.len());
        for (a, b) in [
            ("Café", "CAFE\u{301}"),
            ("Straße", "STRASSE"),
            ("Σςσ", "σσσ"),
            ("\u{130}", "i\u{307}"),
            ("\u{ab70}", "\u{13a0}"),
        ] {
            assert_eq!(fold(a), fold(b));
        }
        assert_ne!(fold("I"), fold("ı"));
        assert_eq!(casefold("Straße"), "strasse");
        assert_eq!(casefold("CAFE\u{301}"), "cafe\u{301}");
        assert_ne!(casefold("Café"), casefold("CAFE\u{301}"));
    }
}
