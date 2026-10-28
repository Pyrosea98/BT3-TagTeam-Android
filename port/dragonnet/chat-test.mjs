import assert from 'node:assert/strict';
import {DatabaseSync} from 'node:sqlite';
import fs from 'node:fs';
import {chatFetch} from './chat.js';
import {rankedFetch} from './ranked.js';
const sql=new DatabaseSync(':memory:');
for(const f of ['schema.sql','ranked-schema.sql','chat-schema.sql'])sql.exec(fs.readFileSync(new URL('./'+f,import.meta.url),'utf8'));
const DB={prepare(text){return {values:[],bind(...v){return {...this,values:v};},async first(){return sql.prepare(text).get(...this.values)??null;},async run(){return {meta:{changes:Number(sql.prepare(text).run(...this.values).changes)}};},async all(){return {results:sql.prepare(text).all(...this.values)};}};}};
const env={DB,CHAT_ENABLED:'1',RANKED_ENABLED:'1'},now=1800000000;
async function register(name){return (await rankedFetch(null,env,'register',{v:40001,name},now,'127.0.0.1')).json();}
const a=await register('Alpha'),b=await register('Beta');
const encode=s=>Buffer.from(s).toString('hex');
let serial=0;
async function call(who,extra={},time=now){const r=await chatFetch(env,{v:40001,...who,after:0,...extra},time);return {status:r.status,...await r.json()};}
let r=await call(a);assert.deepEqual(r.rows,[]);
const msg={nonce:'1'.repeat(32),text_hex:encode('Hola, "rival"! / \\ ñá')};
r=await call(a,msg);assert.equal(r.rows.length,1);assert.equal(r.rows[0].mine,true);
assert.equal(Buffer.from(r.rows[0].text_hex,'hex').toString(),'Hola, "rival"! / \\ ñá');
assert.equal((await call(a,msg)).rows.length,1); // HTTP retry is idempotent.
r=await call(b);assert.equal(r.rows.length,1);assert.equal(r.rows[0].mine,false);assert.equal(Buffer.from(r.rows[0].name_hex,'hex').toString(),'Alpha');
assert(!JSON.stringify(r).includes(a.id) && !JSON.stringify(r).includes(a.token));
assert.equal((await call({id:a.id,token:b.token},msg)).status,401);
assert.equal((await call(a,{...msg,nonce:'2'.repeat(32),text_hex:'ff'})).status,400);
assert.equal((await call(a,{...msg,nonce:'2'.repeat(32),text_hex:encode(' '.repeat(161))})).status,400);
assert.equal((await call(a,{...msg,nonce:'2'.repeat(32),text_hex:encode('hello\u202e\n\u0000world')})).status,200);
r=await call(b,{after:1});assert.equal(r.rows.length,1);assert.equal(Buffer.from(r.rows[0].text_hex,'hex').toString(),'hello world');
for(let i=0;i<4;i++)await call(a,{nonce:(i+3).toString(16).padStart(32,'0'),text_hex:encode('message '+i)});
assert.equal((await call(a,{nonce:'f'.repeat(32),text_hex:encode('too fast')})).status,429);
assert.equal((await call(a,msg)).status,200); // Duplicate resend does not consume another send slot.
assert.equal((await call(a,{},now+86401)).rows.length,0);
assert.equal((await chatFetch({...env,CHAT_ENABLED:'0'},{v:40001,...a,after:0},now)).status,503);
assert.equal(sql.prepare('SELECT rating FROM ranked_players WHERE id=?').get(a.id).rating,1000);
console.log('PASS general chat: authenticated profile, escaping/UTF-8/control cleanup, idempotent retries, shared visibility, no credential exposure, read cursor, send limits, expiry and untouched Ranked points');
