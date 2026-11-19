# NX Cloud Device Diagnostics — experimento isolado

> **Status:** experimental, sem liberação de jogos restritos. Não altera o catálogo da interface, o login, as credenciais, os headers, o fluxo WebRTC ou o decodificador. A branch deve permanecer separada de main até revisão e validação.

## Objetivo

Descobrir em qual etapa um título com restrição por tipo de dispositivo deixa de ficar acessível:

1. O título está na resposta GSSV (/v2/titles) para a conta autenticada neste cliente?
2. O título foi oferecido também na coleção Game Pass SIGL All Games? (A ausência aqui **não** exclui títulos comprados.)
3. Quais são details.allowedDeviceTypes e hasEntitlement?
4. **Somente se solicitado:** a criação de sessão (/v5/sessions/cloud/play) foi aceita ou recusada?

Nenhum resultado do comando deve ser interpretado como permissão para usar identidade de outro dispositivo.

## Android: APK Alpha com diagnostico integrado

**Nao requer Windows nem xcloud-cli.** No APK debug NimboPlay Alpha, entre na conta Xbox normalmente e aguarde a biblioteca aparecer. Na tela inicial, abra o botao flutuante **NX Teste** (o botao fica oculto durante o streaming).

1. Selecione **Killer Instinct 2 Classic**, **Grand Theft Auto VI** ou **Fortnite**.
2. Escolha **Consultar catalogo e dispositivos** para verificar a lista GSSV sem iniciar nenhuma sessao de jogo.
3. Opcionalmente, escolha **Testar solicitacao de sessao**; a segunda confirmacao e obrigatoria. Apenas um jogo encontrado sem ambiguidades e com `hasEntitlement` verdadeiro pode ser testado.
4. Leia a resposta no painel e utilize **Copiar resultado** para compartilhar apenas o texto tecnico, sem capturas de login.

O menu e o JNI existem apenas no aplicativo Android, enquanto o botao visual e exposto somente quando o APK tiver a flag Android **debuggable** (o artefato `nimboplay-android-alpha`). O programa reutiliza **o worker C++ e o GSSV autenticado do proprio aplicativo**; nao cria um segundo login e nao altera autenticacao, device info, controle, video ou audio.

**Limitacoes:** o resultado depende dos titulos enviados para a conta/cliente naquele instante. A ausencia na lista nao significa indisponibilidade universal. O campo allowedDeviceTypes nao constitui atestacao completa. O teste de sessao apenas pede/encerra a sessao; nenhum gameplay e iniciado. Descarte o resultado se o aplicativo for fechado durante a consulta. A validacao em aparelho fisico deve ocorrer antes de afirmar que o fluxo funciona.

## Observacoes dos testes Android (18/11/2026)

A pessoa que testou no NimboPlay Alpha encontrou **Killer Instinct 2 Classic** na biblioteca e recebeu uma recusa de reproducao com a mensagem indicando Xbox One, Series X|S e Samsung TV. O app so exibe esse erro quando o caminho de gameplay falha e o codigo reconhece \`DeviceTypeNotAllowed\`; a lista legivel vem de \`allowedDeviceTypes\`.

**GTA VI nao apareceu em nenhuma pesquisa feita no app.** Isso e um resultado de busca local, e nao comprova que o titulo nao esta na resposta GSSV ou que nunca sera autorizado. Os titulos GSSV sao retornados sem nome; para comparar por nome e necessario ter a correspondencia de productId/titleId obtida do catalogo.

O botao NX Teste da primeira revisao podia ficar preso porque chamava \`hydrateTitles()\` para todos os titulos e nao exibia progresso ou tempo limite. A revisao seguinte:

- Reaproveita os nomes/IDs que a biblioteca do proprio NimboPlay ja carregou, sem hidratar milhares de produtos novamente.
- Consulta GSSV e cruza os IDs reconhecidos pela biblioteca.
- Distingue \`NAO_ENCONTRADO_CATALOGO_LOCAL\` de \`ENCONTRADO_LOCALMENTE_MAS_NAO_VINCULADO_GSSV\`. O primeiro **nao prova** ausencia na lista GSSV, cujos nomes nao vem no protocolo.
- Exibe estagio e tempo de espera no Android; apos 120 segundos, mostra timeout em vez de permanecer indefinidamente em loading. A requisicao nativa em curso nao e interrompida por fechar a janela.
- Mantem uma solicitacao em fila separada do \`g_command\` para nao a perder quando o worker recebe uma atualizacao, comando de login ou outra operacao.

Primeiro testar Killer Instinct 2 Classic em **Consultar catalogo**; em seguida **Testar solicitacao de sessao** (com confirmacao). Testar GTA VI em **Consultar catalogo** e anotar se aparece \`NAO_ENCONTRADO_CATALOGO_LOCAL\`; o status nao significa que uma hipotetica sessao de GTA VI foi recusada.

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
