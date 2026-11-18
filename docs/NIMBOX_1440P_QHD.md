# Nimbox Alpha — Resolução 1440p (QHD) explícita

## Escopo
A seleção de resolução é compartilhada pelos builds Android, Windows, Linux, PS5 e demais plataformas que utilizam a UI nativa. O Web Control oferece a mesma opção.

| Menu | Valor salvo no perfil | Configuração global |
| --- | ---: | --- |
| 720p | 1 | `720p` |
| 1080p | 0 | `1080p` |
| 1440p (QHD) | 3 | `1440p` |
| Melhor (Auto) | 2 | `best` |

Os valores anteriores (0, 1 e 2) não mudaram. Os arquivos de configuração antigos continuam válidos.

## Negociação
Selecionar QHD antes de iniciar um jogo faz o Nimbox enviar o perfil de dispositivo QHD existente e solicitar o alias `1440`, sem depender de cache/probe prévio de 1440p. No menu durante o jogo, a mesma opção solicita `1440` diretamente; a mudança efetiva pode exigir reiniciar a sessão, pois o perfil de dispositivo é negociado ao iniciá-la.

O modo Melhor (Auto) mantém sua decisão automática existente. O servidor xCloud pode responder com 1080p ou outro formato por elegibilidade, jogo, dispositivo ou região: o rótulo do menu representa a **qualidade solicitada**, não a recebida.

O Nimbox já apresenta a resolução real do fluxo nas estatísticas (`width` / `height`) e nos logs de renderização; use esses valores para confirmar `2560×1440`. A presença da opção não prova que a plataforma física poderá decodificar 1440p.

## Checklist por plataforma
1. Selecionar 1440p nas Configurações gerais; reiniciar app e confirmar persistência.
2. Selecionar 1440p em um perfil de jogo e verificar que o restante das preferências fica intacto.
3. Iniciar sessão e conferir nos logs o pedido `1440`; confirmar separadamente as dimensões efetivas.
4. Alternar 720p, 1080p, QHD e Auto durante a transmissão; validar áudio/controles/retomada e persistência.
5. Repetir no Android (GPU/CPU/Surface), Windows, Linux e PS5, conforme hardware disponível.
6. No controle web por QR, selecionar QHD mesmo sem resolução alta previamente registrada.
7. Em contas/regiões sem QHD, confirmar fallback do servidor sem falha da sessão.

## Proteções
Nenhum codec novo é adicionado. O caminho de autenticação, transporte, entrada e áudio do Nimbox permanece igual, e o motor Surface Android não substitui os decodificadores CPU/GPU.
