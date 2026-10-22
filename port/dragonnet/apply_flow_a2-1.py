#!/usr/bin/env python3
# A2: o flow:2 sobrevive ao relaunch (execv) e o `ready` sai antes dele. So net_dragon.c.
# Modo teste por padrao; --apply aplica. Falha sem alterar NADA se algum trecho nao for achado 1x.
import sys

APPLY = "--apply" in sys.argv
C = "port/src/gs/net_dragon.c"

HELPER = '''/* The game restarts (relaunch = execv) between the lobby and the fight, and again after the session: the new process
   knows nothing of sView. Connection(1) leaves the match in dragonnet-flow.txt, which says "this match is flow:2". */
static int flow_file(const char *match) {
    char text[64], saved[33];
    if (!read_text("dragonnet-flow.txt", text, sizeof(text))) {
        return 0;
    }
    return sscanf(text, "%32s", saved) == 1 && strcmp(saved, match) == 0;
}

static void connection_queue(int ready) {
'''

NEWCONN = '''/* The lobby is connected (flow:2). The game restarts right after (relaunch), so the worker could not be trusted to
   send `ready` in time: it is woken, and this waits (at most 2.5 s) until it has been sent. */
void DrRanked_Connection(int ready) {
    static char done[33];
    char match[33], line[64];
    int i, pending, flow;
    if (!ready) {
        return;
    }
    SDL_LockMutex(sLock);
    flow = sView.flow && is_hex(sView.match, 32);
    SDL_strlcpy(match, sView.match, sizeof(match));
    SDL_UnlockMutex(sLock);
    if (!flow || strcmp(done, match) == 0) {
        return;
    }
    SDL_strlcpy(done, match, sizeof(done));
    SDL_snprintf(line, sizeof(line), "%s 1\\n", match);
    write_text("dragonnet-flow.txt", line);
    connection_queue(1);
    for (i = 0; i < 250; i++) {
        SDL_LockMutex(sLock);
        pending = sLifeReady && strcmp(sLifeMatch, match) == 0;
        SDL_UnlockMutex(sLock);
        if (!pending) {
            break;
        }
        SDL_Delay(10);
    }
}

'''

EDITS = [
    ("Connection antiga vira connection_queue",
     "void DrRanked_Connection(int ready) {\n",
     HELPER),
    ("Connection nova antes do FightStarted",
     "void DrRanked_FightStarted(void) {\n",
     NEWCONN + "void DrRanked_FightStarted(void) {\n"),
    ("FightStarted le o flow do arquivo",
     "ok = sView.flow && strcmp(sView.match, match) == 0 && strcmp(sStartSent, match) != 0;",
     "ok = (flow_file(match) || (sView.flow && strcmp(sView.match, match) == 0)) && strcmp(sStartSent, match) != 0;"),
    ("FightStarted reenvia o ready",
     "        sLifeStarted = 1;\n        sLifeTries = 0;\n",
     "        sLifeReady = 1; /* again: the process that sent it may be gone (the service takes a repeat) */\n"
     "        sLifeStarted = 1;\n        sLifeTries = 0;\n"),
    ("SessionEnding le o flow do arquivo",
     "flow = sent && sView.flow && strcmp(sView.match, match) == 0;",
     "flow = sent && (flow_file(match) || (sView.flow && strcmp(sView.match, match) == 0));"),
    ("FlowAct le o flow do arquivo",
     "if (after || (sView.flow && strcmp(sView.match, match) == 0)) {",
     "if (after || flow_file(match) || (sView.flow && strcmp(sView.match, match) == 0)) {"),
    ("worker acorda para ready/started",
     "sRoomPending[0] != '\\0' || sChatOutN > 0",
     "sRoomPending[0] != '\\0' || sChatOutN > 0 || sLifeReady || sLifeStarted"),
]

try:
    text = open(C, encoding="utf-8").read()
except OSError as e:
    print("ERRO: nao abri", C, e)
    sys.exit(1)

if "flow_file(" in text:
    print("ERRO: ja aplicado (flow_file existe). Nada foi alterado.")
    sys.exit(1)

bad = False
for name, old, new in EDITS:
    n = text.count(old)
    print("OK  " if n == 1 else "ERRO", "%dx" % n, name)
    if n != 1:
        bad = True
if bad:
    print("\nERRO: nada foi alterado.")
    sys.exit(1)

if not APPLY:
    print("\nModo teste: nada foi alterado. Rode com --apply para aplicar.")
    sys.exit(0)

for name, old, new in EDITS:
    text = text.replace(old, new, 1)
open(C, "w", encoding="utf-8").write(text)
print("\nAplicado. Agora: git diff --stat")
