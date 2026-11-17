# Nimbox Android — WebRTC como reprodutor de vídeo (fase 0)

## Direção corrigida

O usuário já possui um Nimbox funcional. **Não reconstruir** autenticação, sessão GSSV, WebRTC de transporte, SDP/ICE, áudio, microfone, entrada, controle ou reconexão.

O Nimbox já recebe unidades de acesso **H.264 Annex-B** completas e com timestamps no fluxo existente:
`src/stream/stream_session.cpp` → `stream::StreamCallbacks::video` → `src/app/stream_player.cpp` → `videoLoop()`.

As duas rotas atuais são `MediaCodec` e `FFmpeg` dentro de `media::VideoDecoder`. A nova rota é um **terceiro reprodutor Android**, e não outra conexão WebRTC.

## Implementado nesta fase

1. Removeu-se `AndroidNativeWebRtcEngine.java`, que criava indevidamente `PeerConnection`, transceivers, canais e SDP/ICE duplicados;
2. Criou-se `android/app/src/main/java/com/rafaelngp/psbox/streaming/AndroidWebRtcVideoPlayer.java`:
   - usa a API `org.webrtc.HardwareVideoDecoderFactory` para H.264;
   - recebe unidades de acesso H.264 já desfragmentadas em `submitH264(byte[], captureTimeNs, keyFrame)`;
   - decodifica com `org.webrtc.VideoDecoder` e usa `org.webrtc.SurfaceViewRenderer` como saída de vídeo;
   - roda exclusivamente no Android; preserva os drivers C++ existentes em outras plataformas;
   - expõe contadores de quadros entregues e apresentados e faz limpeza do decodificador;
   - não gera `PeerConnection`, não negocia SDP nem faz login.
3. O SDK Android WebRTC permanece como dependência Gradle exclusiva do aplicativo Android.

**Não utilizamos código fonte do XStreaming.** A implementação é nova e usa API pública do WebRTC Android.

## Limitações e requisitos para tornar jogável

- **JNI ainda não ligado:** o `StreamPlayer::videoLoop()` C++ precisa encaminhar as unidades H.264 ao `AndroidWebRtcVideoPlayer`, sem iniciar outro transporte. Necessário lidar com SPS/PPS, IDR, mudanças de resolução, timestamps, reconexão e retorno de falhas.
- **Composição visual ainda pendente:** `SurfaceViewRenderer` adiciona uma superfície Android sobre o SDL/Vulkan, que pode **cobrir o painel, menu e controles touch do Nimbox**. O método `attachVideoSurface()` permanece sem chamada por esse motivo. É obrigatório projetar e testar a composição dos dois elementos antes de habilitar o modo.
- **Sem seletor funcional ainda:** não mostrar `WebRTC` ao usuário antes que a troca preserve áudio, controles, UI e vídeo real.
- **Baixa latência não garantida:** o caminho copia bytes H.264 comprimidos para Java, mas pretende manter o output decodificado em textura/EGL, sem YUV readback. Confirmar em Android real.
- **Recomenda-se manter FSR e demais filtros SDL desativados inicialmente nesse modo**, porque a saída de vídeo usará outra superfície; validar visual e integração posteriormente.

## Critérios

- [ ] CI compila Android com a nova classe e dependência WebRTC, sem alterações no resto;
- [ ] JNI recebe o H.264 do `StreamPlayer` existente;
- [ ] Primeiro quadro é mostrado dentro do Nimbox, sem ocultar controles/menu;
- [ ] Toque, gamepad, áudio e microfone preservados;
- [ ] H.264 keyframe, tamanho variável, reconnect e fallback exercitados;
- [ ] Somente depois: modo `Reprodutor Android: atual / WebRTC` no menu, **independente** de `Decodificador: GPU / CPU`;
- [ ] A/B de fps, tempo de render, latência e perdas no mesmo aparelho.

**Essa fase prepara o reprodutor, mas ainda não é um modo selecionável e funcional.**
