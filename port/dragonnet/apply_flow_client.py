#!/usr/bin/env python3
"""Etapa A do cliente: anuncia flow:2 na fila e envia "ready" (lobby conectado)
e "started" (inicio da luta) ao servico. Nao inclui a tela de revanche (Etapa B).
Uso (raiz do repo):  python3 port/dragonnet/apply_flow_client.py          (teste)
                     python3 port/dragonnet/apply_flow_client.py --apply  (aplica)
Falha sem alterar NENHUM arquivo se um trecho nao for achado exatamente 1 vez."""
import os, sys
FUNCS = r'''static void do_lifecycle(void) {
    char match[33], answer[512], extra[96];
    int ready, started;
    SDL_LockMutex(sLock);
    ready = sLifeReady;
    started = sLifeStarted;
    SDL_strlcpy(match, sLifeMatch, sizeof(match));
    SDL_UnlockMutex(sLock);
    if ((!ready && !started) || !is_hex(match, 32)) {
        return;
    }
    SDL_snprintf(extra, sizeof(extra), ",\"match\":\"%s\",\"action\":\"%s\"", match, ready ? "ready" : "started");
    if (post_ranked("lifecycle", extra, answer, sizeof(answer)) || ++sLifeTries >= 5) {
        SDL_LockMutex(sLock);
        if (ready) {
            sLifeReady = 0;
        } else {
            sLifeStarted = 0;
        }
        sLifeTries = 0;
        SDL_UnlockMutex(sLock);
    }
}'''
PUB = r'''void DrRanked_Connection(int ready) {
    if (!ready) {
        return;
    }
    SDL_LockMutex(sLock);
    if (sView.flow && is_hex(sView.match, 32)) {
        SDL_strlcpy(sLifeMatch, sView.match, sizeof(sLifeMatch));
        sLifeReady = 1;
        sLifeTries = 0;
    }
    SDL_UnlockMutex(sLock);
    start(NULL);
}

void DrRanked_FightStarted(void) {
    char match[33];
    int role, mode, sent, ok;
    if (!session_read(match, &role, &mode, &sent) || sent) {
        return;
    }
    SDL_LockMutex(sLock);
    ok = sView.flow && strcmp(sView.match, match) == 0 && strcmp(sStartSent, match) != 0;
    if (ok) {
        SDL_strlcpy(sLifeMatch, match, sizeof(sLifeMatch));
        SDL_strlcpy(sStartSent, match, sizeof(sStartSent));
        sLifeStarted = 1;
        sLifeTries = 0;
    }
    SDL_UnlockMutex(sLock);
    if (ok) {
        start(NULL);
    }
}'''
W = "/* ---- the window's side ---- */"
EDITS = {
"port/src/gs/net_dragon.h": [
 ("struct-flow", "int searching, role, mode;",
  "int flow;            /* 2: the service negotiated the lifecycle (flow:2) with the rival */\n    int searching, role, mode;"),
 ("decl", "int DrRanked_Handover(void);",
  "void DrRanked_Connection(int ready);   /* the lobby is connected (flow:2): tells the service */\nvoid DrRanked_FightStarted(void);      /* the fight begins (flow:2): tells the service */\nint DrRanked_Handover(void);"),
],
"port/src/gs/net_dragon.c": [
 ("statics", "static char sCancelMatch[33], sRoomPending[8], sHandedOver[33];",
  "static char sCancelMatch[33], sRoomPending[8], sHandedOver[33];\nstatic char sLifeMatch[33], sStartSent[33]; /* flow:2: the match the lifecycle calls are for */\nstatic int sLifeReady, sLifeStarted, sLifeTries;"),
 ("update_match", 'sView.role = number_value(answer, "role");',
  'sView.role = number_value(answer, "role");\n    sView.flow = number_value(answer, "flow") == 2;'),
 ("queue", r'''",\"rematches\":0,\"rules\":\"%s\"", mode''',
  r'''",\"rematches\":0,\"flow\":2,\"rules\":\"%s\"", mode'''),
 ("urgent", "sChatOutN > 0 || sPresenceLeave;",
  "sChatOutN > 0 || sPresenceLeave || sLifeReady || sLifeStarted;"),
 ("call", "/* the next round: at once when asked for something",
  "do_lifecycle();\n        /* the next round: at once when asked for something"),
 ("def", "static int SDLCALL worker(void *unused) {",
  "/* flow:2 (the service's lifecycle): \"ready\" once the lobby is connected, \"started\" when the fight begins.\n   Sent by this thread (the game never waits for HTTP); gives up after a few failed tries. */\n" + FUNCS + "\n\nstatic int SDLCALL worker(void *unused) {"),
 ("hooks", W,
  W + "\n\n/* flow:2 hooks (the game's side): only for matches whose rival also announced flow:2. */\n" + PUB),
],
"port/src/gs/ui_dragonnet.inc": [
 ("connected", "if (sRankedLobby) { DrRanked_Handover(); }",
  "if (sRankedLobby) { DrRanked_Connection(1); DrRanked_Handover(); }"),
],
"src/battle/btl_seq.c": [
 ("fight", "s32 BtlSeqFight_Enter(BtlSeqWaitCtx *ctx) {",
  "s32 BtlSeqFight_Enter(BtlSeqWaitCtx *ctx) {\n#ifdef PORT\n    { extern void DrRanked_FightStarted(void); DrRanked_FightStarted(); }\n#endif"),
],
}
def main():
    apply = "--apply" in sys.argv
    data, bad = {}, 0
    for path, edits in EDITS.items():
        if not os.path.isfile(path):
            print("FALTA:", path); return 1
        raw = open(path, "rb").read().decode("utf-8")
        crlf = "\r\n" in raw
        txt = raw.replace("\r\n", "\n")
        if "DrRanked_FightStarted" in txt or "sLifeReady" in txt:
            print("ja aplicado em", path, "- nada feito."); return 1
        for name, a, b in edits:
            n = txt.count(a)
            print(("OK   " if n == 1 else "FALHA"), os.path.basename(path), name, "(achado %d vez)" % n)
            bad += n != 1
        data[path] = (txt, crlf)
    if bad:
        print("\n%d trecho(s) nao batem. NADA foi alterado." % bad); return 1
    if not apply:
        print("\nTeste OK. Rode com --apply para aplicar."); return 0
    for path, edits in EDITS.items():
        txt, crlf = data[path]
        for name, a, b in edits:
            txt = txt.replace(a, b, 1)
        if crlf: txt = txt.replace("\n", "\r\n")
        open(path, "wb").write(txt.encode("utf-8"))
        print("Aplicado em", path)
    return 0
sys.exit(main())
