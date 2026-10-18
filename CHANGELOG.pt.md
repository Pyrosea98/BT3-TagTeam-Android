# Dragon Rage 0.8.0

**Chegou o Dragon Net**: online com o port de Nintendo Switch (DBT-NX) num serviço próprio, com Classificatória, chat
geral e jogadores online, que não para mais a cada versão nova do port de PC. O online contra PC continua como opção.
Esta versão também traz tudo da 0.7.3 (a 0.1.37 do port de PC, a correção do heap em alguns Snapdragon, logs salvos
sozinhos) e um log para tudo o que o app faz.

## Dragon Net: Android + Switch
- **Dois tipos de online**, escolhidos em **Jogar contra**:
  - **Android + Switch (Dragon Net)**, o padrão: o Dragon Rage e o DBT-NX no servidor oficial deles (o do fork
    Tenkaichi3DecompAndroidOnline), para Android × Android e Android × Switch. Não depende das versões do port de PC,
    então uma versão nova do PC não desliga mais o online.
  - **PC**: salas e partida rápida contra o port de PC, no serviço dele (que pede a versão mais nova do port de PC).
- Escolha no jogo (Dragon Net Battle → Jogar contra, ou **Y** no controle) ou no app (Ajustes → **Online**). Os dois
  jogadores precisam escolher o mesmo.
- Jogos de versões que não podem lutar juntas nunca se encontram: o serviço separa cada versão.

## Uma nova tela Dragon Net Battle
- O **Dragon Net Battle** do jogo (menu principal) abre uma tela nova no visual do DBT-NX, feita para toque e controle,
  em **inglês, português e espanhol**.
- **Criar sala**: escolha o combate (individual, equipe, DP 10/15/20) e abra; aparece um código grande de 6
  caracteres, um toque copia.
- **Entrar em sala**: digite ou **cole** o código.
- **Partida rápida**: escolha o combate; quantos esperam em cada fila; a busca termina sozinha depois de três minutos.
- **Classificatória** e **Chat geral** (Dragon Net).
- Seu **nome** num selo no alto: toque nele (ou Start) para mudar.
- O número de **jogadores online** (Android e Switch) embaixo.
- Mensagens claras para cada problema: o outro jogo em outra versão, ninguém procurando, os dois jogos sem se alcançar,
  o serviço ainda não aceitando esta versão.
- Controle: direcional para escolher, A aceita, B volta, Y troca Android + Switch / PC, Start edita o nome.

## Classificatória (Elo)
- Um **perfil** criado na primeira vez que você abre a Classificatória: rank (Bronze a Mestre), **Elo**, vitórias,
  derrotas e posição.
- **Buscar rival**, individual ou equipe: você enfrenta um jogador do seu nível.
- Lutas ranqueadas: sem limite de tempo, velocidade padrão, rollback.
- No fim os dois jogos mandam o resultado; **os pontos só mudam quando os dois batem**. Sair de uma luta ranqueada
  antes do fim conta como derrota.
- **Classificação**: os melhores; **Atualizar perfil**.

## Chat geral
- Um chat para todos no Dragon Net, Android e Switch, na tela online.

## O app (launcher)
- Ajustes → **Online**: Jogar contra (Android + Switch / PC).
- O **cartão de online** do Início segue o modo escolhido: Dragon Net funcionando, Dragon Net sendo atualizado (o
  serviço ainda não aceita esta versão), PC precisando de atualização, sem resposta.
- **Como funciona o online** virou um guia completo, nas três línguas: os dois tipos de online, como jogar, a
  Classificatória, o chat e os jogadores online, o que cada mensagem quer dizer e o que fazer, e um botão **Enviar log**.

## Logs para tudo
- O app guarda um **diário do que faz** (`app_activity.txt` na pasta do jogo): instalar o jogo pela ISO (o arquivo, o
  espaço livre, cada etapa, quanto tempo levou), pacotes de textura, mods, drivers, atualizações, a verificação do
  online, o renderizador, cada vez que o jogo abre, e o **erro inteiro** quando algo falha.
- **Qualquer falha salva um log em Downloads sozinha** e mostra uma janela com **Enviar log**: a ISO que não instalou,
  um driver, um mod, uma atualização, um pacote de textura, o jogo fechando com problema.
- O jogo guarda as **três últimas execuções** (`bt3_log.txt`, `.1`, `.2`): um problema visto uma execução depois
  continua lá.
- O log exportado tem tudo isso: o aparelho, o diário do app, a pasta do jogo (tamanhos, pacotes, mods), os ajustes, as
  três últimas execuções do jogo e o log do sistema.

## Da 0.1.37 do port de PC (na 0.7.3)
- Online: uma linha na tela enquanto a partida começa (*Connecting to your rival…*, depois *Your rival "NOME" is
  here!*); acabou a pausa antes da partida que crescia com o ping; o nome do perfil não é mais apagado pela Partida
  rápida.
- Blending: um alfa de origem acima de 0x80 conta como no PS2 (o clarão da Solar Flare).
- Choques, a opção do Quadrado: um toque pode valer 2,25, o padrão a não ser que outro seja escolhido.
- Traduções dos textos do jogo a partir de catálogos em `translations/`, copiados pelo app para a pasta do jogo (por
  enquanto só inglês).

## Correções e ajuda (na 0.7.3)
- **Celulares em que o jogo fechava na hora** (um Redmi com Snapdragon 8s Gen 3, HyperOS 2: *"cannot map the game
  heap"*): o heap do jogo agora vai para outro lugar quando o endereço do PS2 já está ocupado.
- **Página de Texturas**: um cartão explicando como os pacotes funcionam (versão americana, o .zip do jeito que veio, o
  que esperar) com botão de log; depois de uma luta com um pacote ligado em que nenhuma textura dele foi usada, o app
  avisa.
- O aviso *"o jogo fechou"* mostra a causa real (memória, dados, motor, texturas).
- Changelogs: um arquivo por língua, mostrado pela janela de atualização do app.

## Para desenvolvedores
- `port/dragonnet`: o código do serviço do Dragon Net (salas, partida rápida, Classificatória, chat, jogadores online;
  aceita várias versões ao mesmo tempo e separa cada uma) com os testes, para o servidor oficial.
- `docs/DRAGON_NET.md` (inglês e espanhol): o que Android e Switch precisam ter igual (versão 5001), o fluxo da
  Classificatória, o digest do resultado, o chat e a presença, e como atualizar o serviço.

## Notas de atualização
- Instala por cima da 0.7.x e da 0.6.0 e mantém tudo.
- O Dragon Net precisa do servidor rodando o `port/dragonnet` desta versão (feito junto com este lançamento). Se o cartão de online
  disser *"Dragon Net está sendo atualizado"*, escolha **Jogar contra: PC** enquanto isso.
- Se a atualização disser *"App não instalado: o pacote está em conflito com um pacote existente"*, o app do celular é
  outra build do código (outra assinatura): salve a pasta `saves/` de
  `Android/data/com.dfdx047.dragonrage/files/`, desinstale, instale esta e devolva os saves.
