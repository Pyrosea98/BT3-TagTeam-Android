# Dragon Rage 0.8.0

**Dragon Net**: online com o port de Nintendo Switch (DBT-NX) num serviço próprio, que não para mais a cada nova
versão do port de PC. O online contra PC continua como opção.

## Dragon Net Battle, uma nova tela de online
- O **Dragon Net Battle** do jogo (menu principal) abre uma tela nova no visual do DBT-NX, para toque e controle, em
  inglês, português e espanhol: **Criar sala**, **Entrar em sala**, **Partida rápida**, **Classificatória** e
  **Chat geral**, com o número de jogadores online.
- **Jogar contra**: **Android + Switch** (Dragon Net, o padrão) ou **PC** (o serviço do port de PC, que pede a versão
  mais nova dele). Também em Ajustes → Online. Classificatória e chat são do Dragon Net.
- **Classificatória**: perfil com rank, Elo, vitórias, derrotas e posição; busca individual ou em equipe pelo nível;
  os melhores da classificação. Os pontos mudam quando os dois jogos mandam o mesmo resultado; sair de uma luta
  ranqueada antes do fim conta como derrota.
- As salas mostram um código grande para copiar; Entrar em sala aceita código colado. Lutas ranqueadas e rápidas: sem
  limite de tempo, velocidade padrão, rollback.
- Controle: direcional para escolher, A aceita, B volta, Y troca Android + Switch / PC, Start edita seu nome.

## Por dentro
- O Dragon Net usa o netcode da 0.1.37 do port de PC com a versão **5001**; o serviço separa cada versão, então jogos
  que não podem jogar juntos nunca se encontram (`docs/DRAGON_NET.md`).
- `port/dragonnet`: o serviço (o Worker do DBT-NX, aceitando várias versões ao mesmo tempo) e os testes dele.
- O cartão de online do app segue o modo escolhido e avisa quando o serviço Dragon Net ainda não aceita esta versão.
