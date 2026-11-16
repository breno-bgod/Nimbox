# Nimbox 3D Library — Plano técnico (Fase 0)
Status: protótipo web + MVP visual 2.5D implementado na interface nativa; ainda SEM validação de compilação/dispositivo. Não substitui a interface clássica por padrão.
Referência visual: vídeo fornecido pelo usuário em 16/11/2026 (carrossel de caixas físicas em perspectiva, caixa selecionada central, detalhes de verso, disco e encarte).

## Objetivo
Criar uma biblioteca visual original com apresentação de jogos como mídias físicas, sem alterar login Microsoft, catálogo, serviços de streaming, controle de sessão, áudio ou vídeo. O projeto atual já foi validado funcionalmente pelo proprietário. Preservar `main` e oferecer rollback simples.

## Diagnóstico da base existente
- Interface: C++ em `src/ui/app_ui*.cpp`, com `AppUi`, `Canvas` RGBA, e sistema de imagens `ImageCache`.
- Resolução lógica do menu: 1920×1080 (`src/ui/app_ui_internal.h`).
- Dados de jogos: `GameTile` em `src/ui/app_ui.h`: `name`, `tileUrl`, `heroUrl`, `titleId`, `productId`, `playable`, entre outros.
- Entrada e navegação: `NavInput`, `AppUi::handleHome` em `src/ui/app_ui_input.cpp`. O fluxo de seleção precisa continuar encaminhando ao mesmo `openDetails`/ação existente.
- `Canvas` atual é 2D e oferece retângulos, linhas, círculos, gradientes e `drawImage`, mas não oferece diretamente projeção de textura em quadriláteros 3D. **A animação 3D nativa é trabalho novo.**
- Build Android existente em `.github/workflows/android.yml` produz APK de debug; a versão experimental exigirá identidade de pacote distinta antes de distribuir testes paralelos.

## Escopo fechado por etapas
### Fase 0 — Preparar
- [x] Criar branch experimental `feature/nimbox-3d-library`.
- [x] Registrar esta arquitetura e critérios de aceitação.
- [x] Criar uma demonstração isolada em HTML/CSS/JS para validar o conceito de navegação e estética (não é o APK).
- [ ] Revisar o protótipo com o usuário; definir design final de caixas e identidade.
### Fase 1 — MVP no aplicativo
- [x] Implementar primeira renderização nativa de carrossel 2.5D (caixas com lombada, cinco posições, foco central). Não é rotação 3D real.
- [x] Reusar `GameTile` e `ImageCache` com placeholder para capas indisponíveis.
- [x] Reutilizar navegação existente de `NavInput` e fluxo de abertura de detalhes sem modificar a lógica de execução dos jogos.
- [x] Alternância temporária com R3 na home Game Pass; modo clássico é padrão e continua intacto.
- [ ] Compilar em CI, medir FPS, tempo de desenho e memória em Android.
### Fase 2 — Caixa 3D
- [ ] Definir renderer: textura de capa/lombada/verso, transformações, sombra e oclusão.
- [ ] Garantir desenho correto e suave em plataformas de destino, evitando regressões na UI nativa.
- [ ] Rotação, verso, abertura e disco virtual de forma incremental.
- [ ] Não fabricar informações de embalagem: gerar elementos gráficos genéricos e usar apenas metadados realmente disponíveis.
### Fase 3 — Tema Nimbox
- [ ] Marca, paleta, tipografia, estados de foco, ícones, tela de detalhes e configurações.
- [ ] Rever permissões/licença de artes e manter créditos e GPL-3.0-or-later.
### Fase 4 — APK Alpha
- [x] Configurar APK debug isolado com `applicationId` `com.rafaelngp.psbox.nimboxalpha` e rótulo `Nimbox Alpha`; manter namespace Java/JNI original. **Instalação paralela ainda não foi validada em aparelho.**
- [ ] Aguardar CI do último commit; confirmar APK e checar artefato antes do primeiro teste no aparelho.
- [ ] Testar em aparelho Android: controles touch e físicos, navegação, catálogo, login, play, troca de sessão, áudio, retorno e fechamento.
- [ ] Corrigir regressões antes de considerar merge. Sem merge automático.

## Regras de não-regressão
1. Não editar `src/auth`, `src/xcloud`, `src/stream`, `src/media` nem os serviços de rede para implementar o layout.
2. Não mudar URLs do serviço, tokens, autenticação ou IDs de título.
3. Não sobrescrever a tela de detalhes nem os handlers de play sem teste específico.
4. Imagens carregadas sob demanda: não decodificar o catálogo inteiro simultaneamente.
5. Navegação utilizável com gamepad, toque, teclado e telas pequenas; considerar acessibilidade e redução de movimento.
6. Manter caminho inequívoco de volta à interface antiga.

## Critérios de aceitação do MVP
- Carrossel mostra ao menos 5 posições quando a tela permitir; selecionado central é claramente maior.
- Não perde foco ao alternar categorias; navegação esquerda/direita previsível.
- Capa ausente não trava.
- Ao aceitar, abre os detalhes do jogo correto.
- Nenhuma mudança no login ou no protocolo de streaming.
- Build Android passa e experiência é validada no aparelho do usuário.

## Riscos
- `Canvas` é 2D: CSS 3D do protótipo não pode ser copiado diretamente para C++.
- Arte do catálogo não contém necessariamente lombada, verso ou disco.
- Perspectiva e muitas imagens podem aumentar consumo de memória e latência em dispositivos modestos.
- O fork ainda possui identidade PSBox em códigos e documentação; alterações de marca devem ser isoladas de mudanças de backend.

## Entregas
- Este documento é a especificação inicial.
- `docs/prototypes/nimbox-3d-library.html` é um protótipo de interação sem ligação com xCloud; usa capas ilustrativas geradas por CSS, não imagens de jogos reais.
- Integração nativa e APK virão apenas após validação do desenho e das regras de navegação.

## Implementação inicial (continuação da Fase 1)
- `src/ui/app_ui_render.cpp`: `drawNimboxCarousel` usa renderização 2D do Canvas para criar caixas em perspectiva aproximada, com lombada, borda de foco, placeholder e metadados reais.
- `src/ui/app_ui.h`: flag temporária `nimboxCarousel_`, desligada por padrão; método de desenho declarado.
- `src/ui/app_ui_input.cpp`: R3 alterna exibição quando o usuário está na Home, aba Game Pass, fora da pesquisa.
- Sem mudanças em autenticação, catálogo, WebRTC, protocolo, decodificação ou sessão.
- PENDÊNCIA CRÍTICA: compilar e validar visual; não afirmar que APK foi produzido ou que este MVP está homologado.
- LIMITAÇÃO: as imagens fornecidas pelo catálogo podem ser quadradas e serão ajustadas a um retângulo vertical; a arte final exige tratamento específico.

## Continuação — preparação do Alpha e toque
- `android/app/build.gradle.kts`: apenas o build **debug** recebe sufixo `.nimboxalpha`; build release mantém ID original. A identificação Java/JNI não foi renomeada.
- `android/app/src/main/AndroidManifest.xml`: rótulo da aplicação vem de placeholder do build (`Nimbox Alpha` em debug, `PSBOX` em release).
- `src/ui/app_ui_input.cpp`: hit-testing do carrossel alinha as regiões clicáveis com as caixas; tocar numa lateral a seleciona, tocar na central abre detalhes. Hover não reutiliza a grade antiga.
- Ainda falta compilar e testar controles/touch reais. APK gerado antes dessas alterações NÃO contém este recurso.
