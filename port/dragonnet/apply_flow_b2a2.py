#!/usr/bin/env python3
# B2a-2: tela Revanche (REMATCH) + interceptacao no fim da sessao + retomada no boot.
# Arquivos: net_dragon.c/.h, ui_dragonnet.inc, ui.cpp, src/menu/menu_a.c.
# Modo teste por padrao; --apply aplica. Falha sem alterar NADA se algum trecho nao for achado 1x.
import sys

APPLY = "--apply" in sys.argv
C = "port/src/gs/net_dragon.c"
H = "port/src/gs/net_dragon.h"
INC = "port/src/gs/ui_dragonnet.inc"
UI = "port/src/gs/ui.cpp"
MENU = "src/menu/menu_a.c"

C_OFFER = '''/* flow:2: the game flags for the Revanche screen (ui.cpp shows it while MainMenu_Run waits, before the session ends). */
volatile int gPortRematchRequest, gPortRematchDone, gPortRematchUi;

/* Is the Revanche screen to be offered now: a session whose result is already registered, and that is flow:2? */
int DrRanked_RematchOffer(void) {
    char match[33];
    int role, mode, sent;
    if (!gPortRematchUi || !session_read(match, &role, &mode, &sent) || !sent) {
        return 0;
    }
    return flow_file(match);
}

int DrRanked_InSession(void) {
'''

INC_SCREEN = '''// ---- Rematch (flow:2): after the result of a ranked match. While it is open the game waits in MainMenu_Run
// (menu_a.c); gPortRematchDone says how it ended: 1 = leave (the online menu), 2 = a new match was saved
// (dragonnet-next.txt: the game restarts and rematch_boot() picks it up).
extern volatile int gPortRematchRequest, gPortRematchDone;
extern volatile int gPortNetWindowClose;
static Uint64 sRematchSince, sRematchAccept, sRematchLeave;
static bool sRematchBooted;

static void rematch_open(void) {
    sRematchSince = SDL_GetTicks();
    sRematchAccept = sRematchLeave = 0;
    DrRanked_FlowAct(1);
    goto_screen(REMATCH, 0);
}

static void rematch_done(int how) {
    gPortRematchDone = how;
    gPortNetWindowClose = 1; // the window closes by the same way as when a session ends
}

// The game has restarted after a rematch was agreed: the new match is resumed as a paired search and the lobby takes over.
static void rematch_boot(void) {
    char m[33];
    int r, md;
    if (!DrRanked_FlowNext(m, &r, &md)) {
        return;
    }
    DrRanked_Open(sName);
    if (DrRanked_Resume()) {
        net_open();
        sFoundAt = 0;
        sLobbyLast = 0;
        goto_screen(RANKED_WAIT, 0);
    }
}

static void screen_rematch(void) {
    const float u = U();
    const Uint64 now = SDL_GetTicks();
    const int st = DrRanked_FlowState();
    const bool back = header(T("Rematch", "Revanche", "Revancha"), true) || padB();
    if (!sRematchLeave && back) { // B / back: the rival is told, then the game goes on to the online menu
        DrRanked_FlowAct(3);
        sRematchLeave = now;
    }
    if (st == 5) { // both accepted: the new match is saved
        rematch_done(2);
        return;
    }
    if (sRematchLeave) {
        if (st == 4 || now - sRematchLeave > 2500) {
            rematch_done(1);
            return;
        }
    } else if (st == 4) {
        rematch_done(1);
        return;
    } else if (!sRematchAccept && st < 2 && now - sRematchSince > 20000) { // the service does not answer
        rematch_done(1);
        return;
    } else if (sRematchAccept && now - sRematchAccept > 15000) { // the rival did not accept in time
        DrRanked_FlowAct(3);
        sRematchLeave = now;
    }
    const char *big;
    const char *note;
    if (sRematchLeave) {
        big = T("LEAVING", "SAINDO", "SALIENDO");
        note = T("Telling the rival...", "Avisando o rival...", "Avisando al rival...");
    } else if (sRematchAccept) {
        big = T("WAITING", "AGUARDANDO", "ESPERANDO");
        note = T("Waiting for the rival to accept...", "Esperando o rival aceitar...", "Esperando que el rival acepte...");
    } else if (st == 2) {
        big = T("REMATCH?", "REVANCHE?", "¿REVANCHA?");
        note = T("Both must accept. The result is saved.", "Os dois precisam aceitar. O resultado ja esta salvo.",
                 "Ambos deben aceptar. El resultado ya esta guardado.");
    } else {
        big = T("CONFIRMING", "CONFIRMANDO", "CONFIRMANDO");
        note = T("Confirming the result...", "Confirmando o resultado...", "Confirmando el resultado...");
    }
    wait_card(u * 3.3f, T("Online match", "Partida online", "Partida en linea"), big, note, col(1.0f, 0.86f, 0.30f));
    if (st == 2 && !sRematchAccept && !sRematchLeave) {
        card_start(u * 9.2f, u * 3.3f, NULL);
        int m = padMove();
        if (m != 0) {
            sRow = (sRow + m + 2) % 2;
        }
        bool again = card_row(0, u * 0.35f, T("Rematch", "Revanche", "Revancha"), "") || (sRow == 0 && padA());
        bool menu = card_row(1, u * 1.9f, T("Online menu", "Menu online", "Menu en linea"), "") || (sRow == 1 && padA());
        if (again) {
            DrRanked_FlowAct(2);
            sRematchAccept = now;
        }
        if (menu) {
            DrRanked_FlowAct(3);
            sRematchLeave = now;
        }
    }
    footer(T("A: choose", "A: escolher", "A: elegir"), "", T("B: back", "B: voltar", "B: volver"));
}

static void screen_chat(void) {
'''

MENU_BLOCK = '''        { /* Dragon Net (flow:2): the Revanche screen (port/src/gs/ui.cpp) comes before the session ends */
            extern volatile int gPortRematchRequest, gPortRematchDone;
            extern int DrRanked_RematchOffer(void);
            if (DrRanked_RematchOffer()) {
                int frames;
                gPortRematchDone = 0;
                gPortRematchRequest = 1;
                for (frames = 0; frames < 3000 && !gPortRematchDone; frames++) {
                    Gfx_BeginFrame();
                    Pad_Update();
                    Snd_Update();
                    Gfx_EndFrame(1);
                    Dma_Flush();
                }
            }
        }
        Port_NetLeave(); /* an online session lives'''

EDITS = [
    (C, "net_dragon.c: RematchOffer", "int DrRanked_InSession(void) {\n", C_OFFER),
    (H, "net_dragon.h: declaracao",
     "int DrRanked_Resume(void);\n",
     "int DrRanked_Resume(void);\n"
     "/* flow:2: should the Revanche screen come before the session ends (1 = yes). */\n"
     "int DrRanked_RematchOffer(void);\n"),
    (INC, "inc: enum Screen", "BOARD, CHAT };", "BOARD, CHAT, REMATCH };"),
    (INC, "inc: despachante", "case CHAT: screen_chat(); break;",
     "case CHAT: screen_chat(); break;\n        case REMATCH: screen_rematch(); break;"),
    (INC, "inc: tela e funcoes", "static void screen_chat(void) {\n", INC_SCREEN),
    (UI, "ui.cpp: pedido da revanche e retomada",
     "        gPortNetMenuRequest = 0;\n        net_open();\n",
     "        gPortNetMenuRequest = 0;\n        net_open();\n"
     "    } else if (sReady && gPortRematchRequest) { // flow:2: the Revanche screen, after the result of a ranked match\n"
     "        gPortRematchRequest = 0;\n        net_open();\n        rematch_open();\n"
     "    } else if (sReady && !sRematchBooted) { // after a rematch the game restarted: its new match waits in dragonnet-next.txt\n"
     "        sRematchBooted = true;\n        rematch_boot();\n"),
    (UI, "ui.cpp: gPortRematchUi", "    sReady = true;\n",
     "    sReady = true;\n    extern volatile int gPortRematchUi;\n    gPortRematchUi = 1; // the overlay exists: the game may wait for the Revanche screen\n"),
    (MENU, "menu_a.c: espera a tela antes do Port_NetLeave",
     "        Port_NetLeave(); /* an online session lives", MENU_BLOCK),
]

texts = {}
for path in (C, H, INC, UI, MENU):
    try:
        texts[path] = open(path, encoding="utf-8").read()
    except OSError as e:
        print("ERRO: nao abri", path, e)
        sys.exit(1)

if "screen_rematch" in texts[INC] or "DrRanked_RematchOffer" in texts[C]:
    print("ERRO: ja aplicado. Nada foi alterado.")
    sys.exit(1)

bad = False
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
for path in (C, H, INC, UI, MENU):
    open(path, "w", encoding="utf-8").write(texts[path])
print("\nAplicado. Agora: git diff --stat")
