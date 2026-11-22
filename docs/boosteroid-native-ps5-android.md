# NimboPlay — Provedor Boosteroid nativo (PS5 e Android)

**Estado:** cliente HTTPS nativo, login QR e biblioteca Horizon em implementação experimental; transporte ainda pendente.
**Branch:** `experiment/boosteroid-native-ps5-android`, baseada na `main`.
**Objetivo:** usar o Horizon e o pipeline de decodificação/entrada existentes, sem Electron, para conectar o serviço Boosteroid em PS5 e Android.
**Não é gameplay funcional:** login e catálogo estão ligados ao app; reprodução e sinalização Boosteroid ainda faltam.

## Referências verificadas e seu papel

| Código existente | Evidência no projeto | Uso proposto |
| --- | --- | --- |
| NimboPlay/PSBox: `stream/stream_session.h` | saída H.264 Annex-B + Opus por callbacks, mas depende de `GssvClient` | reaproveitar formatos de mídia, não sinalização Xbox |
| NimboPlay: `media/decoder.h` | `VideoDecoder` H.264 e `AudioDecoder` Opus, com backends próprios | decodificar áudio/vídeo vindos de novo transporte |
| NimboPlay: `input/controller.h` e `stream/input_packet.h` | captura e envio atual de gamepad | capturar o mesmo controle, com encoder Boosteroid diferente |
| [ZERODROID 0.8.2](https://github.com/elzerdodata/ZERODROID) | Switch nativo em C++17, login QR, biblioteca, H.264, Opus, `libpeer`, SRTP/SCTP, controles | estudar autenticação, fila, gateway e protocolo próprio |
| [OpenStroid](https://github.com/OpenCloudGaming/OpenStroid) | sessão, catálogo e inicialização Boosteroid documentados no código do servidor | comparar APIs e formatos; **não** exigir Electron |
| [BoosteroidATV](https://github.com/luizhdramos/BoosteroidATV) | implementação tvOS Swift/WebRTC/H.264 que declara login e gameplay | estudar fluxo WebRTC de transporte separado do xCloud |
| [OpenNOW Boosteroid Provider](https://github.com/OpenCloudGaming/opennow-provider-boosteroid) | plugin headless de controle/mídia; somente testes offline comprovados | modelo para isolamento e interface de provedor; **não** assumir gameplay validada |

Projetos de terceiros são clientes não oficiais. Sua compatibilidade pode mudar com alterações do serviço. Antes de importar qualquer trecho, preservar licenças e notices (OpenStroid/ZERODROID Apache-2.0; BoosteroidATV MIT).

## Por que um processo Electron não pode ser a solução

O PR #32 tem a apresentação Horizon do Boosteroid no **Windows**, com um serviço Electron auxiliar e um `OpenStroid.exe`. Não existe executável Electron equivalente dentro do app Android/PS5. Copiar `OpenStroid.exe` ou liberar um `#if` não implementa autenticação/streaming nessas plataformas.

Por isso, esta branch nasce da **main**, isolada do PR #32. Não modifica `GssvClient`, `StreamPlayer`, `StreamSession`, WebRTC xCloud, telas existentes nem o funcionamento oficial.

## Primeiro incremento implementado

- `src/boosteroid/native_contract.h`: modelo de autorização QR **público**, catálogo com ID do provedor, estados do serviço, escolha de transporte e callbacks de mídia codificada.
- `src/boosteroid/native_contract.cpp`: projeção defensiva de JSON pública, filtragem de IDs/capas, limites para H.264 Annex-B e Opus, máquina de estados sem tokens.
- `CMakeLists.txt`: compila o módulo no `xcloud_core` para todos os destinos, inclusive **Android e PS5**, sem ativar o serviço ainda.
- `tests/test_main.cpp`: testes de QR com URL HTTPS, bibliotecas/IDs duplicados, não exportação de `sessionHandoff`, H.264, Opus e transições de estado.

**Limite:** compilar a interface para PS5 não demonstra que a autenticação, o transporte ou o decoder no hardware funcionem. A CI não compila um eboot PS5 sem toolchain; validar Android não substitui teste de dispositivo.

## Integração experimental de login QR e biblioteca

- **Cliente nativo:** src/boosteroid/native_client.h/.cpp usa o HTTPS
  existente do PSBox para o fluxo Android TV do Boosteroid, QR de autorização,
  perfil e jogos instalados. Sessão criptografada com auth::sealAccount.
- **Worker nativo:** src/boosteroid/native_worker.h/.cpp consulta login,
  perfil e biblioteca fora da renderização, com limites de polling e logout.
- **Horizon compartilhado:** tela QR e catálogo desenhados em C++, sem
  substituir a Home Xbox. O modo é uma superfície independente e segura.
- **Acesso experimental:** segurar L2 e apertar R1 na Home/tela de login,
  ou tocar a marca NimboPlay apenas na tela de login Xbox do Android. Círculo volta; Triângulo atualiza;
  R3 sai da conta Boosteroid. PS5 exibe o QR sem navegador.
- **Sem falso Jogar:** as capas são visíveis, mas o botão de streaming
  permanece bloqueado até haver uma sessão e player nativos confirmados.

Base técnica de protocolo: ZERODROID 0.8.2, Apache-2.0, cliente Android TV.
Compilação e testes simulados não confirmam conta real, máquina ou gameplay.
## Etapas de integração, em ordem

1. **Autenticação independente de Electron:** estudar o cliente nativo ZERODROID e o fluxo de dispositivo do OpenStroid; implementar HTTP seguro sobre `xc::net` existente, persistência privada por plataforma, refresh e QR dentro do Horizon. Nenhum token no `ui::AppUi`.
2. **Catálogo e sessão:** consumir dados reais do backend Boosteroid, normalizar para `boosteroid:<appId>`, separar cache/perfil/conta dos IDs Xbox. Manter a flag de disponibilidade desabilitada até login real funcionar.
3. **Transporte nativo:** primeiro verificar a compatibilidade de WebRTC/H.264/Opus com a API Boosteroid usando a implementação Swift como referência; investigar também a rota Android TV UDP/AES usada no código Switch. São protocolos distintos: **não misturar SDP/SCTP do Xbox com mensagens Boosteroid**.
4. **Renderização nativa:** ao receber H.264 Annex-B, passar ao `VideoDecoder` e à saída GPU já usada pela plataforma; Opus ao `AudioDecoder` e saída existente. No Android, preferir o caminho de MediaCodec/Surface disponível; no PS5, verificar Videodec2 e GPU. Não reconstruir os decodificadores.
5. **Controle/áudio:** mapear DualSense e controles Android para pacotes Boosteroid, incluindo analógicos, gatilhos e retorno ao Horizon. Nunca reutilizar cegamente `gamepadReport` do xCloud.
6. **Teste real:** login QR, biblioteca, máquina/fila, conexão, primeiro frame, áudio contínuo, controle, desligar stream, troca de serviço, reconexão. Repetir em Android e PS5 com hardware. Não liberar o botão Jogar antes de passar.

## Critérios de aceite

- Xbox inalterado após testes de regressão.
- Login Boosteroid nativo e catálogo reais sem Electron nem navegador embutido no PS5/Android.
- Primeiro frame H.264 e áudio Opus entregues aos decodificadores existentes no hardware.
- Gamepad jogável, saída de sessão, cancelamento e recuperação de falhas.
- CI de Android verde; build PS5 via toolchain correspondente e testes de console.
- Nenhuma atualização da `main` antes dessas condições.

**Resultado de hoje:** contrato e testes offline adicionados; fluxos reais de conta e streaming permanecem pendentes. Nunca apresentar o módulo como Boosteroid funcional no PS5/Android sem evidência do serviço.
