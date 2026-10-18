# Dragon Rage 0.7.3

**O online volta a funcionar**: o port de PC lançou a 0.1.37 e o serviço de partidas passou a exigi-la. Esta versão é
a 0.1.37.

## Da 0.1.37 do port de PC
- Online: uma linha na tela enquanto a partida começa (*Connecting to your rival…*, depois *Your rival "NOME" is
  here!*); acabou a pausa antes da partida que crescia com o ping; o nome do perfil não é mais apagado pela Partida
  rápida.
- Blending: um alfa de origem acima de 0x80 conta como no PS2 (o clarão da Solar Flare).
- Choques, a opção do Quadrado: um toque pode valer 2,25, o padrão a não ser que outro seja escolhido.
- Traduções dos textos do jogo (menus, HUD da batalha, diálogos, textos do próprio port) a partir de catálogos em
  `translations/`, que o app copia para a pasta do jogo. Por enquanto só existe o catálogo em inglês; a opção do disco
  europeu do instalador de PC não faz parte do Dragon Rage.

## Correções e ajuda
- **Celulares em que o jogo fechava na hora** (um Redmi com Snapdragon 8s Gen 3, HyperOS 2: *"cannot map the game
  heap"*): quando o endereço do heap do jogo (o do PS2) já está ocupado no processo, o heap agora vai para outro lugar
  abaixo de 2 GB, como no Windows.
- **Logs salvos sozinhos**: quando o jogo fecha com problema, ou um pacote de textura não instala, o app salva o log
  na pasta Downloads na hora, mostra o nome dele, e um botão **Enviar log** abre o compartilhamento. *"Mande esse
  arquivo para o desenvolvedor."*
- **Página de Texturas**: um cartão explicando como os pacotes funcionam (versão americana, o .zip do jeito que veio,
  o que esperar) com botão de log; depois de uma luta com um pacote ligado em que nenhuma textura dele foi usada, o
  app avisa (normalmente um pacote de outra versão do jogo) e salva o log.
- O aviso *"o jogo fechou"* mostra a causa real (memória, dados, motor, texturas) antes de um travamento que vem
  depois dela.

## Servidor online
- Configurações → **Servidor de partidas**: **Oficial** (o do port de PC, PC e Android juntos; o padrão) ou
  **Comunidade** (um serviço mantido por jogadores, do fork Tenkaichi3DecompAndroidOnline; lá só se encontram os que
  escolheram ele). A conexão direta (endereço e porta) não usa servidor.

## Notas de atualização
- Instala por cima da 0.7.x / 0.6.0 e mantém tudo.
- Se a atualização disser *"App não instalado: o pacote está em conflito com um pacote existente"*, o app do celular
  não é uma destas versões (outra build do código, com outra assinatura): salve a pasta `saves/` de
  `Android/data/com.dfdx047.dragonrage/files/`, desinstale, instale esta e devolva os saves.
