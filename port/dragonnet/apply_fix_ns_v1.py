#!/usr/bin/env python3
# Corrige o namespace dn da revanche (ui.cpp + ui_dragonnet.inc).
# Modo teste por padrao. Use --apply para gravar.
# Se qualquer trecho nao for achado EXATAMENTE 1 vez, nao altera NADA.
import os, sys

APPLY = "--apply" in sys.argv
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.environ.get("FLOW_ROOT") or os.path.abspath(os.path.join(HERE, "..", ".."))

UI = os.path.join(ROOT, "port/src/gs/ui.cpp")
INC = os.path.join(ROOT, "port/src/gs/ui_dragonnet.inc")

EDITS = {
    UI: [
        ("extern global em ui.cpp (fora do namespace dn)",
         "extern volatile int gPortNetWindowClose; // gs/state.c",
         "extern volatile int gPortNetWindowClose; // gs/state.c\n"
         "extern volatile int gPortRematchRequest, gPortRematchDone; // net_dragon.c (global C)"),
        ("rematch_open -> dn::", "rematch_open();", "dn::rematch_open();"),
        ("!sRematchBooted -> dn::", "!sRematchBooted)", "!dn::sRematchBooted)"),
        ("sRematchBooted = true -> dn::", "sRematchBooted = true;", "dn::sRematchBooted = true;"),
        ("rematch_boot -> dn::", "rematch_boot();", "dn::rematch_boot();"),
    ],
    INC: [
        ("remove externs de dentro do namespace dn",
         "extern volatile int gPortRematchRequest, gPortRematchDone;\n"
         "extern volatile int gPortNetWindowClose;\n",
         "// gPortRematchRequest/Done e gPortNetWindowClose: declarados no escopo global em ui.cpp.\n"
         "// Dentro de dn virariam outro simbolo (dn::...) e o link falharia.\n"),
    ],
}

def fail(msg):
    print("ERRO:", msg)
    print("Nada foi alterado.")
    sys.exit(1)

new_text = {}
for path, edits in EDITS.items():
    if not os.path.isfile(path):
        fail("arquivo nao encontrado: " + path)
    with open(path, "r", encoding="utf-8", newline="") as f:
        s = f.read()
    if "\r\n" in s:
        fail(path + " usa CRLF; script nao preparado")
    for desc, old, new in edits:
        n = s.count(old)
        if n != 1:
            fail("%s: trecho '%s' achado %d vez(es) em %s (esperado 1)" % (desc, old[:50], n, os.path.basename(path)))
    t = s
    for desc, old, new in edits:
        t = t.replace(old, new, 1)
        print("OK  1x", os.path.basename(path) + ":", desc)
    new_text[path] = t

if not APPLY:
    print("\nModo teste: tudo achado 1x. Nada gravado. Rode com --apply.")
    sys.exit(0)

for path, t in new_text.items():
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(t)
print("\nAplicado. Agora: git diff --stat")
