// Anonymous, approximate online population. SPDX-License-Identifier: MIT
import {versionAllowed} from './versions.js';
export const PRESENCE_INTERVAL = 90, PRESENCE_TTL = 210;
const MIN_WRITE_INTERVAL = 60, PER_NETWORK = 8;
const reply = (body, status = 200) => new Response(JSON.stringify(body), {
    status, headers: {'Content-Type': 'application/json', 'Cache-Control': 'no-store'},
});
async function hash(text) {
    const data = await crypto.subtle.digest('SHA-256', new TextEncoder().encode(text));
    return Array.from(new Uint8Array(data), b => b.toString(16).padStart(2, '0')).join('');
}
export async function presenceFetch(env, body, now, ip) {
    if (!versionAllowed(env, body.v)) return reply({error: 'version'}, 409);
    if (typeof body.id !== 'string' || !/^[0-9a-f]{32}$/.test(body.id) ||
        (body.action !== undefined && !['beat', 'leave'].includes(body.action)))
        return reply({error: 'bad request'}, 400);
    const db = env.DB, id = await hash('presence:' + body.id);
    // A daily network digest limits phantom clients without storing
    // raw IP addresses in this table. Identifiers/names never appear in responses.
    const network = await hash('presence:' + Math.floor(now / 86400) + ':' + ip);
    const cutoff = now - PRESENCE_TTL;
    await db.prepare('DELETE FROM online_presence WHERE seen<=?').bind(cutoff).run();
    if (body.action === 'leave') {
        await db.prepare('DELETE FROM online_presence WHERE id=?').bind(id).run();
    } else {
        // One atomic admission/update: retries, multiple tabs and the NRO reload
        // into a battle do not add another player or rewrite a lease every poll.
        await db.prepare(`INSERT INTO online_presence(id,network,seen)
            SELECT ?,?,? WHERE EXISTS(SELECT 1 FROM online_presence WHERE id=?)
            OR (SELECT COUNT(*) FROM online_presence WHERE network=? AND seen>?)<?
            ON CONFLICT(id) DO UPDATE SET network=excluded.network,seen=excluded.seen
            WHERE online_presence.seen<=?`)
            .bind(id, network, now, id, network, cutoff, PER_NETWORK, now - MIN_WRITE_INTERVAL).run();
        if (!await db.prepare('SELECT id FROM online_presence WHERE id=?').bind(id).first())
            return reply({error: 'busy'}, 429);
    }
    const row = await db.prepare('SELECT COUNT(*) AS n FROM online_presence WHERE seen>?').bind(cutoff).first();
    return reply({online: row.n, interval: PRESENCE_INTERVAL, ttl: PRESENCE_TTL, approximate: true});
}
