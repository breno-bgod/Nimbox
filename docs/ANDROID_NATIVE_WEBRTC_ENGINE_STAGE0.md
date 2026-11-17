# Nimbox — motor WebRTC Android adicional (fase 0, **não jogável**)

## Objetivo explicitamente solicitado
O usuário quer **mais um motor de streaming no Android**, sem substituir MediaCodec (GPU) ou FFmpeg (CPU), e sem tocar nas implementações PS5/desktop/Linux.

## Descoberta essencial
O Nimbox **já usa WebRTC**, via libdatachannel em `src/stream/stream_session.cpp`:
- `rtc::PeerConnection`, negociação SDP/ICE com `GssvClient`;
- canais `chat`, `control`, `input`, `message`;
- H.264 RTP com `H264Receiver`, NACK/keyframes, Opus e retornos de input.

A escolha `videoDecoder` é de codec **dentro desse mesmo stream**:
- MediaCodec/Hardware (Android) e
- FFmpeg/Software.
Ela não seleciona implementações WebRTC.

## Nova arquitetura (exclusivamente Android)
Seletor futuro **Motor de transmissão**, independente de decodificador:
1. `Nimbox WebRTC (libdatachannel)` — existente e default.
2. `WebRTC Android Native (libwebrtc)` — novo, opcional e experimental.

No futuro o motor alternativo precisa fazer a sessão **inteira**: SDP/ICE/GSSV, canais de input, áudio, vídeo, reconexão e apresentação de quadros numa superfície Android com o mínimo possível de cópias de CPU. Inicialmente continuará oculto e não será oferecido como opção funcional.

## O que foi de fato implementado na fase 0
- Dependência de WebRTC Android distribuída em Maven Central: `io.github.webrtc-sdk:android:150.7871.04`. Esta dependência é diferente de libdatachannel. Distribuição do SDK possui licenciamento e NOTICE próprios; verificar dependências/redistribuição antes do merge.
- Classe isolada `AndroidNativeWebRtcEngine` para:
  - Inicializar `PeerConnectionFactory` e componentes de áudio/vídeo;
  - Criar `PeerConnection`, transceivers de mídia e os quatro data channels;
  - Gerar uma oferta SDP local, receber resposta SDP e candidatos ICE;
  - Expor eventos de conexão, canais binários e vídeo decodificado por callback.
- Nenhum código foi copiado dos arquivos do XStreaming. A implementação é nova, baseada nas APIs públicas WebRTC.
- Não há alteração no `StreamPlayer`, `StreamSession`, `Settings`, menus, PS5, Windows ou Linux; o motor atual permanece o único selecionável.
- O novo código **não tem a ponte GSSV**, handshake completo dos canais, envio de controle/gamepad para xCloud, controle de áudio, renderer nem reconexão — **não deve ser descrito como motor jogável**.

## Trabalho pendente antes da opção de seleção aparecer
**Fase 1 — protocolo:** implementar bridge C++/JNI que reutilize autenticação e `GssvClient` Nimbox (sem duplicar logins), adapte oferta/resposta/candidatos SDP/ICE e Handshake/Transaction dos canais. Validar data channels e primeiro pacote de vídeo em stream real.

**Fase 2 — áudio/vídeo:** renderizar `VideoTrack` por superfície Android sem copiar quadro YUV para memória CPU; integrar composição SDL/Nimbox e gerenciamento de foco/rotação/resume; áudio Opus e microfone sem rota duplicada.

**Fase 3 — experiência:** conduzir input de controles do Nimbox, vibração, touch, teclado virtual, microfone, conexão/queda/reconexão; só habilitar opção de motor quando tudo funcionar e houver fallback para o existente.

**Fase 4 — comparação:** com a mesma rede/jogo/bitrate/resolução, comparar 60 s ou mais de RTT, jitter, pacotes perdidos, FPS, tempo de decode/render, drops, input-lag, CPU, memória e temperatura. Um motor é melhor quando dados mostram isso, não apenas porque usa WebRTC.

## Regras de segurança de integração
- Não mexer em `videoDecoder` (GPU/CPU) para codificar esse terceiro motor.
- Não adicionar botão `WebRTC Native` enquanto o motor não for operacional.
- Não migrar PS5/Windows/Linux para libwebrtc.
- Não copiar código AGPL-3.0 do XStreaming sem avaliar obrigações de licença.
- Manter a branch experimental e impedir merge na main até validação do usuário e CI completo, incluindo alinhamento 16 KB no APK.

## Testes
- [ ] Dependência resolvida em Gradle Android e compilação do APK;
- [ ] Verificar alinhamento 16 KB de **todas** bibliotecas nativas, inclusive libwebrtc;
- [ ] Confirmar que a versão Nimbox Alpha e suas opções de MediaCodec/FFmpeg continuam presentes;
- [ ] Confirmar PS5/Windows/Linux não alterados;
- [ ] Só depois realizar as fases 1–4.
