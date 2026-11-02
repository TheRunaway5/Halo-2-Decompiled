//! Private client-supplied voice preferences and accepted social audiences.
//! These are modern service rules, not recovered Xbox codec/Live procedures.
use super::*;
use std::collections::BTreeSet;

fn schema(c: &mut Cursor<'_>) -> Flow<Value> {
    // This owner is constructed before the public friends API. Share its
    // actual schema rather than creating a second block relationship owner.
    friends::schema(c)?;
    c.write("CREATE TABLE IF NOT EXISTS voice_blocks(player TEXT NOT NULL REFERENCES players(id),subject TEXT NOT NULL,PRIMARY KEY(player,subject))",json!([]),None)?;
    Ok(Value::Null)
}
pub(super) fn subject(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    Ok(c.one("SELECT subject FROM players WHERE id=? AND provider='discord' UNION ALL SELECT subject FROM player_identities WHERE player=? AND provider='discord' LIMIT 1",json!([who,who]))?.map(|r|r[0].clone()).unwrap_or(Value::Null))
}
pub(super) fn allowed(c: &mut Cursor<'_>, a: &str, b: &str) -> Flow<bool> {
    // One current SQL snapshot owns all bilateral decisions. There is no
    // Python projection or derived cache to invalidate on another connection.
    Ok(c.one("WITH a_subject AS (SELECT subject FROM players WHERE id=?1 AND provider='discord' UNION ALL SELECT subject FROM player_identities WHERE player=?1 AND provider='discord' LIMIT 1),b_subject AS (SELECT subject FROM players WHERE id=?2 AND provider='discord' UNION ALL SELECT subject FROM player_identities WHERE player=?2 AND provider='discord' LIMIT 1) SELECT 1 WHERE EXISTS(SELECT 1 FROM players WHERE id=?1) AND EXISTS(SELECT 1 FROM players WHERE id=?2) AND NOT EXISTS(SELECT 1 FROM player_friend_blocks WHERE (player=?1 AND target=?2) OR (player=?2 AND target=?1)) AND NOT EXISTS(SELECT 1 FROM voice_blocks WHERE (player=?1 AND subject IN b_subject) OR (player=?2 AND subject IN a_subject))",json!([a,b]))?.is_some())
}
fn update(c: &mut Cursor<'_>, r: &Value) -> Flow<Value> {
    let body = &r["body"];
    m::fields(body, &["blocked"])?;
    let rows = body["blocked"]
        .as_array()
        .ok_or(Error(400, "Invalid voice block list"))?;
    need(rows.len() <= 2000, 400, "Invalid voice block list")?;
    let mut values = BTreeSet::new();
    for row in rows {
        let value = row.as_str().ok_or(Error(400, "Invalid voice block list"))?;
        need(
            (17..=20).contains(&value.len())
                && value.bytes().all(|c| c.is_ascii_digit())
                && values.insert(value.to_owned()),
            400,
            "Invalid voice block list",
        )?;
    }
    // This is a typed fact returned by the existing verified OAuth/Steam IO,
    // not a client-selected player ID or an independently verified block list.
    let owner = &r["verified_owner"];
    m::fields(owner, &["provider", "subject"])?;
    let provider = txt(&owner["provider"])?;
    need(
        matches!(provider, "discord" | "steam"),
        403,
        "Account identity required",
    )?;
    let subject = m::text(&owner["subject"], 256)?;
    let who=c.one("SELECT id FROM players WHERE provider=? AND subject=? UNION ALL SELECT player FROM player_identities WHERE provider=? AND subject=? LIMIT 1",json!([provider,subject,provider,subject]))?.ok_or(Error(409,"Connect to ONLINE before syncing voice preferences"))?;
    let who = txt(&who[0])?;
    let own_subject = if provider == "discord" {
        json!(subject)
    } else {
        self::subject(c, who)?
    };
    if let Some(own) = own_subject.as_str() {
        values.remove(own);
    }
    let total=c.one("SELECT (SELECT count(*) FROM voice_blocks)-(SELECT count(*) FROM voice_blocks WHERE player=?)",json!([who]))?.ok_or(Error(503,"Voice preference count unavailable"))?;
    need(
        m::integer(&total[0], 0, i64::MAX)? <= 100000 - values.len() as i64,
        503,
        "Voice preference capacity reached",
    )?;
    let mut statements = vec![statement(
        "DELETE FROM voice_blocks WHERE player=?",
        json!([who]),
        None,
    )];
    statements.extend(values.into_iter().map(|v| {
        statement(
            "INSERT INTO voice_blocks VALUES(?,?)",
            json!([who, v]),
            Some(1),
        )
    }));
    c.batch(statements)?;
    Ok(json!({"state":"updated"}))
}

fn relay_schema(c: &mut Cursor<'_>, kind: &str) -> Flow<Value> {
    c.write("CREATE TABLE IF NOT EXISTS social_voice_membership_epoch(kind TEXT NOT NULL CHECK(kind IN ('clan','party')),player TEXT NOT NULL REFERENCES players(id) ON DELETE CASCADE,revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>=0),PRIMARY KEY(kind,player))",json!([]),None)?;
    let table = if kind == "clan" {
        "clan_members"
    } else {
        "party_members"
    };
    c.write(&format!("INSERT OR IGNORE INTO social_voice_membership_epoch SELECT '{kind}',player,0 FROM {table}"),json!([]),None)?;
    for (suffix, event, who) in [("insert", "INSERT", "NEW"), ("delete", "DELETE", "OLD")] {
        c.write(&format!("CREATE TRIGGER IF NOT EXISTS social_voice_{kind}_{suffix} AFTER {event} ON {table} BEGIN INSERT INTO social_voice_membership_epoch VALUES('{kind}',{who}.player,1) ON CONFLICT(kind,player) DO UPDATE SET revision=revision+1; END"),json!([]),None)?;
    }
    // Role/leader changes keep the same membership. Moving a persisted member
    // to another group (or replacing a party handle) ends the old incarnation.
    let columns = if kind == "clan" {
        "clan,player"
    } else {
        "party,player,handle"
    };
    let changed = if kind == "clan" {
        "OLD.clan!=NEW.clan OR OLD.player!=NEW.player"
    } else {
        "OLD.party!=NEW.party OR OLD.player!=NEW.player OR OLD.handle!=NEW.handle"
    };
    c.write(&format!("CREATE TRIGGER IF NOT EXISTS social_voice_{kind}_update AFTER UPDATE OF {columns} ON {table} WHEN {changed} BEGIN INSERT INTO social_voice_membership_epoch VALUES('{kind}',OLD.player,1) ON CONFLICT(kind,player) DO UPDATE SET revision=revision+1; INSERT INTO social_voice_membership_epoch VALUES('{kind}',NEW.player,1) ON CONFLICT(kind,player) DO UPDATE SET revision=revision+1; END"),json!([]),None)?;
    Ok(Value::Null)
}
fn member(c: &mut Cursor<'_>, kind: &str, who: &str) -> Flow<Option<Value>> {
    if kind == "party" && c.playing(who)? {
        return Ok(None);
    }
    let row = if kind == "clan" {
        c.one("SELECT m.clan,m.player,e.revision FROM clan_members m JOIN clans g ON g.id=m.clan JOIN social_voice_membership_epoch e ON e.kind='clan' AND e.player=m.player WHERE m.player=?",json!([who]))?
    } else {
        c.one("SELECT m.party,m.handle,e.revision FROM party_members m JOIN parties g ON g.id=m.party JOIN social_voice_membership_epoch e ON e.kind='party' AND e.player=m.player WHERE m.player=?",json!([who]))?
    };
    Ok(row.map(|r| json!({"group":r[0],"wire":r[1],"epoch":r[2]})))
}
fn valid(c: &mut Cursor<'_>, kind: &str, peer: &Value) -> Flow<bool> {
    if peer.is_null() {
        return Ok(false);
    }
    let current = member(c, kind, txt(&peer["player"])?)?;
    Ok(current.as_ref().is_some_and(|m| {
        m["group"] == peer["group"] && m["wire"] == peer["wire"] && m["epoch"] == peer["epoch"]
    }))
}
pub(super) fn operation(r: &Value, c: &mut Cursor<'_>) -> Flow<Value> {
    let mode = txt(&r["mode"])?;
    match mode {
        "schema" => return schema(c),
        "update" => return update(c, r),
        "subject" => return subject(c, txt(&r["player"])?),
        "allowed" => return Ok(json!(allowed(c, txt(&r["player"])?, txt(&r["target"])?)?)),
        _ => {}
    }
    need(c.voices, 503, "Native voice source required")?;
    let context = c.voice(json!({"op":"context"}))?;
    let kind = txt(&context["kind"])?;
    if mode == "relay_schema" {
        return relay_schema(c, kind);
    }
    if mode == "preflight" || mode == "attach" {
        let who = txt(&r["player"])?;
        let membership = member(c, kind, who)?.ok_or(Error(
            403,
            if kind == "clan" {
                "Join an available clan before joining voice"
            } else {
                "Join an available party before joining voice"
            },
        ))?;
        need(
            mode == "preflight" || c.transaction_owned,
            409,
            "Voice attachment requires an owned transaction",
        )?;
        return c
            .voice(json!({"op":mode,"player":who,"key":r["account_key"],"membership":membership}));
    }
    let body = &r["body"];
    if mode == "detach" {
        m::fields(body, &["peer"])?;
        need(
            c.transaction_owned,
            409,
            "Voice detach requires an owned transaction",
        )?;
        return c.voice(json!({"op":"detach","peer":body["peer"]}));
    }
    m::fields(
        body,
        if mode == "frame" {
            &["peer", "binary", "packet"]
        } else {
            &["peer"]
        },
    )?;
    let peer = c.voice(json!({"op":"peer","peer":body["peer"]}))?;
    need(
        c.transaction_owned,
        409,
        "Voice relay requires an owned transaction",
    )?;
    if !valid(c, kind, &peer)? {
        return c.voice(json!({"op":"close","peer":body["peer"],"code":1008}));
    }
    match mode {
        "poll" => {
            let queued = c.voice(json!({"op":"next","peer":body["peer"]}))?;
            if queued.is_null() {
                return Ok(json!({"packet":null,"again":false}));
            }
            let source = c.voice(json!({"op":"peer","peer":queued["source"]}))?;
            let allowed = valid(c, kind, &source)?
                && allowed(c, txt(&source["player"])?, txt(&peer["player"])?)?;
            c.voice(json!({"op":"deliver","peer":body["peer"],"receipt":queued["receipt"],"allowed":allowed}))
        }
        "frame" => {
            let audience = c.voice(json!({"op":"audience","peer":body["peer"]}))?;
            let mut targets = Vec::new();
            for listener in audience
                .as_array()
                .ok_or(Error(503, "Native voice audience required"))?
            {
                if valid(c, kind, listener)?
                    && allowed(c, txt(&peer["player"])?, txt(&listener["player"])?)?
                {
                    targets.push(listener["id"].clone());
                }
            }
            c.voice(json!({"op":"frame","peer":body["peer"],"binary":body["binary"],"packet":body["packet"],"targets":targets}))
        }
        _ => Err(Error(400, "Unknown private voice operation").into()),
    }
}
