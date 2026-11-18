# Nimbox — Paridade entre Configurações nativas e Web Control (V2)

## Problema diagnosticado
A interface web aberta pelo QR tinha preferências de vídeo/imagem que não estavam acessíveis na lista principal do aplicativo C++. Em Android, o código ocultava inclusive a linha do decodificador. A página web era uma interface funcional, porém visualmente densa e com pouco contraste de hierarquia.

## Codec versus motor de decodificação
- **H.264 / AVC:** codec recebido no fluxo xCloud. `src/media/decoder.cpp` usa `AV_CODEC_ID_H264`.
- **Hardware (GPU):** preferência `videoDecoder=0`. O backend pode tentar um decodificador nativo específico da plataforma, depois aceleradores oferecidos pelo FFmpeg.
- **Software (CPU):** preferência `videoDecoder=1`, com decodificação FFmpeg.
- A seleção de hardware **não comprova** que a aceleração foi realmente ativada; o sistema pode usar fallback software. Conferir logs em execução.
- Não existe seletor H.264 versus H.265 neste painel e nenhum foi adicionado.

## Paridade de opções gerais
| Campo de configuração persistido | Web Control | UI nativa antes | UI nativa V2 |
| --- | --- | --- | --- |
| `videoDecoder` | GPU / CPU | Existia, oculto no Android | Visível no Android e demais plataformas |
| `resolution` | Sim | Sim | Mantido |
| `upscaler` | FSR / Anime4K | Só perfil por jogo | Global quando `caps.upscalers` |
| `deband` | Off / Baixa / Alta / Auto | Só perfil por jogo | Global quando `caps.deband` |
| `sharpness` | CAS 0..3 | Só perfil por jogo | Global quando `caps.sharpen` |
| `fpsLimit` | Sim | Sim | Mantido |
| `maxBitrateMbps` | Sim | Sim | Mantido |
| `highRefresh` | Sim | Sim, segundo capacidade | Mantido |
| `queueReadyAlert` | Sim | Sim | Mantido |
| `voiceRouting` | Auto / TV ou headset | Não exposto no menu geral | Global |
| `gameVolume` | Existe no sistema de preferências | Campo já implementado, mas omitido da ordem de UI | Restabelecido na ordem |
| `storeCountry` | Sim | Sim | Mantido |

Os novos IDs de UI são 31 upscaler, 32 deband, 33 sharpness e 34 voiceRouting. Os IDs 0..30 permanecem intactos. Os dados são os próprios campos existentes de `SettingsChoice`/`app::Settings` e `settings.json`, sem um segundo armazenamento.

## WebUI V2
Arquivos: `src/app/web_server_html.cpp` e `tests/test_main.cpp`.

- Marca N própria, contraste aprimorado, fundo atmosférico, painéis em camadas, foco visível, controle adequado a retrato e paisagem.
- Menus, busca, mapeamento, jogo atual, controle virtual, logs e ações mantêm seus IDs e endpoints.
- Explicação visível de H.264 e das opções CPU/GPU, sem prometer novos codecs.
- Sem bibliotecas externas, sem fontes remotas, respeitando `prefers-reduced-motion`.

## Validação obrigatória antes do merge
1. Verificar CI Linux, Windows e Android. Inspecionar screenshots nativas de `--selftest`.
2. **Android:** abrir Configurações sem QR, localizar decoder, upscaler, CAS e Deband conforme capacidades e salvar; retornar para confirmar persistência.
3. **Web pelo QR:** abrir no navegador do celular em retrato/paisagem; verificar navegação de seis abas, rolagem, seleção, edição e salvamento.
4. **Sincronização:** mudar uma preferência pela WebUI e voltar às configurações nativas; confirmar leitura do valor atualizado.
5. **Jogabilidade:** diagnosticar AVC durante streaming por logs `h264 decoder:` e `stream: video decoder`, comparar hardware versus software com mesmo jogo e rede; monitorar FPS/latência/quadros perdidos. Não atribuir travamentos ao codec sem medições.
6. **PS5:** capturar tela real, testar com DualSense, verificar safe area e legibilidade. Sem aprovação visual não publicar como atualização oficial.

Branch experimental, encadeada sobre PR #15. Nenhuma publicação de release ou merge sem homologação.
