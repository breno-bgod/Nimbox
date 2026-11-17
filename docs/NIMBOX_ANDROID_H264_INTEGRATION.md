# Integração Android H.264 — Nimbox Alpha (branch correta)

## Por que este ramo existe

O PR #17 foi criado por engano a partir de `main`, que ainda tem a apresentação original do PSBox. Seu APK, portanto, não continha os menus, painéis e configurações já preparados na cadeia de branches do Nimbox. **Não utilizar o APK do PR #17 para homologar o Nimbox.**

Este ramo deriva de `feature/nimbox-settings-unified-web-v1` (PR #16), preservando:
- Menu e painéis nativos Nimbox e seus temas/ambientes;
- Configurações nativas unificadas com Web Control e identidade visual do QR Code;
- Branding Android **Nimbox Alpha** presente no Gradle/manifest da base;
- `applicationIdSuffix = ".nimboxalpha"` para instalação experimental lado a lado;
- Imagens de ícone/banner próprias do Nimbox e artefatos de CI da cadeia visual.

## Alterações de H.264 portadas do PR #17

1. `MainActivity.preferredAvcDecoder()`: identifica AVC por MediaCodecList, priorizando decodificadores de hardware com indicação de baixa latência;
2. `HwDecoder` Android: tenta esse codec via `AMediaCodec_createCodecByName` e mantém fallback genérico;
3. Diagnóstico de componente real no Android, buffers vazios, queda de outputs antigos e tamanho de quadro com `BufferInfo.size`;
4. Mensagens do decodificador H.264 com identificação correta da plataforma.

Não houve alteração no núcleo de transmissão WebRTC, nos comandos de controle, no decoder PS5 ou nas implementações de vídeo Windows/Linux. Não copiamos código AGPL do XStreaming.

A base PR #16 já contém o workaround Windows para `libdatachannel` (`<algorithm>`), portanto **não sobrescrever** seu CMake com o arquivo de `main`.

## Proteção contra APK errado

O CI Android valida, depois do build, os metadados reais do arquivo via `aapt dump badging`. Exige:
- Nome exibido: `Nimbox Alpha`;
- Identificador experimental: `com.rafaelngp.psbox.nimboxalpha`;
- Artefato: `nimbox-android-alpha`, com APK `Nimbox-Android-Alpha.apk`.

O nome interno Java `com.rafaelngp.psbox` permanece por dependências SDL/JNI; ele **não** significa que o binário tenha a interface original do PSBox. Uma mudança de namespace definitivo requer migração separada.

## Homologação obrigatória

- [ ] CI Android compilou e verificou os metadados e alinhamento 16 KB;
- [ ] APK instala e mostra **Nimbox Alpha** e não o PSBox;
- [ ] Tela de menu, painéis, temas, ajustes nativos e Web Control do PR #16 permanecem;
- [ ] Reprodução H.264 hardware/software e fallback funcionam no aparelho;
- [ ] A/B no mesmo jogo, conexão e bitrate com logs de decoder e quadros descartados;
- [ ] Interface e build de outros alvos sem regressão;
- [ ] Aprovação explícita antes de merge/publicação.

O PR deve ficar em **draft** e **não ser mesclado na main** enquanto a cadeia de UI do Nimbox não estiver homologada.
