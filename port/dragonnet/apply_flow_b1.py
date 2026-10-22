#!/usr/bin/env python3
"""B1 (flow:2 rematch logic): port/src/gs/net_dragon.c and net_dragon.h.

Test mode by default (changes NOTHING). Use --apply to write.
If any anchor is not found exactly once, nothing is changed.
Run from the repo root:  python3 port/dragonnet/apply_flow_b1.py [--apply]
"""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
C_PATH = ROOT / "port/src/gs/net_dragon.c"
H_PATH = ROOT / "port/src/gs/net_dragon.h"

H_DECLS = r'''/* flow:2 rematch, after the result (the Revanche screen): 1 asks the state, 2 accepts, 3 leaves. */
void DrRanked_FlowAct(int act);
/* 0 none, 1 confirming, 2 choice, 3 waiting, 4 declined or gave up, 5 a new match was saved. */
int DrRanked_FlowState(void);
/* The saved new match (match needs 33 bytes): 1 if there is one. */
int DrRanked_FlowNext(char *match, int *role, int *mode);
void DrRanked_FlowClear(void);
'''

C_STATICS = r'''
/* flow:2 rematch: what the game asked for (act: 0 none, 1 status, 2 accept, 3 leave) and what the service answered
   (state: 0 none, 1 confirming, 2 choice, 3 waiting, 4 declined, 5 a new match was saved to dragonnet-next.txt). */
static int sFlowAct, sFlowNow, sFlowTries, sFlowState;
static char sFlowMatch[33];
static Uint64 sFlowAt;'''

C_DO_FLOW = r'''/* flow:2 rematch, on the worker thread: the game never waits for HTTP. "accept" is repeated until the service
   has both results (it answers "confirming" until then), then the state is asked every 4 seconds. */
static void do_flow(void) {
    char match[33], next[33], st[24], answer[512], extra[96], text[100];
    int act, fresh, role, mode;
    Uint64 now = SDL_GetTicks();

    SDL_LockMutex(sLock);
    act = sFlowAct;
    fresh = sFlowNow;
    sFlowNow = 0;
    SDL_strlcpy(match, sFlowMatch, sizeof(match));
    mode = sView.mode;
    SDL_UnlockMutex(sLock);
    if (!act || !is_hex(match, 32)) {
        return;
    }
    if (fresh) {
        sFlowTries = 0;
    } else if (now - sFlowAt < 4000) {
        return;
    }
    sFlowAt = now;
    SDL_snprintf(extra, sizeof(extra), ",\"match\":\"%s\",\"action\":\"%s\"", match,
                 act == 3 ? "leave" : act == 2 ? "accept" : "status");
    if (!post_ranked("lifecycle", extra, answer, sizeof(answer))) {
        if (++sFlowTries >= 5) { /* the service does not answer (or is an old one): give up */
            SDL_LockMutex(sLock);
            if (act == sFlowAct && strcmp(match, sFlowMatch) == 0) {
                sFlowAct = 0;
                sFlowState = 4;
            }
            SDL_UnlockMutex(sLock);
        }
        return;
    }
    sFlowTries = 0;
    st[0] = next[0] = '\0';
    text_value(answer, "state", st, sizeof(st));
    text_value(answer, "match", next, sizeof(next));
    role = number_value(answer, "role");
    SDL_LockMutex(sLock);
    if (act != sFlowAct || strcmp(match, sFlowMatch) != 0) { /* the game asked for something else meanwhile */
        SDL_UnlockMutex(sLock);
        return;
    }
    if (is_hex(next, 32) && strcmp(next, match) != 0 && (role == 1 || role == 2)) { /* a NEW match: both accepted */
        SDL_snprintf(text, sizeof(text), "%s %d %d 0\n", next, role, mode);
        write_text("dragonnet-next.txt", text);
        sFlowState = 5;
        sFlowAct = 0;
    } else if (strcmp(st, "declined") == 0) {
        sFlowState = 4;
        sFlowAct = 0;
    } else if (strcmp(st, "waiting") == 0) {
        sFlowState = 3;
        if (act == 2) {
            sFlowAct = 1; /* the vote is in: from now on only ask */
        }
    } else if (strcmp(st, "choice") == 0) {
        sFlowState = 2;
        if (act == 2) {
            sFlowAct = 1;
        }
    } else {
        sFlowState = 1; /* confirming: the service does not have both results yet (accept is repeated) */
    }
    SDL_UnlockMutex(sLock);
}

'''

C_PUBLIC = r'''/* flow:2 rematch (the game's side). The match is the one of the running session (dragonnet-session.txt). */
void DrRanked_FlowAct(int act) {
    char match[33];
    int role, mode, sent;
    if (act < 1 || act > 3 || !session_read(match, &role, &mode, &sent)) {
        return;
    }
    SDL_LockMutex(sLock);
    if (sView.flow && strcmp(sView.match, match) == 0) {
        if (strcmp(sFlowMatch, match) != 0) {
            SDL_strlcpy(sFlowMatch, match, sizeof(sFlowMatch));
            sFlowState = 0;
        }
        if (sFlowState == 0) {
            sFlowState = 1;
        }
        sFlowAct = act;
        sFlowNow = 1;
    }
    SDL_UnlockMutex(sLock);
    start(NULL);
}

int DrRanked_FlowState(void) {
    int s;
    SDL_LockMutex(sLock);
    s = sFlowState;
    SDL_UnlockMutex(sLock);
    return s;
}

int DrRanked_FlowNext(char *match, int *role, int *mode) {
    char text[160];
    int sent = 0;
    *role = *mode = 0;
    if (!read_text("dragonnet-next.txt", text, sizeof(text))) {
        return 0;
    }
    if (sscanf(text, "%32s %d %d %d", match, role, mode, &sent) < 3 || !is_hex(match, 32) || (*role != 1 && *role != 2)) {
        return 0;
    }
    return 1;
}

void DrRanked_FlowClear(void) {
    write_text("dragonnet-next.txt", "0 0 0 0\n"); /* not a valid match: reads as "none" */
    SDL_LockMutex(sLock);
    sFlowAct = sFlowNow = sFlowState = 0;
    sFlowMatch[0] = '\0';
    SDL_UnlockMutex(sLock);
}

'''

# (file, anchor, replacement, description)
EDITS = [
    (H_PATH, "void DrRanked_Open(const char *name);",
     H_DECLS + "void DrRanked_Open(const char *name);", ".h: declaracoes"),
    (C_PATH, "static int sLifeReady, sLifeStarted, sLifeTries;",
     "static int sLifeReady, sLifeStarted, sLifeTries;" + C_STATICS, ".c: variaveis"),
    (C_PATH, "static int SDLCALL worker(void *unused) {",
     C_DO_FLOW + "static int SDLCALL worker(void *unused) {", ".c: do_flow antes do worker"),
    (C_PATH, "do_lifecycle();",
     "do_lifecycle();\n        do_flow();", ".c: chamada no worker"),
    (C_PATH, "urgent = sRefresh ||",
     "urgent = sFlowNow || sRefresh ||", ".c: acorda a thread"),
    (C_PATH, "void DrRanked_Open(const char *name) {",
     C_PUBLIC + "void DrRanked_Open(const char *name) {", ".c: funcoes publicas"),
]


def main():
    apply = "--apply" in sys.argv
    texts = {}
    for path in (C_PATH, H_PATH):
        if not path.exists():
            print("FALHOU: nao achei", path, "(rode na raiz do repositorio)")
            return 1
        with open(path, encoding="utf-8", newline="") as f:
            texts[path] = f.read()
    for path, text in texts.items():
        if "DrRanked_FlowAct" in text:
            print("FALHOU: ja aplicado em", path.name, "- nada alterado")
            return 1
    bad = 0
    for path, old, new, what in EDITS:
        n = texts[path].count(old)
        print(("OK    " if n == 1 else "ERRO  ") + "%dx  %s" % (n, what))
        bad += n != 1
    if bad:
        print("\nNada foi alterado: algum trecho nao foi achado exatamente 1 vez.")
        return 1
    if not apply:
        print("\nModo teste: nada foi alterado. Rode com --apply para aplicar.")
        return 0
    for path, old, new, what in EDITS:
        texts[path] = texts[path].replace(old, new, 1)
    for path, text in texts.items():
        with open(path, "w", encoding="utf-8", newline="") as f:
            f.write(text)
    print("\nAplicado. Agora: git diff --stat")
    return 0


if __name__ == "__main__":
    sys.exit(main())
