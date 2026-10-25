# Dragon Rage 0.9.0

**A versão 0.1.38.7 do port de PC, o Budokai Tenkaichi 4 desenhado pelo renderizador do ARMSX2 e lutas mais leves em
GPUs Mali.** Tudo medido num RG Cube (Mali-G57) e num Retroid Pocket 5 (Adreno 650), com uma luta gravada e
comparação de imagem pixel a pixel.

## Mais rápido
- **Resolução interna 2x ou mais**: as passadas de desenho que não usam profundidade rodam sem ela, as sombras planas
  dividem uma limpeza só, e o brilho dos personagens é desenhado na resolução do PlayStation 2 (vindo do PC). No RG
  Cube, a luta em Vulkan 2x foi de cerca de 76% para **95%** da velocidade (OpenGL 2x: 92% para 98%); no Retroid
  Pocket 5, Vulkan 4x foi de 71% para 84%.
- **OpenGL em Adreno**: buffers persistentes também no Adreno. No Retroid Pocket 5, OpenGL 2x levava uns 4 segundos
  por quadro e agora roda a 30 estáveis.
- **Adreno**: o stencil das sombras planas fica desligado por padrão, mais rápido nesses chips.
- **Perfis de GPU** medidos para Mali-G57 MC4 e Adreno 650: o launcher sugere a resolução que mantém a luta em 30.
- **Do PC**: menos travadas na primeira vez que um especial ou uma transformação aparece (as texturas novas são
  preparadas bem mais rápido, em vários núcleos), buffers de desenho preparados antes do primeiro grande efeito da
  luta, e o contorno dos personagens desenhado só onde há linha.
- **Pacotes de textura**: os arquivos são lidos em segundo plano e ficam na memória, e um quadro recebe no máximo
  4 MB deles. Menos travadas com pacotes grandes (vindo do PC).

## Imagem (vindo do PC)
- **Brilho dos Super Saiyajins** certo em qualquer resolução interna: antes saía como uma cópia nítida e deslocada do
  personagem.
- **Desfoque da distância** como no console em resolução alta (as folhas não aparecem mais como quadrados).
- **Contornos** com a mesma espessura em qualquer proporção de tela.
- **Costas da camisa da Videl** sem a linha tracejada.
- **Tela branca** com a terceira roupa do Gohan adolescente em Namek corrigida.
- Uma textura escrita em formato de 24 bits ou de 4/8 bits por pixel não fica mais com o conteúdo antigo nas últimas
  linhas.

## Novo: Budokai Tenkaichi 4
- **O mod da Team BT4 no mesmo app**, ao lado do BT3: personagens, formas e cenários novos. Na abertura do launcher
  você escolhe o jogo; cada um tem sua imagem, saves e ajustes. Precisa da sua imagem do mod **Beta 14 Rev2
  (SLUS-219.78)**: o jogo foi recompilado para essa versão (o motor é o mod recompilado, do BT3-Recomp).
- **Renderizador do ARMSX2 por padrão**: o do emulador de PlayStation 2 (o do PCSX2, com as correções do ARMSX2 para
  Mali e Adreno), a imagem mais fiel do mod. O do Dragon Rage fica como opção **experimental** (Ajustes e menu do jogo,
  com uma linha explicando a diferença). Nele, o cenário bem distante ainda pode sair com blocos de ruído
  colorido; no do ARMSX2 sai certo.
- **Shaders compilados de antemão**: o APK já traz os shaders do ARMSX2 compilados num Mali e num Adreno. A primeira
  imagem aparece em ~2,5 s em vez de ~4,5 s, sem as pequenas travadas de compilação nos menus e nas lutas.
- **16:9** (como a Team BT4 fez), 4:3 ou tela cheia; **drivers personalizados** também no BT4; **medidor de FPS**.
- **60 fps nas lutas (experimental)**: quando o aparelho não aguenta (menos de 57 atualizações por segundo durante
  3 s numa luta), o jogo volta sozinho para 30 e avisa: "Este aparelho não aguenta 60 fps nas lutas do BT4; voltando
  para 30". Nos aparelhos medidos ainda não aguenta: o RG Cube faz 30 e o Retroid Pocket 5 uns 36 a 41.
- A imagem pode ser escolhida pelo Downloads e continua abrindo depois de reiniciar o aparelho.

## Pacotes de textura
- **Agora funcionam no Android**: o jogo procurava os pacotes numa pasta interna do app, e um pacote importado nunca
  era usado ("instalou e nada mudou"). Agora são usados no **BT3** e no **BT4** (nos dois renderizadores). Medido com
  o pacote HD do SLUS-21678 (18.704 texturas, 10,3 GB): umas 3.000 texturas trocadas na luta gravada, a 30 fps
  estáveis no RG Cube e no Retroid Pocket 5.
- **Mais zips abrem**: os feitos no 7-Zip com Deflate64 ou BZip2 e os gravados em fluxo (sem compressão, com descritor)
  davam "compressão que o app não lê".
- **Espaço**: um pacote que não cabe é recusado antes de extrair, com o tamanho dele e o espaço livre ("Espaço
  insuficiente: este pack ocupa 10,3 GB depois de extraído e o aparelho tem 7,8 GB livres"), em vez de extrair por
  mais de um minuto e falhar com um erro do sistema em inglês. O que uma importação interrompida deixava para trás é
  apagado.
- O seletor de arquivos mostra também .rar e .7z, e o app explica como convertê-los para .zip.

## Online
- **Servidor ocupado**: quando o servidor gratuito chega ao limite diário (HTTP 429, erro 1027 do Cloudflare), o jogo
  para de perguntar por um tempo (30 s, dobrando até 10 min) em vez de insistir, passa para o servidor reserva, e o
  launcher mostra **"Servidor ocupado (limite diário), tente mais tarde"**. Jogar offline não muda nada.
- **Menos pedidos**: a fila da Classificatória só é consultada com a tela dela aberta; a busca pergunta a cada 4 s
  no primeiro meio minuto, depois a cada 8 s e, depois de dois minutos, a cada 15 s; um resultado que o rival ainda
  não confirmou é reenviado em 5 s, dobrando até um minuto (antes: a cada rodada).
- **Do PC** (vale para o Dragon Net e contra PC): sons do rival que sumiam com o rollback (Z-counter, vanish,
  teleporte), o som de carregar ki que ficava a luta inteira, a Solar Flare cegando como offline, vozes japonesas
  trocadas por falas em inglês, e uma dessincronia vira aviso em vez de encerrar a partida. Dois jogos com dados
  diferentes (uma ISO modificada contra a original) não são mais pareados.
- O log do app registra o motivo quando o launcher não consegue falar com o serviço online.

## Controles e launcher
- **Tela inteira**: no Retroid Pocket 5 (e em aparelhos com barra de navegação) o jogo às vezes começava numa área
  menor (1920x972) que o sistema esticava para a tela, com tudo 11% mais alto e faixas pretas dos lados em 16:9.
  Agora a imagem ocupa a tela toda desde o início, no BT3 e no BT4 (10 de 10 inícios certos; antes, 3 de 8 errados).
- **Controle clássico em tela quadrada** (RG Cube 720x720): L1/L2 e R1/R2 não se sobrepõem mais, e o losango de botões
  e o direcional ficam dentro da tela. Em 16:9 nada muda.
- **START + SELECT** também fecha o menu do jogo, não só abre.
- O controle de toque fica escondido ao abrir quando um controle com analógico está ligado.
- Na aba Drivers, a linha inteira escolhe o driver.
- A barra lateral do launcher rola (as nove páginas do BT3 em telas 1080p).
- Quando o Vulkan não abre (por exemplo com o driver MaliSX2 no Mali-G57), o jogo cai para OpenGL em vez de ficar
  com a tela preta.
- ADPF e modo de desempenho sustentado nos aparelhos que têm, para o sistema não baixar o clock no meio da luta.

## Notas de atualização
- Instala por cima da 0.8.x e mantém saves, ajustes e as ISOs importadas.
- Online: o modo PC conversa com o servidor oficial da versão 0.1.38; o Dragon Net continua com o Switch (DBT-NX).
