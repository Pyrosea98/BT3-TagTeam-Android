# Dragon Rage 0.8.1

**Dragon Net em três servidores**, para o online continuar funcionando quando um deles cai ou gasta os pedidos
gratuitos do dia. Por Ariel R Rodrigues (pull request #3).

## Dragon Net
- **Servidor A** (o principal, `mitigo-rooms`): salas, partida rápida e jogadores online.
- **Servidor B** (o reserva, do DBT-NX): quando o servidor A falha três vezes seguidas, o jogo passa sozinho para o B
  até 00:00 UTC (21:00 em Brasília) e depois volta para o A. Dois jogadores precisam estar no mesmo servidor para se
  encontrar: se a sala não for encontrada, tente de novo um pouco depois.
- **Servidor C** (`bt-ranked`): a **Classificatória**, as buscas e as salas dela.
- Outros servidores podem ser escolhidos com arquivos de uma linha na pasta do jogo: `dragonnet_url.txt` (A),
  `dragonnet_url_b.txt` (B), `dragonnet_ranked_url.txt` (C).
- **Chat geral desligado por enquanto**: fica apagado no menu do Dragon Net Battle e volta numa próxima versão.

## Menos pedidos aos servidores
Os servidores gratuitos têm um limite de pedidos por dia, então o jogo pergunta menos vezes:
- os jogadores online a cada 5 minutos (antes 90 segundos);
- o perfil da Classificatória a cada minuto (antes 15 segundos), e a busca ranqueada a cada 4 segundos (antes 2);
- uma sala esperando o rival a cada 3 segundos no primeiro minuto e a cada 6 depois (antes 2 e 4); o código da sala
  continua durando uns dez minutos;
- as filas da partida rápida (modo PC) a cada 90 segundos (antes 30).

## Outros
- O guia de online e os ajustes do app não falam mais do chat.
- Um APK de teste que tinha entrado no código por engano foi removido.

## Notas de atualização
- Instala por cima da 0.8.0 (e da 0.7.x) e mantém tudo.
- Tudo da 0.8.0 continua: a tela Dragon Net Battle, a Classificatória, o online contra PC, os logs para tudo e o guia
  de online nas três línguas.
