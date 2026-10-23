import re, sys
from pathlib import Path

M = Path("port/src/gs/net_match.c")
D = Path("port/src/gs/net_dragon.c")
H = Path("port/src/gs/net_dragon.h")
m, d, h = M.read_text(), D.read_text(), H.read_text()
if "rk_post" in m:
    sys.exit("ja aplicado")

HDR = r'''const char *DrNet_RankedUrlD(void);      /* dragonnet_ranked_url_d.txt (https://host); padrao: D embutido */
const char *DrNet_RankedActiveUrl(void); /* Ranked: C, ou D ate 00:00 UTC se o C falhar */
int DrNet_CdLoad(char *out, int n);      /* id da autoridade E aprendido de um C verificado */
int DrNet_CdSave(const char *authority);
'''

BLOCK = r'''/* ---- Ranked C/D (DBT-NX v1.7): C primeiro; D so depois que o C provou a autoridade E ---- */
#ifndef DRNET_URL_RANKED_D
#define DRNET_URL_RANKED_D "https://bt3-net-rooms.alexsanchez01es.workers.dev"
#endif

static int rk_host_ok(const char *u) {
    const char *p;
    size_t n = SDL_strlen(u);
    if (n < 12 || n > 120 || SDL_strncmp(u, "https://", 8) != 0) {
        return 0;
    }
    for (p = u + 8; *p != '\0'; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '.' || *p == '-')) {
            return 0;
        }
    }
    return 1;
}

const char *DrNet_RankedUrlD(void) {
    static char f[256];
    static int read;
    if (!read) {
        read = 1;
        url_file("dragonnet_ranked_url_d.txt", f, sizeof(f));
        if (f[0] == '\0' || !rk_host_ok(f)) {
            SDL_strlcpy(f, DRNET_URL_RANKED_D, sizeof(f));
        }
    }
    return f;
}

static SDL_AtomicInt sRkUseD, sRkDay, sRkFails, sRkShared, sRkVerC, sRkVerD;
static char sRkAuth[72];

static int rk_text(const char *json, const char *key, char *out, size_t cap) {
    char field[64];
    const char *s;
    size_t n = 0;
    SDL_snprintf(field, sizeof(field), "\"%s\":", key);
    s = SDL_strstr(json, field);
    if (s == NULL) {
        return 0;
    }
    s += SDL_strlen(field);
    while (*s == ' ') {
        s++;
    }
    if (*s++ != '"') {
        return 0;
    }
    while (*s != '\0' && *s != '"' && n + 1 < cap) {
        if (*s == '\\') {
            return 0;
        }
        out[n++] = *s++;
    }
    out[n] = '\0';
    return *s == '"';
}

static int rk_quota(const char *out) {
    return out[0] != '{' && (SDL_strstr(out, "1027") != NULL || SDL_strstr(out, "1015") != NULL ||
                             SDL_strstr(out, "Too Many Requests") != NULL);
}

static int rk_bad(int ok, const char *out) {
    return !ok || out[0] != '{' || SDL_strstr(out, "\"ranked unavailable\"") != NULL ||
           SDL_strstr(out, "\"migration incomplete\"") != NULL;
}

static const char *rk_pick(void) {
    if (SDL_GetAtomicInt(&sRkUseD)) {
        if (SDL_GetAtomicInt(&sRkDay) != utc_day()) {
            SDL_SetAtomicInt(&sRkUseD, 0); /* novo dia UTC: volta ao C */
            SDL_SetAtomicInt(&sRkFails, 0);
            fprintf(stderr, "dragonnet: ranked back to C\n");
        } else {
            return DrNet_RankedUrlD();
        }
    }
    return DrNet_RankedUrl();
}

/* so o C conta; o D falhando nao volta ao C no mesmo dia (como o B) */
static void rk_note(const char *base, int ok, int exhausted) {
    if (SDL_strcmp(base, DrNet_RankedUrl()) != 0 || !SDL_GetAtomicInt(&sRkShared) || SDL_GetAtomicInt(&sRkUseD)) {
        return;
    }
    if (ok) {
        SDL_SetAtomicInt(&sRkFails, 0);
        return;
    }
    if (exhausted || SDL_AddAtomicInt(&sRkFails, 1) + 1 >= DRNET_FAILS) {
        SDL_SetAtomicInt(&sRkDay, utc_day());
        SDL_SetAtomicInt(&sRkUseD, 1);
        SDL_SetAtomicInt(&sRkFails, 0);
        fprintf(stderr, "dragonnet: ranked C failed, using D until 00:00 UTC\n");
    }
}

/* 1 = C/D compartilhado, 0 = C antigo (so C), -1 = inalcancavel ou inseguro */
static int rk_verify(const char *base) {
    char url[320], out[2048], a[72], c[256], d[256];
    int ok, isD = SDL_strcmp(base, DrNet_RankedUrl()) != 0;
    if (!SDL_GetAtomicInt(&sRkShared) && DrNet_CdLoad(a, (int)sizeof(a))) {
        SDL_strlcpy(sRkAuth, a, sizeof(sRkAuth));
        SDL_SetAtomicInt(&sRkShared, 1);
    }
    if (isD) {
        if (!SDL_GetAtomicInt(&sRkShared)) {
            return -1;
        }
        if (SDL_GetAtomicInt(&sRkVerD)) {
            return 1;
        }
    } else {
        if (SDL_GetAtomicInt(&sRkVerC) == 1) {
            return 1;
        }
        if (SDL_GetAtomicInt(&sRkVerC) == 2) {
            return 0;
        }
    }
    SDL_snprintf(url, sizeof(url), "%s/v1/ranked/capabilities", base);
    out[0] = '\0';
    ok = http_post_native(url, "{\"v\":5001,\"cd\":1}", NULL, out, sizeof(out));
    if (rk_bad(ok, out)) {
        rk_note(base, 0, rk_quota(out));
        return -1;
    }
    if (SDL_strstr(out, "\"cd\":1") == NULL || SDL_strstr(out, "\"migration_complete\":1") == NULL) {
        if (!isD && !SDL_GetAtomicInt(&sRkShared)) {
            SDL_SetAtomicInt(&sRkVerC, 2); /* C antigo: segue so com o C */
            return 0;
        }
        return -1;
    }
    if (!rk_text(out, "authority", a, sizeof(a)) || !rk_text(out, "url_c", c, sizeof(c)) ||
        !rk_text(out, "url_d", d, sizeof(d)) || SDL_strcmp(c, DrNet_RankedUrl()) != 0 ||
        SDL_strcmp(d, DrNet_RankedUrlD()) != 0) {
        return -1;
    }
    if (SDL_GetAtomicInt(&sRkShared)) {
        if (SDL_strcmp(a, sRkAuth) != 0) {
            fprintf(stderr, "dragonnet: ranked authority differs: refused\n");
            return -1;
        }
    } else if (!isD) {
        if (!DrNet_CdSave(a)) {
            return -1;
        }
        SDL_strlcpy(sRkAuth, a, sizeof(sRkAuth));
        SDL_SetAtomicInt(&sRkShared, 1);
    } else {
        return -1;
    }
    SDL_SetAtomicInt(isD ? &sRkVerD : &sRkVerC, 1);
    fprintf(stderr, "dragonnet: ranked c/d verified (%s)\n", isD ? "D" : "C");
    return 1;
}

/* salas ranqueadas: o D so entra depois de verificado */
const char *DrNet_RankedActiveUrl(void) {
    const char *base = rk_pick();
    if (SDL_strcmp(base, DrNet_RankedUrl()) != 0 && !SDL_GetAtomicInt(&sRkVerD)) {
        return DrNet_RankedUrl();
    }
    return base;
}

static int rk_post(const char *what, const char *json, char *out, size_t size) {
    char url[320], a[72];
    const char *base;
    int ok, bad, shared;
    out[0] = '\0';
    if (DrNet_RankedUrl()[0] == '\0') {
        return 0; /* sem C: Ranked indisponivel */
    }
    base = rk_pick();
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
    return ok;
}

'''

DRG = r'''/* ---- Ranked C/D: id da autoridade E aprendido de um C verificado (DBT-NX v1.7) ---- */
static int cd_id_ok(const char *s) {
    size_t n = 0;
    for (; s[n] != '\0'; n++) {
        char c = s[n];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return 0;
        }
    }
    return n >= 16 && n <= 64;
}

int DrNet_CdLoad(char *out, int n) {
    char text[96];
    int i = 0;
    out[0] = '\0';
    if (!read_text("dragonnet-ranked-authority.txt", text, sizeof(text))) {
        return 0;
    }
    while (text[i] != '\0' && text[i] != '\n' && text[i] != '\r' && i + 1 < n) {
        out[i] = text[i];
        i++;
    }
    out[i] = '\0';
    if (!cd_id_ok(out)) {
        out[0] = '\0';
        return 0;
    }
    return 1;
}

int DrNet_CdSave(const char *authority) {
    char text[96];
    if (!cd_id_ok(authority)) {
        return 0;
    }
    SDL_snprintf(text, sizeof(text), "%s\n", authority);
    return write_text("dragonnet-ranked-authority.txt", text);
}
'''

h, n = re.subn(r'(const char \*DrNet_RankedUrl\(void\);[^\n]*\n)', lambda x: x.group(1) + HDR, h, count=1)
if n != 1: sys.exit("ancora do net_dragon.h nao achada")

marker = "/* Dragon Net (net_dragon.c): a POST to its service"
if m.count(marker) != 1: sys.exit("ancora do Match_PostDragon nao achada")
m = m.replace(marker, BLOCK + marker)

old = "SDL_strlcpy(sM.url, DrNet_RankedUrl(), sizeof(sM.url));"
if m.count(old) != 1: sys.exit("ancora do sM.url nao achada")
m = m.replace(old, "SDL_strlcpy(sM.url, DrNet_RankedActiveUrl(), sizeof(sM.url));")

pat = re.compile(r"""if \(SDL_strncmp\(what, "ranked/", 7\) == 0\) \{\s*base = DrNet_RankedUrl\(\);\s*if \(base\[0\] == '\\0'\) \{\s*return 0;[^\n]*\n\s*\}\s*\} else if \(presence\) \{""")
new = 'if (SDL_strncmp(what, "ranked/", 7) == 0) {\n        return rk_post(what, json, out, size);\n    } else if (presence) {'
m, n = pat.subn(lambda x: new, m)
if n != 1: sys.exit("ancora do POST ranked nao achada")

d = d.rstrip("\n") + "\n\n" + DRG
M.write_text(m); D.write_text(d); H.write_text(h)
print("OK: net_match.c, net_dragon.c e net_dragon.h alterados")
