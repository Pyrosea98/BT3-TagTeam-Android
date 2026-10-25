# Dragon Rage 0.9.0: relatório da versão

Branch `release-0.9.0`, criado de `device-lab` (23fa0ef + 2649a87 + eefe5d9). Commits só locais: sem push e sem
tag. Datas: 24 e 25/10/2026. Os branches `bt4`, `main`, `armsx2` e `mali-lab` não foram tocados.

| Aparelho | SoC / GPU | Driver | Android |
|---|---|---|---|
| RG Cube | Unisoc T820, Mali-G57 MC4 | do sistema r40p0 (MaliSX2 0.2.3 no teste de drivers) | 13 (LineageOS GSI), root |
| Retroid Pocket 5 (RP5) | Snapdragon 865, Adreno 650 | Qualcomm 512.746.0 (Turnip no teste de drivers) | 13 |

Como foi medido: igual ao device lab (`docs/device-lab-2026-10.md`, seção "Como foi medido"). BT3 com a luta
gravada (`BT3_PAD_PLAY`), a mesma em todas as builds; BT4 com o roteiro que abre Duelo → Espectador pelo launcher;
imagem comparada pixel a pixel nos quadros 6300 a 7350 da luta gravada (um a cada 150).

## 1. Merge com o PC

- **Versão:** Tenkaichi3Decomp **v0.1.38.7** (remote `pc`), por cima da 0.1.37 do último merge. Commit
  **01224d0 "Merge PC 0.1.38.7"**; `PORT_RELEASE` = 138.
- **Como:** as versões do PC não têm ancestral comum com a nossa árvore, então o merge foi feito com
  `git merge-tree --write-tree --merge-base=v0.1.37 HEAD v0.1.38.7`, os conflitos resolvidos na working tree e
  o commit criado com os dois pais (HEAD e `v0.1.38.7`). Assim o próximo merge do PC já parte da 0.1.38.7.
- **O que veio** (0.1.37.1 a 0.1.38.7):
  - GPU: o brilho (glow) desenhado na resolução do PS2 (`gswb.frag`, bit 25 do pipeline), passadas sem buffer
    auxiliar (bit 26), menos trabalho por camada de tela cheia nos especiais, limpeza e cópias mais baratas;
  - texturas novas preparadas em vários núcleos; buffers de desenho prontos antes do primeiro grande efeito;
    contorno só onde há linha; pacotes de textura lidos em segundo plano (cache de até 1 GB, no máximo 4 MB por
    quadro);
  - imagem: glow certo em qualquer resolução, desfoque da distância como no console, contornos com a mesma
    espessura em qualquer proporção, camisa da Videl, tela branca do Gohan adolescente em Namek (52b675c),
    texturas 24 e 4/8 bits sem linhas velhas;
  - online: sons do rival com rollback, som de ki preso, Solar Flare, vozes japonesas, dessincronia vira aviso,
    jogos com dados diferentes não são pareados;
  - salvamento rápido, músicas em MP3/WAV/FLAC (`snd_ma.c` entra, `song_mp3.c` sai), menus que crescem em telas
    grandes, força separada de glow e glare, "Sharp stage blur", clima.
- **Conflitos resolvidos** (10 arquivos): `gs_core.c`, `gs_draw.c` (12 trechos), `gs_draw.h`, `gs_gl.c`,
  `gs_gpu.c`, `gs_input.c`, `gs_texpack.h`, `net.c`, `snd_adx.c`, `ui.cpp`. Regra usada: código do jogo e
  comportamento do PC ficam como no PC; ficam como aqui o renderizador e as otimizações Android (passes sem
  profundidade, refs de stencil das sombras, GL persist, ADPF, cache de RAM), `ui_android.inc`, o loader, o
  launcher, o Dragon Net e os ganchos do BT4 (`bt4_shim.c`). Bits de pipeline: PC 25 = Wb, 26 = noAux; nossos
  27 = Nd, 28 = depthless; o cache passou a `bt3_pipelines2.txt`.
- **Depois do merge** (6e57077): o `libbt4.so` não abria ("cannot locate symbol gPortNetWaitNs" e mais cinco
  símbolos novos do PC). Stubs no `bt4_shim.c`, e o `build_bt4.py` agora confere os símbolos indefinidos do
  `libbt4.so` depois do link (falha a build em vez de falhar no aparelho). De passagem: o `seam_widen` lia um
  `extern int gsScale` que o link resolvia para uma função do jogo (valor negativo, a costura ficava sempre em
  0,5); agora usa `gsScale4`.
- **Online depois do merge:** o modo PC fala com o servidor oficial da 0.1.38 (`PC: Working(mine=138,
  searching=3)` no RP5) e o Dragon Net continua com o Switch (`Dragon Net: Working(mine=138, searching=1)`).

### Imagem e velocidade antes e depois do merge (BT3, luta gravada)

| Aparelho, config | antes | depois | pixels diferentes |
|---|---|---|---|
| Cube, VK 1x | 100% (trabalho 20,9 ms) | 100% (21,8 ms) | **0 nos 8 quadros** |
| Cube, VK 2x | 86,8% (present 20,83 ms) | **95,0%** (present 17,35 ms) | n/a |
| Cube, GL 2x | 92,3% | **97,7%** | n/a |
| RP5, VK 2x | 100% | 99,7% | 169 mil a 774 mil por quadro, quase todos de 1 a 16 níveis; no máximo 4235 acima de 16 |
| RP5, VK 4x | 79,8% (present 22,97 ms) | **83,5%** (present 19,31 ms) | n/a |
| RP5, GL 2x | 100% | 100% | n/a |

- Em 1x a imagem é idêntica. Em 2x+ as diferenças são as mudanças de imagem **intencionais** do PC: o glow
  passou a ser desenhado na resolução do PS2 e ampliado (antes, uma cópia nítida e deslocada), o desfoque da
  distância tira detalhe como no console, os contornos têm a mesma espessura em qualquer proporção, e a costura
  (seam) agora usa a escala certa. Os mapas de diferença mostram halos de glow e bordas, não geometria.
- O que o PC otimizou na GPU ajudou mais o Mali: +8 pontos em VK 2x e +5 em GL 2x no Cube; +4 em VK 4x no RP5.

## 2. Desempenho final por aparelho, contra a 0.8.1

A base "0.8.1" é a build `armsx2` (950764a), a mesma base do device lab, com o motor da 0.8.1 e os mesmos
ajustes. "0.9.0" é a build final limpa (8d9d5fb, APK `final`), luta gravada do BT3.

| Aparelho, config | 0.8.1 | 0.9.0 | trabalho (ms) | present (ms) |
|---|---|---|---|---|
| Cube, VK 1x | 100% | 100% | 18,2 → 21,7 | 5,9 → 6,4 |
| Cube, **VK 2x** | 75,8% | **94,8%** | 43,4 → 33,7 | 25,3 → 17,2 |
| Cube, **GL 2x** | 92,5% | **97,8%** | 34,6 → 29,9 | n/a |
| RP5, VK 2x | 100% | 100% | 13,6 → 16,3 | 0,2 → 0,2 |
| RP5, **VK 4x** | 70,8% | **83,7%** | 46,0 → 38,6 | 28,9 → 19,2 |
| RP5, **GL 2x** | ~4 s por quadro | **100%** | n/a → 16,5 | n/a |
| RP5, VK 2x com Turnip axfl2-006 | n/a | 100% | 13,6 | 0,27 |

- O "trabalho" maior em 1x no Cube e em VK 2x no RP5 vem do motor da 0.1.38.7 (mais CPU por quadro: o `list` sobe
  de ~10 para ~13 ms no RP5), sem efeito na velocidade (100% nos dois). Onde a GPU é o limite (2x+ no Mali, 4x
  no Adreno), a 0.9.0 ganha de 5 a 19 pontos.
- Imagem: a build final dá **0 pixels diferentes** contra a build de antes do merge e contra a de depois no Cube
  (VK 1x, 8 quadros), e é idêntica à build pós-merge no RP5 (VK 2x). Contra a de antes do merge no RP5 ficam só as
  diferenças intencionais do PC descritas na seção 1.

## 3. Os itens da versão

1. **ARMSX2 é o renderizador padrão do BT4** (1bc64e2). O do Dragon Rage fica como opção "experimental", com uma
   linha que explica a diferença em Ajustes e no menu do jogo, nas três línguas. A descrição do BT4 na escolha do
   jogo também foi corrigida (dizia "desenhado pelo renderizador do Dragon Rage").
2. **Primeira abertura do ARMSX2** (b94a6fa). O APK traz o cache de SPIR-V do ARMSX2 já compilado: os caches do
   Cube (275 entradas) e do RP5 (235) juntados por `port/bt4/gsx/merge_cache.py` (504 entradas, 1,5 MB; as
   chaves GLSL dependem da GPU, só 6 são comuns). O launcher instala em `bt4/gsx/ARMSX2/cache` se lá não houver
   cache. Medido no Cube:

   | Abertura | primeira imagem | logos | página laranja | compilações |
   |---|---|---|---|---|
   | Sem cache | ~4,5 s | 13,2 s | 34,0 s | 81 numa sessão de luta (2,2 s no total, 20 a 50 ms cada, em menus e na luta) |
   | Cache do APK, primeira vez | **~2,5 s** | 11,1 s | 31,7 s | 0 na abertura, **1 numa luta inteira** |
   | Segunda abertura | ~2,5 s | 11,2 s | 31,9 s | n/a |

   Os ~25 s de tela preta do device lab eram o cache vazio mais a abertura do jogo. O que sobra (~30 s até a
   página laranja) é o boot do jogo e as logos, não shaders, por isso não fiz tela de "preparando shaders". Também
   saíram os dois avisos do OSD do ARMSX2 (configurações inseguras, `patches.zip` ausente).
3. **BT4 a 60 fps nas lutas.**
   - Runner (`PS2X_RUNNER_OPT`), medido no RP5 com os 60 presos (`BT4_FPS60_KEEP=1`), 144 s de luta cada:

     | Variante | guest_ms | atualizações da luta/s |
     |---|---|---|
     | -O2 (atual) | 12,45 / 12,66 (duas rodadas) | 37,0 / 36,1 |
     | -O3 | 12,62 | 36,1 |
     | ThinLTO | 12,52 | 36,2 |

     Nenhuma ganha além do ruído; fica -O2. PGO não foi feito: o runner é um `.so` de 100 MB gerado, e o perfil
     teria que vir de um build instrumentado rodando no RP5 com a luta; com -O3 e LTO sem ganho, o gargalo não
     parece ser de otimização do compilador.
   - **Vigia** (1bc64e2): com `bt4_fps60` ligado, se a luta fica entre 20 e 57 atualizações/s por 3 s seguidos, o
     jogo volta a 30, salva `bt4_fps60=0` e `bt4_fps60_dropped=1`, registra no diário do app e mostra "Este
     aparelho não aguenta 60 fps nas lutas do BT4; voltando para 30". Testado no Cube: recuou e avisou, a luta
     seguiu a 30 (guest_ms 20 a 23). A opção diz "(experimental)" nas três línguas; ligá-la de novo limpa o aviso.
   - Nenhum dos dois aparelhos segura 60: Cube ~30, RP5 36 a 41.
4. **Movimento suave no BT4 (renderizador Dragon Rage): não oferecido.** O movimento suave do BT3 faz quadros
   intermediários interpolando as matrizes do VU0 (`Vu0Uniform`) entre dois ticks. No BT4 o stream chega ao GS já
   transformado (o jogo não usa os uniforms do VU, `d->vu = 0`), então só sobraria a heurística de pares de
   triângulos dos efeitos/2D, que sozinha deforma o cenário. Além disso o laço do host do BT4 nunca chama
   `GsGpu_InterpPump` (só o `Port_VBlank` do `plat_stub.c` do BT3 chama). Sem imagem certa garantida, a opção não
   foi oferecida.
5. **Ruído no cenário distante (renderizador Dragon Rage, BT4): aberto.** Ver seção 4.
6. **Toque em telas 1:1** (81e16c6). No Cube (720x720) o controle CLÁSSICO tinha L2/L1 sobrepostos no canto e o
   círculo cortado na borda direita. Agora as pílulas de fora ficam nas bordas e as de dentro ao lado, e o losango e
   o direcional entram até caber. Em 16:9 nada muda (conferido no RP5). O controle MOBILE já cabia no Cube.
7. **Dragon Net e HTTP 429** (0c7ed75, 89a39bb).
   - `net_match.c` tem agora um freio por serviço: num 429 (ou "error code: 1027" do Cloudflare) aquele serviço
     não recebe nada por 30 s, dobrando até 10 min; uma resposta boa zera a espera. Um limite diário no servidor A
     passa para o B na hora (antes: depois de três falhas).
   - O jogo mostra "Servidor ocupado (limite diário), tente mais tarde." na página do Dragon Net e na
     Classificatória (uma falha de perfil por limite apagava o aviso; corrigido em 89a39bb), e o launcher mostra o
     mesmo no banner.
   - Menos pedidos: a fila da Classificatória só com a tela aberta; a busca a cada 4 s no primeiro meio minuto,
     8 s até dois minutos e 15 s depois; o resultado não confirmado é reenviado em 5 s, dobrando até 1 min.
   - Teste: `BT3_NET_TEST_429=1` no `bt3_env.txt` faz cada pedido responder como um 429 diário. No RP5: "hello",
     "presence" e "ranked/profile" pararam por 30 s cada, o A passou para o B, e o aviso apareceu na tela.
   - Pedidos por minuto (contador no log: "net: N requests to the online services in S s"):

     | Situação | antes (0.8.1, pelo código) | depois (medido no RP5) |
     |---|---|---|
     | Página do Dragon Net parada | presença a cada 5 min + fila da Classificatória a cada 4 s, também em segundo plano | 1 presença ao abrir, **nada** em 3,5 min |
     | Tela da Classificatória | fila a cada 4 s (15/min) | **3 pedidos em 176 s** |
     | Resultado esperando o rival | a cada rodada (até 30/min) | 5 s, dobrando até 1 min |

8. **DATE com stencil no Mali: não implementado, nada no `mali-lab`.** O `SetupDATE` do PCSX2
   (`GSDeviceVK.cpp`) também fecha o render pass, roda um pass de stencil que lê o alvo e reabre. No Mali isso
   quebra o pass de tiles exatamente como a nossa cópia, então não daria o ganho esperado (as 14,5 cópias por
   quadro que custam ~13 pontos em VK 2x). O caminho que daria ganho é manter o bit alto do alfa num bit de stencil
   ao desenhar (dividindo os desenhos que escrevem alfa), o que é grande e arriscado demais para esta versão. O
   branch `mali-lab` ficou como estava (regra 2).
9. **Versão:** versionName 0.9.0, versionCode 900 (8d9d5fb); status do README nas três línguas e o roadmap com
   0.8.1 e 0.9.0.
10. **Licença do APK.** Nada da ISO nem de dados do jogo:
    - `lib/`: `libbt3.so` (o motor: a decomp recompilada), `libbt4.so` (o runner do BT3-Recomp com o código do mod
      traduzido para C++ e compilado: é código gerado a partir do ELF, sem as seções de dados do ELF; `.rodata` de
      1,4 MB são tabelas do runtime), `libbt4gsx.so` (ARMSX2/PCSX2, GPL-3), SDL3 e os ganchos de driver;
    - `classes.dex`: o launcher e, desde a seção 8, o Apache Commons Compress (Apache-2.0, compatível com a AGPL-3.0;
      creditado nos três READMEs), reduzido pelo R8 ao leitor de zip;
    - `assets/engine`: fontes livres (Anton, DejaVu, Fira) e a arte própria do menu (`tbo/ui`);
    - `assets/bt4gsx`: recursos do PCSX2 (GameIndex.yaml, shaders, fontes com as licenças junto) e o cache de
      SPIR-V (shaders compilados do próprio ARMSX2, nada do jogo);
    - `assets/libbt3.so.dat/.rel/.mem`: tabelas de onde vêm os dados e de relocação, sem conteúdo do jogo.
    O ELF e o `DBZP.BIN` do BT4 vêm da ISO do jogador na importação (`runtime/data`), e o `libbt4.so` só é gerado a
    partir deles na máquina de quem compila.

## 4. BT4: estado

Atualizações da luta por segundo (Duelo, Espectador, 60 s de luta, média de blocos de 5 s; 30 é a velocidade
certa), build final:

| | Cube | RP5 |
|---|---|---|
| Renderizador padrão | ARMSX2 | ARMSX2 |
| ARMSX2, Vulkan | 28,5 (27,9 a 29,1), 2 compilações de shader na luta | 30,0, 0 compilações |
| ARMSX2, OpenGL | 28,6 | **6 a 9: bug aberto** (seção 5) |
| ARMSX2, Vulkan, driver MaliSX2 0.2.3 | 29,8 | n/a |
| Dragon Rage (experimental), Vulkan | 29,4 | 30,0 |
| Dragon Rage (experimental), OpenGL | 29,7 | 30,0 |
| Código do jogo por quadro (`guest_ms`) | 15 a 17 ms | 12 ms |
| Abrir depois de reiniciar o aparelho | OK (29,1) | n/a |
| 60 fps | não aguenta (30); o vigia recua e avisa | não aguenta (36 a 41); o vigia recua |
| Movimento suave | não oferecido (item 4) | não oferecido |
| Cenário distante no Dragon Rage | ruído | ruído |

**Cenário distante com ruído (aberto).** Para reproduzir: BT4, renderizador Dragon Rage, Duelo, Espectador, mapa da
planície, ~60 s de luta com a câmera mostrando o horizonte. As montanhas e pedras ao longe saem como blocos de
ruído colorido; o ARMSX2 desenha certo. Hipóteses da lista do brief:

- **Q ≈ 0,001 (descartada).** No GIF PACKED, o Q do registro ST só deveria valer no próximo RGBA (como no GS e no
  PCSX2: `m_q`); o nosso aplicava na hora. Corrigi e medi com o mesmo APK (`BT3_GS_ST_Q=1` volta ao antigo): no
  BT3, 0 pixels diferentes nos 8 quadros e a mesma velocidade; no BT4, o ruído aparece igual com o Q novo e com
  o antigo (6 capturas de cada no RP5). Como não resolve, a mudança **não entrou** na versão. O patch, se quiser
  guardar:

  ```diff
  -            case 0x1: reg_write(1, ... | (uint64_t)*(uint32_t *)&gs.q << 32); break;
  -            case 0x2: gs.st = lo; { uint32_t qq = (uint32_t)hi; memcpy(&gs.q, &qq, 4); } break;
  +            case 0x1: reg_write(1, ... | (uint64_t)*(uint32_t *)&sQNext << 32); break;
  +            case 0x2: gs.st = lo; { uint32_t qq = (uint32_t)hi; memcpy(&sQNext, &qq, 4); } break;
  ```
  (com `static float sQNext = 1.0f;` em `gs_core.c`)
- **Chave do cache de texturas (CBP/CSA/CPSM): improvável.** A chave já é o TEX0 inteiro (com CBP, CPSM e CSA),
  o TEXA, um hash das páginas da textura e um hash das entradas da paleta. Um detalhe que fica anotado: o
  `clut_hash` e a decodificação leem a paleta da memória a partir do CBP e ignoram o CSA (`gs_core.c`: "CSA only
  moves a 16-colour palette inside the GS's internal buffer"). Isso só erraria cores de texturas T4 com CSA ≠ 0 e
  paleta carregada antes (CLD = 0), não blocos de ruído numa T8.
- **X negativo (a testar).** Os vértices do fundo vêm com X fora de 0..4095, onde o console dá a volta; é o
  próximo suspeito, junto com o uso do mesmo tbp 2a00 por várias texturas no mesmo quadro (o dump do device lab).
- Próximo passo: desenhar só esses prims (`BT3_GS_FROM` / `BT3_GS_STOP`) e comparar a textura T8 1024x256 de tbp
  2a00 com o dump do GS do ARMSX2.

## 5. Bugs

**Corrigidos nesta versão** (além dos do device lab, já no branch: 0cabeef, 8e838c7, 23fa0ef, 2e894c0, 069c31a,
4c8794e):

- `libbt4.so` não abria depois do merge (símbolos novos do PC) e a costura com a escala errada (6e57077);
- os avisos do OSD do ARMSX2 na abertura (b94a6fa);
- controle clássico sobreposto e cortado em tela quadrada (81e16c6);
- a falha de perfil por limite apagava o "Servidor ocupado" na Classificatória (89a39bb);
- o teste `BT3_NET_TEST_429` lia o ambiente pelo cache do PC (que guarda a primeira resposta) e não ligava no
  Android; agora usa `getenv` só nele (89a39bb);
- o diário do app (e o log exportado) não dizia qual renderizador do BT4 abriu; agora a linha de início diz
  "started (renderer 1, BT4 renderer ARMSX2, online Dragon Net)" (d138e3a).

**Abertos:**

- **BT4, renderizador Dragon Rage: ruído no cenário distante** (seção 4). Contorno: o ARMSX2, que é o padrão.
- **BT4, ARMSX2 em OpenGL no Adreno 650: 6 a 9 atualizações/s** (a luta a ~25% da velocidade). Para reproduzir:
  RP5, BT4, Ajustes → OpenGL ES, renderizador ARMSX2, Duelo. Medido duas vezes (8,9 e 6,0 a 7,3). No Cube o mesmo
  caminho faz 28,6, e em Vulkan (o padrão) o RP5 faz 30. Não deu para saber se é anterior a esta versão: o APK de
  antes do merge não abre o BT4 com o launcher atual. Uma pegadinha que esconde o bug nos testes: o launcher
  reescreve `gpu_api` a partir de `dr_renderer` antes de abrir o jogo, então quem muda só `gpu_api` no
  `bt3_settings.txt` continua em Vulkan (e o `[vstatus]` do ARMSX2 diz "configured OpenGL" mesmo assim). O "ARMSX2,
  GL: OK" do device lab no RP5 provavelmente era esse caso. Sugestão até corrigir: esconder o OpenGL para o ARMSX2
  no Adreno, ou avisar na opção.
- **Servidor A sem presença.** `POST /v1/presence` em `mitigo-rooms` responde 404 `{"error":"no room"}`; no
  servidor B (DBT-NX) responde 200. Na página do Dragon Net, o "Online" aparece como indisponível enquanto o jogo
  usa o A. É do lado do servidor: falta publicar a rota de presença (`port/dragonnet/presence.js`) no Worker do A.
- **Servidor C (Classificatória).** `bt-ranked` respondeu `{"error":"identity"}` (401) ao perfil e
  `{"error":"ranked unavailable"}` (503) à tabela durante os testes. Por isso a busca da Classificatória não pôde
  ser medida até o fim; a tela e o freio funcionam (3 pedidos em 176 s). Também é do lado do servidor.
- **Cube: launcher marca o online como "Unreachable"** desde 23/10, antes desta versão. O Cube está com
  `restricted_networking_mode=1` (a "rede restrita" do Android 13): o `dumpsys netpolicy` mostra o app em
  `blocked=RESTRICTED_MODE`, o diário do app registra `UnknownHostException`, e o shell do mesmo aparelho chega
  ao servidor (HTTP 200). No RP5 o mesmo APK fala com os dois serviços. É configuração do aparelho, não do app; não
  mexi. Para desligar: `adb shell settings put global restricted_networking_mode 0`.
- START sozinho uns 50 ms antes do SELECT abre a pausa do jogo por baixo do menu do BT4 (já acontecia antes).

## 6. Build final e regressão

**Build limpa:** `python3 port/tools/android.py && python3 port/bt4/build_bt4.py --armsx2 && cd android &&
./gradlew clean assembleRelease`, sem erros, e a checagem de símbolos do `libbt4.so` passou. APK 0.9.0 (900),
58,5 MB. Depois dela só mudou uma linha de log do launcher (d138e3a), conferida no Cube com o APK refeito.

**Instalação por cima:** nos dois aparelhos, por cima da build anterior (900, mesma chave). Saves do BT3 e do BT4
(md5), ajustes, as duas ISOs importadas (`gamedata/disc`, `bt4/runtime/data`, `bt4/iso_uri.txt`) e os drivers
continuaram. A subida 801 → 900 já tinha sido feita no começo da versão (post1 por cima do lab21), com tudo mantido.

| Item | Cube | RP5 |
|---|---|---|
| BT3, luta gravada, VK/GL e resoluções | seção 2 | seção 2 |
| BT3, imagem (pixels) | 0 diferentes | 0 diferentes contra a pós-merge |
| BT4, os dois renderizadores, VK e GL | seção 4 | seção 4 (GL do ARMSX2: bug) |
| Drivers | MaliSX2 0.2.3 importado do Downloads: BT3 em VK cai para GL sem tela preta (97,7% em 2x), BT4 com ARMSX2 em VK a 29,8; volta ao do sistema e remoção registradas no diário | Turnip axfl2-006 (já instalado) escolhido pela linha: BT3 VK 2x a 100% (trabalho 13,6 ms contra 16,3); volta ao do sistema |
| Abrir depois de reiniciar | BT4 abre e luta (29,1) | n/a |
| Toque | CLÁSSICO e MOBILE dentro da tela em 720x720 (81e16c6) | MOBILE e CLÁSSICO em 16:9 sem sobreposição |
| Menu do jogo e START+SELECT | BT4: abre e fecha com START+SELECT, a luta volta (29,4); Desempenho mostra "60 fps nas lutas (experimental)" e o aviso de recuo | n/a (sem root para injetar o combo) |
| Log exportado | "Salvar log em Download": diário do app (drivers, online, início do jogo), pasta do jogo, três logs do jogo, logcat | n/a |
| Online | banner "Não foi possível falar..." (rede restrita do aparelho, seção 5) | banner "Dragon Net está funcionando"; modo PC `Working(mine=138)`; 429 testado |
| Idiomas | as três línguas com as mesmas chaves (só `app_name` sem tradução) | idem |

Não refeitos nesta rodada, por não terem mudado desde o device lab (que os testou nos dois aparelhos): importar e
reimportar as ISOs (`GameDataInstaller`, `Bt4Installer`), temas e o atalho na tela inicial.

Incidente durante os testes: no Cube, logo depois de um `adb install`, o app não veio para a frente, e os botões
do roteiro abriram o DuckStation pelo frontend do aparelho. Fechei com `force-stop`; ele só escreveu caches e um
ícone (nenhum save ou cartão de memória). O roteiro agora só aperta botões com o Dragon Rage na frente.

**Aparelhos restaurados:** `bt3_settings.txt` e `bt4/bt3_settings.txt` como antes da versão, `bt4/iso_uri.txt`
igual, saves iguais (md5), Downloads igual, sem `fight.rec`, sem `bt3_env.txt` novo (o Cube tinha um, com
`BT3_GS_VERBOSE=1`, e continua com ele), `touch_controls.json` do RP5 de volta e nenhum no Cube, driver do
sistema nos dois (o Cube sem drivers instalados; o RP5 com o Turnip instalado, como já estava, sem uso). O RP5
ficou com 20% de bateria, no USB.

## 7. Commits locais e como publicar

```
(seção 8)        Docs: fullscreen and texture packs in the 0.9.0 report
5dab121 Textures: help says what to do with a .rar or .7z pack and the room it needs
d6467b2 Texture packs: used on Android, in both games; more zips read
79ce685 Game: hide the system bars before the surface is made
6dabc20 Docs: release 0.9.0 report
d138e3a Launcher: log the BT4 renderer on start
8d9d5fb Release 0.9.0: version, README, changelog
89a39bb Dragon Net: busy notice on Ranked, 429 test env
81e16c6 Touch: classic pad fits square screens
0c7ed75 Dragon Net: back off on HTTP 429
1bc64e2 BT4: ARMSX2 by default, 60 fps fallback
b94a6fa BT4: ARMSX2 shader cache, link check
6e57077 BT4: shim symbols, seam width scale
01224d0 Merge PC 0.1.38.7
eefe5d9 BT4: ARMSX2's FPS meter counts the game's pictures
(e os do device-lab e armsx2 por baixo)
```

Para publicar (nada disso foi feito):

```
git switch release-0.9.0
git push -u origin release-0.9.0
# PR de release-0.9.0 para main (ou merge local):
git switch main
git merge --no-ff release-0.9.0 -m "Dragon Rage 0.9.0"
git push origin main
git tag android-v0.9.0
git push origin android-v0.9.0     # o CI (android.yml) gera o release "Dragon Rage 0.9.0" com o APK
```

Atenção ao versionCode: no CI ele vem do número da execução (`DR_VERSION_CODE`), e localmente é 900. Os dois
aparelhos de teste estão com o 900 local, assinado com a chave de teste (`dragonrage-test.jks`); um APK do CI com
código menor ou outra assinatura não instala por cima sem desinstalar (o RP5 recusa downgrade). Os jogadores, que
vêm do APK do CI, não têm esse problema.

## 8. Depois do relatório: tela cheia no RP5 e pacotes de textura

Os dois itens pedidos depois do relatório. Os dois valem também para a build estável (ver o fim da seção).

### BT4 (e BT3) sem ocupar a tela inteira no RP5 (79ce685)

**Causa.** Não era a proporção do jogo nem um recorte: a superfície do jogo era criada enquanto a barra de navegação
ainda estava na tela (1920x1080 menos os 108 px da barra = **1920x972**). O SDL só esconde as barras quando o motor
cria a janela, e aí a superfície já existia. Em parte dos inícios ela ficava em 972 a partida inteira e o sistema
esticava a imagem para 1080: no `dumpsys SurfaceFlinger`, a camada do jogo com `size=(1920, 972)` e escala
`tr=[1.00 … 1.11]`, ou seja, tudo 11% mais alto e, em 16:9, faixas pretas dos lados. Não depende do renderizador:
acontecia no BT3 e no BT4 (o ARMSX2 registrava "window 1920x972"). Nos logs do lab, a partir de 24/10 metade das
rodadas do RP5 tinha `swapchain yes 1920x972`.

**Tentativa descartada.** Refazer o swapchain quando ele difere da janela (`SDL_SetGPUSwapchainParameters`) não
adianta: a própria superfície Vulkan continua informando 972 (`currentExtent`). Revertido.

**Correção.** O `GameActivity` esconde as barras no `onCreate`, antes de a superfície existir, com as mesmas flags
do modo tela cheia do SDL (`setWindowStyle(true)` também, para o SDL continuar escondendo-as depois de um gesto).
A linha `present:` do log agora traz o tamanho da janela ao lado do swapchain.

| | antes | depois |
|---|---|---|
| RP5, BT3, inícios seguidos | 3 de 8 em 1920x972 | 10 de 10 em 1920x1080 desde o 1º quadro |
| RP5, BT4 ARMSX2 16:9 / 4:3 / tela cheia | janela 1920x972 | 1920x1080; 16:9 e tela cheia ocupam a tela, 4:3 com faixas (certo) |
| RP5, BT4 Dragon Rage 16:9 | — | 1920x1080, ocupa a tela |
| Cube (720x720), BT3, 4 inícios | 720x720 | 720x720 |
| BT3 luta gravada, Cube VK 1x / VK 2x / GL 2x | 100 / 94,8 / 97,8% | 100 / 94,3 / 97,7% |
| BT3 luta gravada, RP5 VK 2x / VK 4x / GL 2x | 100 / 83,7 / 100% | 100 / 83,7 / 100% |
| Imagem (8 quadros no Cube, 7 no RP5) | — | 0 pixels diferentes contra a final |

### Importar texturas não funcionava (d6467b2, 5dab121)

Havia quatro falhas independentes; a primeira explica sozinha o "instala e nada muda" em qualquer aparelho.

1. **BT3: o pacote importado nunca era usado no Android.** O launcher instala em `files/textures`, e o motor
   procurava `"textures"` com o SDL, que no Android resolve caminho relativo dentro da pasta **interna** do app
   (`/data/user/0/<app>/files`), não na pasta do jogo. O log do jogo dizia `texture pack: no folder textures` com o
   pacote no lugar. O motor foi testado com pacotes só no PC, onde o caminho relativo funciona. Correção: no Android
   o caminho fica absoluto a partir da pasta do jogo (`gs_texpack.c`).
2. **BT4: nenhum dos dois renderizadores olhava `bt4/textures`.** O do Dragon Rage tinha a mesma falha do item 1
   (e o jogo ainda troca de pasta); o ARMSX2 procurava em `bt4/gsx/ARMSX2/textures/SLUS-21978/replacements`, com
   `LoadTextureReplacements = false`. Agora o host passa o caminho completo de `bt4/textures` aos dois (`BT3_TEXTURES`
   e `BT4GSX_TEXTURES`, este com um trecho novo no `armsx2-bt4.patch`), e o ARMSX2 liga as substituições só quando
   há um pacote instalado (sem pacote, o ini é o mesmo de antes).
3. **Zips que o leitor do Java recusa.** Testado no PC com o mesmo leitor do app (`ZipInputStream`) e uma fatia do
   pacote real: 7-Zip com Deflate64 ou BZip2 dava "invalid compression method", e zip gravado em fluxo com entradas
   sem compressão dava "only DEFLATED entries can have EXT descriptor"; Deflate comum, Store, Info-ZIP, Zip64, nomes
   do Windows e com acento passavam. O app agora lê com o Apache Commons Compress: as 11 variantes passam no PC, e no
   RP5 a de Deflate64 e a gravada em fluxo instalaram pela interface (598 texturas cada).
4. **Espaço.** O pacote que circula (o mesmo das 18.704 texturas do log do Odin 3 de 11/10) vem em **RAR 5** de
   2,1 GB e ocupa **10,3 GB** extraído. Com a build anterior, no RP5 com 7,7 GB livres: 68 s extraindo e a janela
   "write failed: ENOSPC (No space left on device)". Agora o app lê o índice do zip antes (1,3 s para esse pacote)
   e recusa na hora: "Espaço insuficiente: este pack ocupa 10,3 GB depois de extraído e o aparelho tem 7,8 GB
   livres". Se o espaço acabar no meio (zip sem índice legível), a mensagem diz isso. O que uma importação cortada
   deixava em `.import` é apagado na próxima.

Também: o seletor de arquivos mostra qualquer arquivo (antes o `.rar`/`.7z` aparecia apagado e o jogador não recebia
explicação nenhuma); um `.7z` escolhido recebe "Escolha um pack em .zip. Um .rar ou .7z precisa ser extraído e
compactado de novo...". A ajuda da tela de texturas diz isso e o espaço necessário. RAR 5 não tem leitor em Java
(o junrar só lê RAR 4); ler `.7z` direto seria possível com o Commons Compress e o XZ, mas não entrou.

Sobre o log do Odin 3 (0.4.0, 11/10): "18704 files, 0 replacement textures", sem nenhuma "other image". O zip abriu,
mas nenhum nome, do jeito que aquela build o via, terminava em `.dds`/`.png`; a 0.6.0 já tinha trocado esse ponto
(nome direto da entrada do zip, com `trim`, e os 3 primeiros nomes no log). Com o mesmo pacote, a build atual
reconhece as 18.704. Mesmo que aquela importação tivesse passado, o jogo não teria usado o pacote (item 1).

**Medido** (pacote inteiro, zip de 1,9 GB feito no 7-Zip, importado pela interface):

| | Cube (Mali-G57) | RP5 (Adreno 650) |
|---|---|---|
| Importar, build anterior | 146 s, 18.704 texturas | ENOSPC depois de 68 s (7,7 GB livres) |
| Importar, build nova | 154 s (BT4) | 136 s (41,5 GB livres, depois que o espaço foi liberado) |
| BT3, luta gravada com o pacote | 18.597 indexadas, ~3.000 trocadas, 100% (VK 1x, trabalho 22,6 ms contra 22,0 sem) | 18.597 indexadas, ~2.800 trocadas, 100% (VK 2x, 16,3 ms contra 16,0) |
| BT4 com o pacote do BT3 | ARMSX2: 18.597 indexadas, 28,7 atualizações/s; Dragon Rage: 34 trocadas, 29,3 (29,6 sem) | não medido |

O Cube não tem texturas DXT na GPU: os `.dds` são descompactados na CPU, e mesmo assim a luta fica em 30. No BT4 o
ARMSX2 reservou um cache de até 5,3 GB para texturas descompactadas (metade da RAM): com um pacote feito para o BT4,
que troque muito mais que as 34 do pacote do BT3, vale medir a memória numa luta longa.

**Ainda aberto:** um pacote em `.rar`/`.7z` continua precisando ser convertido pelo jogador; e o pacote tem pastas
alternativas (`Original Buttons`, `Xbox Layout`) ao lado de `replacements` com os mesmos nomes; vale o primeiro
encontrado, como já estava documentado no `docs/port/README.md`.

**Para a build estável:** as duas correções valem para qualquer build Android (não dependem de nada desta versão): o
`onCreate` do `GameActivity`, o `gs_texpack.c` e o leitor de zip do launcher. A parte do BT4 só existe nesta.

**Aparelhos:** os testes desta seção instalaram o pacote nos dois e mudaram ajustes; tudo voltou ao estado da seção
6 (ajustes do BT3 e do BT4, `bt3_env.txt` do Cube, sem `fight.rec`, sem pacotes em `textures`, Downloads igual,
saves iguais). Os dois estão com o APK desta seção (900). O zip do pacote ficou no PC em `~/dbz-tex/zips/SLUS-21678.zip`.
