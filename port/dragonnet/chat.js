// Optional general chat, never the private peer chat. SPDX-License-Identifier: MIT
import {versionAllowed} from './versions.js';
const reply=(body,status=200)=>new Response(JSON.stringify(body),{status,headers:{'Content-Type':'application/json','Cache-Control':'no-store'}});
const hex=(s,n)=>typeof s==='string' && s.length===n && /^[0-9a-f]+$/.test(s);
async function hash(s){return [...new Uint8Array(await crypto.subtle.digest('SHA-256',new TextEncoder().encode(s)))].map(b=>b.toString(16).padStart(2,'0')).join('');}
const encode=s=>[...new TextEncoder().encode(s)].map(b=>b.toString(16).padStart(2,'0')).join('');
async function limit(db,key,slot,max){
 const row=await db.prepare('INSERT INTO used(what,slot,n) VALUES(?,?,1) ON CONFLICT(what,slot) DO UPDATE SET n=n+1 RETURNING n').bind(key,slot).first();return row.n<=max;
}
export async function chatFetch(env,body,now){
 if(String(env.CHAT_ENABLED)!=='1')return reply({error:'chat unavailable'},503);
 if(!versionAllowed(env,body.v))return reply({error:'version'},409);
 if(!hex(body.id,32)||!hex(body.token,64))return reply({error:'identity'},401);
 const db=env.DB,p=await db.prepare('SELECT id,token_hash,name FROM ranked_players WHERE id=?').bind(body.id).first();
 if(!p || p.token_hash!==await hash(body.token))return reply({error:'identity'},401);
 if(!Number.isSafeInteger(body.after) || body.after<0)return reply({error:'bad request'},400);
 if(!await limit(db,'chat-api:'+p.id,now-now%60,30))return reply({error:'busy'},429);
 let sent=0;
 if(body.text_hex!==undefined){
  if(typeof body.text_hex!=='string'||!body.text_hex.length||body.text_hex.length>320||body.text_hex.length%2||!/^[0-9a-f]+$/.test(body.text_hex)||!hex(body.nonce,32))return reply({error:'bad request'},400);
  let text;
  try{text=new TextDecoder('utf-8',{fatal:true}).decode(Uint8Array.from(body.text_hex.match(/../g),h=>parseInt(h,16)));}catch{return reply({error:'bad request'},400);}
  text=text.replace(/[\p{Cc}\p{Cf}]/gu,' ').replace(/\s+/gu,' ').trim();
  if(!text)return reply({error:'bad request'},400);
  const exists=await db.prepare('SELECT seq FROM community_chat WHERE author=? AND nonce=?').bind(p.id,body.nonce).first();
  if(exists)sent=exists.seq;
  else {
   if(!await limit(db,'chat-send:'+p.id,now-now%60,6))return reply({error:'slow down'},429);
   await db.prepare('INSERT OR IGNORE INTO community_chat(author,nonce,name,text,made) VALUES(?,?,?,?,?)').bind(p.id,body.nonce,p.name,text,now).run();
   sent=(await db.prepare('SELECT seq FROM community_chat WHERE author=? AND nonce=?').bind(p.id,body.nonce).first()).seq;
  }
 }
 await db.prepare('DELETE FROM community_chat WHERE made<? OR seq<(SELECT COALESCE(MAX(seq),0)-199 FROM community_chat)').bind(now-86400).run();
 const rows=body.after ? await db.prepare('SELECT seq,author,name,text FROM community_chat WHERE seq>? ORDER BY seq LIMIT 12').bind(body.after).all() : await db.prepare('SELECT seq,author,name,text FROM community_chat ORDER BY seq DESC LIMIT 12').all();
 if(!body.after)rows.results.reverse();
 return reply({sent,rows:rows.results.map(r=>({seq:r.seq,name_hex:encode(r.name),text_hex:encode(r.text),mine:r.author===p.id}))});
}
