# Android MediaCodec → Surface: validação do terceiro motor

PR: https://github.com/breno-bgod/Nimbox/pull/20
Origem AGPL-3.0: Geocld/XStreaming (detalhes em `third_party/XStreaming/NOTICE.md`)

## Escopo técnico

- Este backend é **opcional e exclusivo do Android** (opção
  `Surface Android`).
- O decodificador tradicional (hardware/CPU) e o renderer SDL/Vulkan permanecem
  nos seus arquivos originais; não trocar autenticação, negociação, áudio,
  entrada, rede ou carrossel.
- O novo backend recebe H.264 Annex B da sessão atual e libera os quadros
  decodificados diretamente na `SurfaceView`.
- O menu, a linha de estatísticas e o HUD touch são *espelhados* em uma view
  Android transparente; os eventos de toque continuam recebidos pelo SDL.
- A fila de entrada retorna Accepted/Busy/Error. Busy é limitado e provoca
  pedido de novo IDR se não for resolvido; Error encerra só a Surface e retoma
  o decoder tradicional.
- Ao trocar de resolução, a Surface é configurada novamente em um IDR.
- O engine usa MediaCodec.OnFrameRenderedListener para contar apresentações
  reais, separadas de compressão recebida e buffers liberados.

## Checks para o APK

1. O workflow `Android APK` deve compilar arm64-v8a e x86_64 no PR (não
   distribuir uma revisão cujo último commit falhou).
2. Verificar que configurações mostram **Hardware**, **Software** e
   **Surface Android** apenas no APK Android.
3. Iniciar o mesmo jogo em hardware/software e repetir em Surface. Verificar
   vídeo, áudio, controle físico, controles virtuais, menu e painel de
   estatísticas, incluindo abrir/fechar menu durante streaming.
4. Testar 720p e 1080p, mudança de resolução com jogo ativo e 1440p
   apenas se a oferta/capacidade do dispositivo permitir.
5. Alternar Surface → Hardware → Software → Surface, sem reiniciar xCloud.
6. Deixar o app em segundo plano e retornar; repetir após girar a tela ou
   recriar Activity quando aplicável. O fluxo pode recair no decoder original.
7. Simular/observar atraso do decoder; confirmar que `Busy` não vira
   sucesso falso e que o pedido de IDR recupera a imagem.
8. Se Surface travar/ficar preta, confirmar retorno ao decodificador antigo
   sem perder a sessão. Testar novo acionamento manual de Surface.
9. Verificar em logs `Surface actual output` os contadores
   `queued`, `released`, `on-screen`, `busy` e `errors`.
   `on-screen` é alimentado pelo callback Android, não por AU enfileirada.
10. Comparar fluidez, FPS apresentado, estabilidade e consumo de CPU/GPU
    em um mesmo jogo/dispositivo por vários minutos.

## Estado de homologação

A compilação do código é automatizada por GitHub Actions; teste de gameplay,
composição correta do overlay, mapeamento touch e latência percebida ainda
exigem execução real em Android. O PR permanece em rascunho até homologação.
