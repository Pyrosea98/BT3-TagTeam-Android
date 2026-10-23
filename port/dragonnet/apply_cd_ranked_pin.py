import sys
from pathlib import Path

M = Path("port/src/gs/net_match.c")
m = M.read_text()
if "rk_pin_get" in m:
    sys.exit("ja aplicado")
if "static int rk_post(" not in m:
    sys.exit("rode antes apply_cd_ranked.py")

NEW = r'''/* a partida ranqueada fica no servidor (C ou D) em que o servico a colocou */
#define RK_PINS 2
static char sRkPinMatch[RK_PINS][33];
static char sRkPinBack[RK_PINS]; /* 'C' ou 'D' */
static int sRkPinNext;
static SDL_AtomicInt sRkPinBusy;

static void rk_pin_lock(void) {
    while (!SDL_CompareAndSwapAtomicInt(&sRkPinBusy, 0, 1)) {
        SDL_Delay(1);
    }
}

static void rk_pin_unlock(void) {
    SDL_SetAtomicInt(&sRkPinBusy, 0);
}

/* acha "match":"<32 hex>" no JSON (pedido ou resposta) */
static int rk_match_of(const char *json, char *match) {
    const char *s = SDL_strstr(json, "\"match\":\"");
    int i;
    if (s == NULL) {
        return 0;
    }
    s += 9;
    for (i = 0; i < 32; i++) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return 0;
        }
        match[i] = c;
    }
    if (s[32] != '"') {
        return 0;
    }
    match[32] = '\0';
    return 1;
}

static char rk_pin_get(const char *match) {
    char back = 0;
    int i;
    rk_pin_lock();
    for (i = 0; i < RK_PINS; i++) {
        if (sRkPinBack[i] != 0 && SDL_strcmp(sRkPinMatch[i], match) == 0) {
            back = sRkPinBack[i];
        }
    }
    rk_pin_unlock();
    return back;
}

static void rk_pin_set(const char *match, char back) {
    int i;
    rk_pin_lock();
    for (i = 0; i < RK_PINS; i++) {
        if (sRkPinBack[i] != 0 && SDL_strcmp(sRkPinMatch[i], match) == 0) {
            rk_pin_unlock(); /* a partida fica onde foi colocada */
            return;
        }
    }
    i = sRkPinNext;
    sRkPinNext = (sRkPinNext + 1) % RK_PINS;
    SDL_strlcpy(sRkPinMatch[i], match, sizeof(sRkPinMatch[i]));
    sRkPinBack[i] = back;
    rk_pin_unlock();
    fprintf(stderr, "dragonnet: ranked match pinned to %c\n", back);
}

static int rk_post(const char *what, const char *json, char *out, size_t size) {
    char url[320], a[72], match[33], back[8];
    const char *base;
    char pin = 0;
    int ok, bad, shared;
    out[0] = '\0';
    if (DrNet_RankedUrl()[0] == '\0') {
        return 0; /* sem C: Ranked indisponivel */
    }
    if (rk_match_of(json, match)) {
        pin = rk_pin_get(match);
    }
    base = rk_pick();
    if (pin == 'D') {
        base = DrNet_RankedUrlD();
    } else if (pin == 'C') {
        base = DrNet_RankedUrl();
    }
    shared = rk_verify(base);
    if (shared < 0) {
        return 0;
    }
    SDL_snprintf(url, sizeof(url), "%s/v1/%s", base, what);
    ok = http_post_native(url, json, NULL, out, size);
    bad = rk_bad(ok, out);
    rk_note(base, !bad, rk_quota(out));
    if (!bad && shared && rk_text(out, "authority", a, sizeof(a)) && SDL_strcmp(a, sRkAuth) != 0) {
        out[0] = '\0';
        return 0;
    }
    if (!bad && shared && rk_match_of(out, match) && rk_text(out, "backend", back, sizeof(back)) &&
        (back[0] == 'C' || back[0] == 'D') && back[1] == '\0') {
        rk_pin_set(match, back[0]);
    }
    return ok;
}

'''

i = m.index("static int rk_post(")
j = m.index("/* Dragon Net (net_dragon.c): a POST to its service")
if j < i:
    sys.exit("ordem inesperada no net_match.c")
M.write_text(m[:i] + NEW + m[j:])
print("OK: rk_post agora fixa o backend por partida")
