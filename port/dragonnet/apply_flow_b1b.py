#!/usr/bin/env python3
"""B1b: the Revanche after the session. Only port/src/gs/net_dragon.c.

- DrNet_SessionEnding saves the match of a flow:2 session whose result was recorded
  (dragonnet-prev.txt), because it erases dragonnet-session.txt right after.
- DrRanked_FlowAct falls back to dragonnet-prev.txt when there is no session any more.
- The mode comes from that file (sView may not survive the return), not from sView.

Test mode by default (changes NOTHING). --apply writes.
If any anchor is not found exactly once, nothing is changed.
Run from the repo root:  python3 port/dragonnet/apply_flow_b1b.py [--apply]
"""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
C_PATH = ROOT / "port/src/gs/net_dragon.c"

C_PREV_READ = r'''/* flow:2: the match of the session that just ended (dragonnet-prev.txt, written by DrNet_SessionEnding). */
static int prev_read(char *match, int *role, int *mode) {
    char text[160];
    int sent = 0;
    *role = *mode = 0;
    if (!read_text("dragonnet-prev.txt", text, sizeof(text))) {
        return 0;
    }
    return sscanf(text, "%32s %d %d %d", match, role, mode, &sent) >= 3 && is_hex(match, 32) && (*role == 1 || *role == 2);
}

'''

OLD_FLOWACT = r'''    int role, mode, sent;
    if (act < 1 || act > 3 || !session_read(match, &role, &mode, &sent)) {
        return;
    }
    SDL_LockMutex(sLock);
    if (sView.flow && strcmp(sView.match, match) == 0) {
'''

NEW_FLOWACT = r'''    int role, mode, sent, after = 0;
    if (act < 1 || act > 3) {
        return;
    }
    if (!session_read(match, &role, &mode, &sent)) { /* no session any more: the one that just ended */
        after = prev_read(match, &role, &mode);
        if (!after) {
            return;
        }
    }
    SDL_LockMutex(sLock);
    if (after || (sView.flow && strcmp(sView.match, match) == 0)) {
        sFlowMode = mode;
'''

C_SESSION_END = r'''    { /* flow:2: after the session the Revanche screen asks the service about this match */
        int flow;
        SDL_LockMutex(sLock);
        flow = sent && sView.flow && strcmp(sView.match, match) == 0;
        SDL_UnlockMutex(sLock);
        if (flow) {
            SDL_snprintf(line, sizeof(line), "%s %d %d 1\n", match, role, mode);
            write_text("dragonnet-prev.txt", line);
        } else {
            remove_file("dragonnet-prev.txt");
        }
    }
'''

# (anchor, replacement, description)
EDITS = [
    ("static Uint64 sFlowAt;",
     "static Uint64 sFlowAt;\nstatic int sFlowMode;", "variavel sFlowMode"),
    ("    SDL_strlcpy(match, sFlowMatch, sizeof(match));\n    mode = sView.mode;",
     "    SDL_strlcpy(match, sFlowMatch, sizeof(match));\n    mode = sFlowMode;", "do_flow usa sFlowMode"),
    ("/* flow:2 rematch (the game's side). The match is the one of the running session (dragonnet-session.txt). */",
     C_PREV_READ + "/* flow:2 rematch (the game's side). The match is the one of the running session (dragonnet-session.txt). */",
     "prev_read"),
    (OLD_FLOWACT, NEW_FLOWACT, "FlowAct le o prev"),
    ('    write_text("dragonnet-next.txt", "0 0 0 0\\n"); /* not a valid match: reads as "none" */',
     '    write_text("dragonnet-next.txt", "0 0 0 0\\n"); /* not a valid match: reads as "none" */\n'
     '    write_text("dragonnet-prev.txt", "0 0 0 0\\n");', "FlowClear limpa o prev"),
    ("    if (!sent && mine) {",
     C_SESSION_END + "    if (!sent && mine) {", "SessionEnding grava o prev"),
]


def main():
    apply = "--apply" in sys.argv
    if not C_PATH.exists():
        print("FALHOU: nao achei", C_PATH, "(rode na raiz do repositorio)")
        return 1
    with open(C_PATH, encoding="utf-8", newline="") as f:
        text = f.read()
    if "DrRanked_FlowAct" not in text:
        print("FALHOU: a B1 (apply_flow_b1) nao esta aplicada - nada alterado")
        return 1
    if "prev_read" in text:
        print("FALHOU: B1b ja aplicada - nada alterado")
        return 1
    bad = 0
    for old, new, what in EDITS:
        n = text.count(old)
        print(("OK    " if n == 1 else "ERRO  ") + "%dx  %s" % (n, what))
        bad += n != 1
    if bad:
        print("\nNada foi alterado: algum trecho nao foi achado exatamente 1 vez.")
        return 1
    if not apply:
        print("\nModo teste: nada foi alterado. Rode com --apply para aplicar.")
        return 0
    for old, new, what in EDITS:
        text = text.replace(old, new, 1)
    with open(C_PATH, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    print("\nAplicado. Agora: git diff --stat")
    return 0


if __name__ == "__main__":
    sys.exit(main())
