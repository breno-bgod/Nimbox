# NX Cloud Device Diagnostics — experimento isolado

> **Status:** experimental, sem liberação de jogos restritos. Não altera o catálogo da interface, o login, as credenciais, os headers, o fluxo WebRTC ou o decodificador. A branch deve permanecer separada de main até revisão e validação.

## Objetivo

Descobrir em qual etapa um título com restrição por tipo de dispositivo deixa de ficar acessível:

1. O título está na resposta GSSV (/v2/titles) para a conta autenticada neste cliente?
2. O título foi oferecido também na coleção Game Pass SIGL All Games? (A ausência aqui **não** exclui títulos comprados.)
3. Quais são details.allowedDeviceTypes e hasEntitlement?
4. **Somente se solicitado:** a criação de sessão (/v5/sessions/cloud/play) foi aceita ou recusada?

Nenhum resultado do comando deve ser interpretado como permissão para usar identidade de outro dispositivo.

## Windows: pacote experimental pronto para testar

O ZIP do GitHub Actions inclui o aplicativo `NimboPlay.exe`, a ferramenta `xcloud-cli.exe` e o iniciador `Testar-xCloud.cmd`. Extraia o ZIP completo e clique duas vezes em `Testar-xCloud.cmd`. Escolha Killer Instinct 2 Classic, Grand Theft Auto VI ou Fortnite; o modo de consulta nao inicia nenhuma sessao. O modo de sessao exige segunda confirmacao e direitos de acesso reconhecidos pelo servico.

O launcher nao salva tokens ou logs automaticamente. Nao compartilhe capturas do login que mostrem XUID, gamertag ou codigos. O teste real com conta ainda depende da execucao no Windows; o smoke test automatico so verifica compilacao e tela de ajuda da ferramenta.

## Uso no Linux ou Windows com build de desenvolvimento

Executar da raiz do repositório, com xcloud-cli já compilado para a máquina:

    build-host/xcloud-cli diagnose-title "Killer Instinct 2 Classic"

    build-host/xcloud-cli diagnose-title "Killer Instinct 2 Classic" --probe-session

    build-host/xcloud-cli diagnose-title "Grand Theft Auto VI"

    build-host/xcloud-cli diagnose-title <titleId> --probe-session

O comando utiliza o login por código de dispositivo que o xcloud-cli já possuía. Ele **não** solicita usuário/senha por argumentos. Evite executar com -v ao compartilhar logs; oculte gamertag, xuid, IPs e quaisquer identificadores pessoais.

### Leitura dos resultados

| Saída | Significado |
|---|---|
| NOT_IN_GSSV_TITLE_LIST | O login deste cliente não recebeu esse título na lista GSSV. **Não comprova** indisponibilidade em um Xbox físico. Pode ser questão de data, região, conta ou classificação. |
| game_pass_feed=no | Não consta na coleção SIGL Game Pass; pode aparecer legitimamente como título comprado e ainda ser transmissível. |
| allowedDeviceTypes=... | Lista informada pelo serviço. Se vier vazia, não presumir ausência de restrição. |
| PROBE_SKIPPED | Nome ambíguo ou hasEntitlement falso. Nenhuma sessão é solicitada. |
| SESSION_REJECTED ... device_type_refusal=yes | O serviço recusou a abertura e retornou a marcação de tipo de dispositivo não permitido. |
| SESSION_REQUEST_ACCEPTED | O servidor aceitou **a solicitação inicial**, não comprova SDP, ICE ou permissão final de gameplay. |
| SESSION_FAILED | O serviço criou o caminho mas a sessão já foi marcada como falha. |

### Procedimento de validação

1. Executar o comando sem --probe-session para um título comum que esteja disponível na própria conta (controle positivo).
2. Repetir com **Killer Instinct 2 Classic**. Comparar presença no GSSV, direitos e allowedDeviceTypes.
3. Se um único título for encontrado e a conta tiver direito de acesso, solicitar **explicitamente** --probe-session. O comando abre e imediatamente encerra uma sessão GSSV sem chamar o mecanismo WebRTC.
4. Com autorização do dono, comparar o status de disponibilidade com o mesmo usuário em um Xbox One original, **sem exportar tokens, atestação de hardware ou credenciais do console**.
5. Repetir com GTA VI somente após confirmação da disponibilidade do jogo no serviço e na região/conta da pessoa que está testando.
6. Registrar apenas: data, região do serviço, presença, tipo de erro e identificadores **públicos de produto/jogo**. Não incluir token, resposta JSON integral, conta ou identificadores privados em issues.

### Critérios de revisão

- Build e testes da ferramenta CLI sem regressões.
- Comando sem --probe-session não pode iniciar sessão.
- Tentativa só ocorre após opção explícita, identificação sem ambiguidades e hasEntitlement.
- Nenhuma alteração nos componentes de console (ps5/, ps4/), na interface, gssv.cpp, fluxo de login ou engines de mídia.
- Não mesclar na main até que o usuário revise o PR e os resultados reais sejam obtidos.

## Limitações

O comando não consulta um catálogo de console Xbox autenticado de forma diferente; ele só utiliza o cliente GSSV existente. Não tenta emular uma identidade Xbox One, não muda clientAppId, não altera token assinado e não promete contornar DeviceTypeNotAllowed. hydrateTitles() pode demorar e a Microsoft pode alterar o formato das APIs.

**Importante:** aceitar uma solicitação de sessão não garante streaming. Para esse teste inicial, nenhuma conexão real de vídeo é estabelecida.
