# Device lab, outubro de 2026: RG Cube e Retroid Pocket 5

Branch `device-lab` (criado de `armsx2`), commits só locais, sem push. Datas: 24 e 25/10/2026.

| Aparelho | SoC / GPU | Driver | Android |
|---|---|---|---|
| RG Cube | Unisoc T820, Mali-G57 MC4 | do sistema (MaliSX2 não testado, ver "Pendente") | 13, root |
| Retroid Pocket 5 (RP5) | Snapdragon 865, Adreno 650 | Qualcomm 512.746.0 (e Turnip axfl2-006 no teste de drivers) | 13 |

Builds medidas: `armsx2` (950764a) como base e os commits deste branch até 2e894c0. O APK final (libbt3, libbt4
e o launcher de 2e894c0) está instalado no RP5.

## Como foi medido

- BT3: uma luta COM x COM gravada (entrada do controle repetida com `BT3_PAD_PLAY`), sempre a mesma, com 215 s
  por execução. A média vem dos "heartbeats" da luta, entre 900 e 4500 blanks depois do início. Configurações
  em `bt3_settings.txt` e chaves em `bt3_env.txt`, com `BT3_GS_VERBOSE=1`.
- Colunas: **vel.** é a velocidade do jogo (100% = 60 blanks/s); **quadros/s** são as imagens por segundo;
  **trabalho** é o tempo médio de CPU+GPU por quadro (o limite para 30 é 33,3 ms); **pior** é o pior quadro;
  **present** é o tempo esperando a GPU no fim do quadro; **passes** e **cópias** são as médias por quadro.
- BT4: script que abre o BT4 pelo launcher, entra em Duelo, Espectador, e mede 60 a 75 s de luta. O log traz
  `logicrate` (atualizações da luta por segundo), stalls e readback.

## Antes e depois (BT3, luta gravada)

### RG Cube, Mali-G57 MC4

| Config | vel. | quadros/s | trabalho (ms) | present (ms) | passes | cópias |
|---|---|---|---|---|---|---|
| VK 1x, base | 100% | 30,0 | 18,2 | 5,9 | 55,3 | 10,5 |
| VK 2x, base (`armsx2`) | 75,8% | 22,7 | 43,4 | 25,3 | 58,5 | 12,9 |
| VK 2x, com profundidade (`BT3_GPU_DEPTHLESS=0`) | 73,3% | 22,0 | 43,3 | 26,9 | 60,7 | 14,5 |
| **VK 2x, passes sem profundidade (c2b26ba)** | **86,8%** | **26,0** | **37,2** | **20,9** | 60,7 | 14,5 |
| VK 2x, sem cópias DATE (só teste, imagem errada) | 99,0% | 29,7 | 26,9 | 11,1 | 48,9 | 1,0 |
| VK 3x, base | 39,0% | 11,8 | 78,4 | 60,9 | 61,7 | 15,0 |
| **VK 3x, sem profundidade** | **48,2%** | 14,4 | 64,4 | 47,7 | 61,6 | 15,1 |
| GL 2x, base | 92,5% | 27,8 | 34,6 | n/a | 53,2 | 14,5 |
| GL 2x, sem profundidade | 92,5% | 27,7 | 34,9 | n/a | 53,2 | 14,5 |
| GL 3x | 43,5% | 13,1 | 71,0 | n/a | 54,0 | 15,1 |

Perfil (`fx_off=48 frame_skip=1 frag_depth=0`):

| Config | vel. | quadros/s | trabalho | pior |
|---|---|---|---|---|
| VK 1x | 100% | 30,0 | 20,8 | 22,6 |
| **VK 1.5x** | **100%** | 30,0 | 23,5 | **25,3** |
| GL 1.5x | 100% | 30,0 | 22,6 | 24,1 |
| GL 2x | 100% | 30,0 | 26,9 | 30,4 (apertado) |
| VK 1x com movimento suave 60 | 100% | 31,2 | 21,3 | n/a |

A comparação de pixels no Cube (1x, mesmos quadros da luta) deu **0 pixels diferentes** em dois casos: código
antigo (sombra e DATE quebrando o pass, com profundidade) contra o novo, e GL com e sem profundidade.

### Retroid Pocket 5, Adreno 650

| Config | vel. | quadros/s | trabalho (ms) | present (ms) | passes | cópias |
|---|---|---|---|---|---|---|
| VK 4x, base (`armsx2`) | 70,8% | 21,2 | 46,0 | 28,9 | 58,6 | 13,4 |
| VK 4x, sem stencil de sombra | 76,0% | 22,8 | 42,6 | 24,8 | 58,6 | 13,4 |
| **VK 4x, sem stencil e sem profundidade (c2b26ba)** | **79,7%** | **24,0** | **40,4** | 23,2 | 58,6 | 13,4 |
| VK 4x, Turnip axfl2-006 | 70,8 a 71,5% | 21,2 | 46,0 | 33,2 | 58,6 | 13,4 |
| VK 3x | 100% | 30,0 | 15,2 | 1,1 | 58,6 | 13,4 |
| VK 2x, base | 100% | 30,0 | 13,6 | 0,2 | 58,6 | 13,4 |
| GL 2x, base | ~4 s por quadro (luta nem chega a medir) | n/a | n/a | n/a | n/a | n/a |
| **GL 2x, buffers persistentes (c2b26ba)** | **100%** | 30,0 | 14,0 | n/a | 51,3 | 13,4 |
| GL 4x com profundidade | 54,6% | 16,4 | 58,7 | n/a | 51,6 | 13,6 |
| **GL 4x sem profundidade** | **60,3%** | 18,1 | 53,2 | n/a | 51,7 | 13,7 |

Perfil do Adreno 650 (tier 3: 2x, profundidade precisa, pular quadros):

| Config | vel. | quadros/s | trabalho | pior |
|---|---|---|---|---|
| 2x, 30 | 100% | 30,0 | 13,4 | 15,5 |
| **2x, movimento suave 60 (perfil novo, aplicado pelo diálogo)** | **100%** | **55,8 a 63** | 27,5 | 30,0 |
| 3x, movimento suave 60 | 100% | 34,3 | 19,1 | n/a |

Modos de apresentação (VK 4x): vsync 80,0%; mailbox 79,8%. O immediate não é oferecido pelo driver e cai em
automático (79,8%). Nenhum ganho.

Sem efeito (dentro do ruído) no RP5 VK 4x: submit único (75,8%), discard no shader (74,7%, levemente pior),
1 ou 3 quadros em voo (76,0 / 75,8%) e ADPF desligado (75,8%). No Cube VK 2x: submit único, 1 ou 3 quadros em
voo, stencil e quebra no DATE, tudo entre 73,0 e 73,7%. No Cube, GL persist=0 deu 93,8% contra 92,5%, que é ruído.

## Gargalo do Mali

1. **Load/store de profundidade em cada render pass** (VK). São cerca de 60 passes por quadro, e o Mali
   carregava e gravava a profundidade em todos, até nos que não usam depth. Os passes "depthless" (bit 26 do
   pipeline; VK e GL) resolveram: +13 pontos em 2x (73,3% → 86,8%) e +9 em 3x, com imagem idêntica. No GL o
   driver do Mali já fazia isso sozinho: 92,5% com e sem.
2. **O que sobra são as cópias DATE** (teste de alfa de destino: barras do HUD e camadas do cenário). São
   cerca de 14,5 por quadro, e cada uma fecha o pass de tiles e abre outro. Tirando as cópias (só teste, imagem
   errada) o VK 2x vai a 99%, com present de 20,9 para 11,1 ms. Caminhos para o `mali-lab`, atrás de checagem
   de GPU:
   - DATE com stencil, como o PCSX2: um quad marca no stencil os pixels pelo bit de alfa e o desenho testa o
     stencil, sem cópia e sem sair do pass;
   - framebuffer fetch (`EXT/ARM_shader_framebuffer_fetch`). Já existe no GL (`gl_fetch=1` / `BT3_GL_FETCH=1`,
     desligado por padrão porque travou dois Mali-G52). **Não foi medido no G57**: o Cube caiu antes.
3. O máximo medido no Cube para 30 estáveis: **VK 1.5x** (pior quadro 25 ms) ou GL 2x (100%, pior 30 ms, sem
   folga). VK 2x chega a 86,8%. Movimento suave 60 não cabe nem em 1x (31 quadros/s).

## Regressão no Adreno: causa e correção

- **Vulkan:** a causa principal é o **stencil da sombra** (D32S8): 70,8% → 76,0% desligando só ele. A primeira
  versão sem quebra de pass (stencil por desenho) chegou a derrubar o RP5 para 44,8%; com refs de stencil por
  sequência de sombras e `shadow_stencil` desligado automaticamente no Adreno, voltou e passou do original. Os
  passes sem profundidade somam mais 3,7 pontos (79,7%). Submit em duas partes, shader sem discard, quadros em
  voo e ADPF **não** são a causa (tabela acima). O bit 9 de blend do PC 0.1.37 não foi isolado: com as
  correções acima o VK 4x ficou acima da base.
- **OpenGL:** sem buffers persistentes, o Adreno levava ~4 s por quadro em 2x. Com persist ligado
  automaticamente no Adreno (c2b26ba), 2x vai a 100% e 4x a 60,3%.
- Corrigido em **c2b26ba** (`GS: depthless passes, shadow refs, Adreno fixes`). Também tem
  `BT3_GPU_NO_STENCIL=1` para A/B.

## BT4

| Item | RP5 | Cube |
|---|---|---|
| Importar ISO (Downloads, "Selecionar ISO") | OK: 23 arquivos, 5,5 MB copiados, imagem lida no lugar, SHA-256 do ELF 224525C5…E8A4 no log, DBZP.BIN copiado | OK, mesmos números; permissão persistida antes da cópia (dumpsys `persisted=0x1`) |
| Remover e reimportar | OK (saves mantidos) | OK |
| Abrir depois de reiniciar | n/a | **pendente**: o Cube saiu do USB no reboot |
| Renderer ARMSX2, VK e GL | OK: abre, desenha certo, luta a 30; readback de 344064 bytes em 2 a 14 ms | pendente |
| Renderer Dragon Rage, VK e GL | 30 quadros/s estáveis por 150 s+ (VK: 36 passes, 11 cópias, draws 4 ms) | pendente |
| 60 fps nas lutas (`bt4_fps60`) | liga (halfstep ativo), mas só faz 37 a 41 atualizações/s: o código do jogo leva ~15 a 16 ms por quadro (`guest_ms`), então a luta roda a ~65% e fica mais **lenta**. Desliga pelo menu (Desempenho → "60 fps nas lutas") ao vivo e salva | pendente |
| Tecla voltar abre o menu de toque e pausa | OK (BT3 e BT4; páginas Vídeo, Desempenho, Efeitos, Áudio, Interface, Controles e Online abrem) | pendente |
| START+SELECT | não dá para injetar no RP5 (teclado não entra no `PortInput_ComboPoll`, gamepad precisa de root) | pendente (pad.py) |

Bugs do BT4 corrigidos:

- **4c8794e**:
  - ARMSX2: os botões não chegavam ao jogo (`sendPad` exigia o renderer Dragon Rage).
  - ARMSX2: o jogo travava no início da luta. O readback de PSMCT24 escrevia 4 bytes/px (458752) num buffer
    de 0x60000, passava do fim e estragava o cabeçalho do bloco seguinte do heap, e o jogo ficava num loop
    infinito em 0x255018. Agora usa 3 bytes/px (344064), no patch do BT3-Recomp.
  - O host agora responde ao readback (`consumerFlags` bit 1).
  - `bt3_env.txt` agora é lido pelo `bt4_main`.
  - O ARMSX2 não mostra mais o OSD de dump no modo ao vivo.
- **069c31a**: no renderer Dragon Rage, depois de ~2 min a tela ficava branca, com quadrados rosa, a 6
  quadros/s. O `gGsFrame` nunca andava no BT4, então o cache de texturas não descartava nada, enchia (3072) e
  devolvia branco.

**Aberto: cenário distante corrompido no renderer Dragon Rage** (BT4, VK e GL, RP5). As pedras e montanhas ao
longe aparecem como blocos de ruído claro e colorido; perto, o cenário sai certo; o ARMSX2 desenha tudo certo.

- Para reproduzir: BT4, renderer Dragon Rage, Duelo, Espectador, mapa da planície (o primeiro), uns 60 s de
  luta com a câmera mostrando o horizonte.
- Hipóteses descartadas, todas com dump de quadro (`BT3_GS_DUMP`):
  - não há cópias local→local (TRXDIR 2);
  - não há mipmap (TEX1 = 0x60, MXL 0);
  - o VU1 não é usado no BT4 (`BT3_VU_INTERP=1` não muda nada);
  - não há region repeat (CLAMP só nos modos 0/1/2);
  - STQ em fp32.
- O que o dump mostra:
  - o BT4 manda muitas texturas pelo mesmo endereço (tbp 2a00, em PSMCT32/24/16, T8 e T4) dentro do mesmo quadro;
  - o fundo distante usa uma textura T8 de 1024x256 nesse endereço, com Q ≈ 0,001 e vértices em X negativo
    (fora da faixa 0..4095 do GS, onde o console dá a volta).
- Próximo passo sugerido: desenhar só esses prims (`BT3_GS_FROM` / `BT3_GS_STOP`) e comparar a textura
  decodificada com a do ARMSX2 (dump do GS).

De passagem, achei e corrigi um bug real no rastreio de páginas (**2e894c0**): um upload PSMCT24 (e 8H/4HL/4HH)
marcava páginas contando 24 (8, 4) bits por pixel, mas na memória ocupa 32. As últimas páginas do retângulo não
eram marcadas e uma textura ali podia ficar velha. Não resolveu o bug acima. BT3 medido depois: 100%, 55,9
quadros/s, igual a antes.

## App inteiro (RP5; Cube onde dito)

| Teste | Resultado |
|---|---|
| Importar ISO do BT3 (remover e reimportar) | OK: 13 arquivos, 2,8 GB em 7 s, "versão USA verificada", saves mantidos |
| Trocar de jogo (BT3 ↔ BT4, botão "Jogo" e escolha na abertura) | OK |
| Temas Escuro / Claro / Sistema | OK, aplica na hora, sem problema visual |
| Ajustes do launcher (resolução, filtro, proporção, idioma, Material You) | abrem e salvam |
| **Trilho lateral em 1080p paisagem** | **bug corrigido (f355797)**: com as 9 páginas do BT3, "Ajustes" e "Jogo" ficavam fora da tela e não dava para chegar neles; agora o trilho rola |
| Drivers: baixar Turnip (RP5), instalar, rodar, voltar ao do sistema | OK: "Turnip Adreno (TM) 650, Mesa 27.0.0-devel"; volta para Qualcomm 512.746.0 confirmada |
| **Drivers: tocar no nome do driver** | **bug corrigido (dc9f17f)**: só o botão de rádio escolhia o driver; agora a linha inteira escolhe |
| Drivers: MaliSX2 no Cube | pendente |
| Pacotes de textura / mods | **não testado**: não há pacote nem mod no PC nem nos aparelhos |
| Atalho na tela inicial | OK: "Criar atalho do jogo" fixa o atalho e ele abre o BT3 direto (ficou na tela inicial do RP5) |
| Exportar log | OK: `Download/DragonRage-log-<data>.txt` (1 MB) e abre o compartilhar |
| Depuração | mostra a linha de resolução ("Adreno (TM) 650 · vulkan · 4x (2048 x 1792)") e o resumo da última partida |
| Medidor de FPS | OK ("60 fps speed 100%" no canto) |
| Diálogo de perfil de GPU | OK com o perfil novo do Adreno 650 (2x, suave 60, profundidade precisa); aplicado e medido: 100%, 55,8 quadros/s |
| **Online (Dragon Net)** | **"Não foi possível falar com o serviço online"**: `mitigo-rooms.minetivacraft.workers.dev/v1/queues` responde **HTTP 429, "error code: 1027"** (limite de requisições do plano do Cloudflare Workers). Os outros dois serviços respondem 200. Não é bug do app: é o limite da conta do Worker |

### Controles de toque (RP5)

- Escondidos ao abrir quando há um controle com analógico, e aparecem no toque. Corrigido em a38b7c1.
- Visual Vidro, símbolos PlayStation e opacidade 50% aplicam.
- No editor do jogo dá para arrastar (L1 movido), "Pronto" salva `touch_controls.json` e "Resetar" volta ao
  padrão.
- Na página Controles do launcher todas as opções aparecem: estilo, menu/luta, visual, símbolos, opacidade,
  tamanho, vibração, analógico flutuante, clashes, macros.
- Dúvida: os botões "…" e de pausa ficam embaixo do overlay de desempenho do sistema do RP5, e um toque em "…"
  não fez nada. Pode ser o overlay; repetir com ele desligado.
- Cube: pendente.

## Perfis de GPU (84b31f2)

- **Mali-G57 MC4**: continua tier 0 ("Entrada"), mas com **1.5x** em vez de 1x, por uma tabela de modelos
  medidos (`measured` em `GpuProfiles.kt`). O G57 MC1/MC2 continua em 1x. Medido: VK 1.5x 100%, pior 25 ms.
- **Tier 3** (Adreno 650 a 669, 680 a 699, 730 a 739): agora com **movimento suave 60**. O 650 é o mais fraco
  da faixa e faz 100% com 56 a 63 quadros/s em 2x.
- Quem já aceitou um tier não recebe a oferta de novo (comportamento existente).

## Commits (`device-lab`, locais, sem push)

| Commit | O quê |
|---|---|
| a38b7c1 | Controles de toque escondidos ao abrir com controle de analógico |
| c2b26ba | GS: passes sem profundidade (VK+GL), DATE sem quebra, refs de stencil por sequência de sombras, stencil de sombra desligado no Adreno, GL persist no Adreno, contadores de passes por causa |
| 4c8794e | BT4: readback ARMSX2 (CT24 a 3 bytes/px), pad, env, OSD |
| 069c31a | BT4: conta quadros para o cache de texturas |
| dc9f17f | Drivers: a linha inteira escolhe o driver |
| f355797 | Launcher: o trilho lateral rola |
| 84b31f2 | Perfis de GPU medidos (G57 MC4, Adreno 650) |
| 2e894c0 | GS: marca todas as páginas de um upload de 24 bits |

## O que vai para o `bt4` estável e o que fica no lab

Testei aplicar cada commit no `bt4` numa worktree temporária (sem tocar no branch).

**Para o `bt4`**, depois dos testes do desenvolvedor:

| Commit | Aplica no `bt4`? | Por quê |
|---|---|---|
| 069c31a | limpo | corrige tela branca e 6 quadros/s no renderer Dragon Rage do BT4 |
| dc9f17f, f355797 | limpo | correções de UI sem risco |
| a38b7c1 | limpo | |
| 84b31f2 | limpo | só muda a oferta do diálogo |
| 2e894c0 | limpo | correção de memória, desempenho igual |
| c2b26ba | **conflito em `gs_gl.c`** | é o maior ganho no Mali (+13 pontos) e no Adreno, com imagem idêntica. O conflito vem do GL persist, que é do `armsx2` (9ecce97). Aplicar no `bt4` exige levar junto ou separar a parte de GL |

**Fica no `device-lab` / `armsx2`:** 4c8794e, que é do renderer ARMSX2 (não existe no `bt4`; dá conflito).

**Vai para o `mali-lab`:**
- DATE com stencil ou framebuffer fetch no Mali;
- medir `gl_fetch` no G57;
- `BT3_GPU_NO_DATE_COPY` continua só como teste.

## Pendente (Cube fora do ar)

O Cube caiu do USB no teste de reboot. Pelo Wi-Fi ele aparece em 192.168.1.178 (portas 36741/43337/49569),
mas como **offline**: falta parear. O código 859879 não funcionou em nenhuma das portas encontradas, porque o
pareamento usa uma porta própria, mostrada na tela "Parear com código". Ficou faltando no Cube:

- abrir o BT4 depois do reboot;
- BT4 nos dois renderers, VK e GL;
- `bt4_fps60`;
- START+SELECT;
- driver MaliSX2 (instalar, rodar, voltar);
- controles de toque;
- `gl_fetch`.

## Estado dos aparelhos

- **RP5:**
  - restaurados `bt3_settings.txt` e `touch_controls.json` do backup;
  - `bt4_fps60=0`;
  - removidos `bt3_env.txt` (raiz e `bt4/`) e as gravações de luta;
  - driver do sistema;
  - o atalho do BT3 ficou na tela inicial;
  - o APK do `device-lab` (2e894c0) continua instalado.
- **Cube:** não foi restaurado (está offline). Ao reconectar:
  - voltar a configuração original: `scale=2 scale4=6 fx_off=48 dr_renderer=1 frame_skip=1 filter=0
    aspect_milli=1333`;
  - apagar `bt3_env.txt` (raiz e `bt4/`).
