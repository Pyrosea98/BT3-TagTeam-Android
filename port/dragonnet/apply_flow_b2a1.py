#!/usr/bin/env python3
# B2a-1: DrRanked_Resume (retoma a partida nova da revanche). So net_dragon.c e net_dragon.h.
# Modo teste por padrao; --apply aplica. Falha sem alterar NADA se algum trecho nao for achado 1x.
import sys

APPLY = "--apply" in sys.argv
C = "port/src/gs/net_dragon.c"
H = "port/src/gs/net_dragon.h"

RESUME = '''/* flow:2 rematch accepted: the new match (dragonnet-next.txt) is already paired at the service. It resumes as a
   paired search: the worker's queue/room calls bring its code, and the lobby screen takes it from there. */
int DrRanked_Resume(void) {
    char text[160], match[33];
    int role = 0, mode = 0, sent = 0;
    if (!read_text("dragonnet-next.txt", text, sizeof(text))) {
        return 0;
    }
    if (sscanf(text, "%32s %d %d %d", match, &role, &mode, &sent) < 3 || !is_hex(match, 32) || (role != 1 && role != 2) || (mode != 0 && mode != 1)) {
        return 0;
    }
    start(NULL);
    SDL_LockMutex(sLock);
    sSearchGen++;
    sHandedOver[0] = '\\0';
    sRoomPending[0] = '\\0';
    sWantSearch = sView.searching = 1;
    sView.state = 3;
    sView.role = role;
    sView.mode = mode;
    sView.flow = 1;
    sView.code[0] = sView.peer[0] = '\\0';
    SDL_strlcpy(sView.match, match, sizeof(sView.match));
    sView.msg = DRMSG_FOUND;
    sFlowAct = sFlowNow = sFlowState = 0;
    sFlowMatch[0] = '\\0';
    SDL_UnlockMutex(sLock);
    write_text("dragonnet-next.txt", "0 0 0 0\\n");
    write_text("dragonnet-prev.txt", "0 0 0 0\\n");
    fprintf(stderr, "dragonnet: rematch resumed (role %d)\\n", role);
    return 1;
}

'''

EDITS = [
    (C, "Resume no .c",
     "void DrRanked_Open(const char *name) {\n    start(name);\n",
     RESUME + "void DrRanked_Open(const char *name) {\n    start(name);\n"),
    (H, "declaracao no .h",
     "void DrRanked_FlowClear(void);\n",
     "void DrRanked_FlowClear(void);\n"
     "/* flow:2 rematch: the saved new match becomes a paired search again (1 = resumed). */\n"
     "int DrRanked_Resume(void);\n"),
]

texts, bad = {}, False
for path in (C, H):
    try:
        texts[path] = open(path, encoding="utf-8").read()
    except OSError as e:
        print("ERRO: nao abri", path, e)
        sys.exit(1)

if "int DrRanked_Resume(void)" in texts[C] or "DrRanked_Resume" in texts[H]:
    print("ERRO: ja aplicado (DrRanked_Resume existe). Nada foi alterado.")
    sys.exit(1)

for path, name, old, new in EDITS:
    n = texts[path].count(old)
    print("OK  " if n == 1 else "ERRO", "%dx" % n, name)
    if n != 1:
        bad = True

if bad:
    print("\nERRO: nada foi alterado.")
    sys.exit(1)

if not APPLY:
    print("\nModo teste: nada foi alterado. Rode com --apply para aplicar.")
    sys.exit(0)

for path, name, old, new in EDITS:
    texts[path] = texts[path].replace(old, new, 1)
for path in (C, H):
    open(path, "w", encoding="utf-8").write(texts[path])
print("\nAplicado. Agora: git diff --stat")
