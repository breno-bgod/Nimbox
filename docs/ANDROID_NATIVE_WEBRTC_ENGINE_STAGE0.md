# Nimbox Android — terceiro reprodutor H.264 usando WebRTC Video

## Propósito

O Nimbox tem autenticação, GSSV, SDP/ICE, transporte `libdatachannel`, áudio, input e reconexão próprios. **Nenhum deles foi duplicado ou substituído.**

O backend opcional usa **apenas o decodificador de vídeo H.264** do SDK Android WebRTC. `org.webrtc.HardwareVideoDecoderFactory` costuma utilizar o Android MediaCodec internamente, mas oferece uma implementação diferente de fila/entrega de quadros. Não afirmar desempenho superior sem benchmarks.

## Implementação integrada

- `src/stream/stream_session.cpp` continua entregando os access units H.264/Annex-B;
- `src/app/stream_player.cpp` continua com o mesmo `videoLoop`, áudio, input e conexão;
- `src/media/decoder.cpp` aceita, exclusivamente no Android, o terceiro backend `androidWebRtcVideo`;
- `src/media/android_webrtc_decoder.{h,cpp}` converte o access unit recebido para chamada JNI na `MainActivity`, preservando a tag RTP;
- `MainActivity.java` cria e controla `AndroidWebRtcVideoPlayer`;
- `AndroidWebRtcVideoPlayer.java` alimenta o hardware H.264 decoder WebRTC, recebe quadros I420 e os empacota para o renderer atual do Nimbox;
- O mesmo `display::drawYuv420` já existente apresenta o quadro, **sem adicionar outra Android SurfaceView por cima do menu/touch**;
- Menu Android durante partida e configurações globais/por jogo oferecem escolha entre `MediaCodec`, `FFmpeg` e `WebRTC Video`;
- Configurações no Android aceitam 0,1,2. Demais plataformas continuam limitadas a 0,1;
- Se o WebRTC video codec não abrir ou parar de produzir frames, o pipeline volta para o MediaCodec nativo (com novo keyframe).

## Limitação explícita da primeira integração

Esta primeira integração faz **cópia de H.264 comprimido para Java e cópia de I420 decodificado de volta a C++**. Isso permite o **primeiro teste real de gameplay** com os painéis, áudio, input e render SDL/Vulkan existentes, mas pode ter custo extra e não é zero-copy. O desempenho e a latência ainda não foram medidos em aparelho real.

A futura otimização de `Surface`/`EGL` para zero-copy é trabalho separado, pois deve compor vídeo sem cobrir a UI e controles touch SDL. Não anunciar o modo como solução de performance definitiva.

## Testes necessários
- [ ] Gradle + CMake compila todas as plataformas; o novo arquivo C++ é compilado só no Android;
- [ ] Instala Nimbox Alpha, mantém todos os menus e opção de decodificação;
- [ ] Em jogo, selecionar WebRTC Video: logs mostram `android webrtc video: connected` e `h264 decoder: Android WebRTC video-only backend active`;
- [ ] Quadro da gameplay aparece e a tag RTP corresponde ao vídeo recebido;
- [ ] Áudio, entrada, touch, microfone e interface continuam funcionais;
- [ ] Troca pelo controle entre três modos funciona sem nova sessão xCloud;
- [ ] Conexão perdida e retomada, keyframe, mudança 720p/1080p e fallback;
- [ ] A/B com mesma rede/jogo/bitrate, gráficos de fps/latência/drops/CPU/temperatura;
- [ ] Se falhar, relatar logs e não mesclar na main.

PR experimental, sem alteração de `main`.
