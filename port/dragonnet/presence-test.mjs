import assert from 'node:assert/strict';
import {DatabaseSync} from 'node:sqlite';
import fs from 'node:fs';
import {presenceFetch, PRESENCE_TTL, PRESENCE_INTERVAL} from './presence.js';
import worker from './worker.js';
const sql = new DatabaseSync(':memory:');
sql.exec(fs.readFileSync(new URL('./presence-schema.sql',import.meta.url),'utf8'));
const DB={prepare(text){return {values:[],bind(...v){return {...this,values:v};},
    async first(){return sql.prepare(text).get(...this.values)??null;},
    async run(){const r=sql.prepare(text).run(...this.values);return {meta:{changes:Number(r.changes)}};}};}};
const env={DB,ALLOWED_VERSION:'40001'}, id=n=>n.toString(16).padStart(32,'0');
let now=1800000000;
async function call(n,extra={},ip='198.51.100.1'){
    const r=await presenceFetch(env,{v:40001,id:id(n),...extra},now,ip);
    return {status:r.status,...await r.json()};
}
let r=await call(1);assert.equal(r.online,1);assert.equal(r.interval,PRESENCE_INTERVAL);assert.equal(r.ttl,PRESENCE_TTL);
const stored=sql.prepare('SELECT * FROM online_presence').get();
assert(!JSON.stringify(stored).includes(id(1)));assert(!JSON.stringify(stored).includes('198.51.100.1'));
assert.equal(Object.keys(r).sort().join(','),'approximate,interval,online,status,ttl');
assert.equal((await call(1)).online,1);now+=20;await call(1);
assert.equal(sql.prepare('SELECT seen FROM online_presence').get().seen,stored.seen);
assert.equal((await call(2)).online,2); // Two consoles behind the same router.
now+=70;assert.equal((await call(1)).online,2); // Battle/relaunch keeps the same identity.
assert.equal(sql.prepare('SELECT seen FROM online_presence WHERE id=?').get(stored.id).seen,now);
assert.equal((await call(2,{action:'leave'})).online,1);
assert.equal((await call(2,{action:'leave'})).online,1); // Idempotent exit.
now+=PRESENCE_TTL+1;assert.equal((await call(3)).online,1);
for(let n=4;n<=10;n++)assert.equal((await call(n)).status,200);
assert.equal((await call(11)).status,429);
now+=PRESENCE_INTERVAL;assert.equal((await call(3)).online,8); // Full networks can renew.
assert.equal((await call(11,{},'203.0.113.1')).online,9);
assert.equal((await call(1,{id:['a'.repeat(32)]})).status,400);
assert.equal((await call(1,{id:'not-hex'})).status,400);
assert.equal((await call(1,{v:4})).status,409);
assert.equal((await call(1,{action:'wipe'})).status,400);
for(const body of ['null','[]','"hello"']){
    const res=await worker.fetch(new Request('https://test/v1/presence',{method:'POST',body}),env);
    assert.equal(res.status,400);
}
let res=await worker.fetch(new Request('https://test/v1/presence',{method:'POST',body:JSON.stringify({v:40001,id:id(20),extra:'x'.repeat(300)})}),env);
assert.equal(res.status,413);
res=await worker.fetch(new Request('https://test/v1/presence',{method:'POST',body:JSON.stringify({v:40001,id:id(20)})}),{DB:{prepare(){throw Error('missing migration');}}});
assert.equal(res.status,503);assert.equal((await res.json()).error,'presence unavailable');
console.log('PASS presence: SQLite leases, unique consoles/reload, two behind NAT, rate-limited writes, expiry, leave, network ceiling, validation and graceful database failure');
