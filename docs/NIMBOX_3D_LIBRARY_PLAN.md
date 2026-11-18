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
- Integração nativa 2.5D já iniciou. APK para a fase 1B depende da confirmação de CI do commit correspondente e testes no Android.

## Implementação inicial (continuação da Fase 1)
- `src/ui/app_ui_render.cpp`: `drawNimboxCarousel` usa renderização 2D do Canvas para criar caixas em perspectiva aproximada, com lombada, borda de foco, placeholder e metadados reais.
- `src/ui/app_ui.h`: flag temporária `nimboxCarousel_`, desligada por padrão; método de desenho declarado.
- `src/ui/app_ui_input.cpp`: R3 alterna exibição quando o usuário está na Home, aba Game Pass, fora da pesquisa.
- Sem mudanças em autenticação, catálogo, WebRTC, protocolo, decodificação ou sessão.
- O MVP inicial compilou com sucesso no Android no CI do commit `73301db` (run 43912831124); Windows falhou separadamente. **A Fase 1B ainda não foi compilada/validada em aparelho.**
- LIMITAÇÃO: o catálogo oferece miniaturas quadradas; a Fase 1B utiliza a imagem inteira em uma inserção quase quadrada sobre a caixa vertical, com placa de título abaixo. Ainda não há imagens oficiais de contracapa/lombada nem imagens físicas de varejo.

## Continuação — preparação do Alpha e toque
- `android/app/build.gradle.kts`: apenas o build **debug** recebe sufixo `.nimboxalpha`; build release mantém ID original. A identificação Java/JNI não foi renomeada.
- `android/app/src/main/AndroidManifest.xml`: rótulo da aplicação vem de placeholder do build (`Nimbox Alpha` em debug, `PSBOX` em release).
- `src/ui/app_ui_input.cpp`: hit-testing do carrossel alinha as regiões clicáveis com as caixas; tocar numa lateral a seleciona, tocar na central abre detalhes. Hover não reutiliza a grade antiga.
- Ainda falta compilar e testar controles/touch reais. APK gerado antes dessas alterações NÃO contém este recurso.

## Fase 1B — refinamento visual solicitado após comparação com vídeo de referência (2026-11-16)
- [x] Marca visível do menu trocada para **NIMBOX** em `src/ui/app_ui_render.cpp`. `src/ui/brand.cpp` usa um novo monograma "N" e nome NIMBOX em splash/ícone gerado por código.
- [x] `src/ui/canvas.h/.cpp`: métodos `fillQuad` e `drawImageQuad` para rasterizar quadriláteros em perspectiva por software. Não representa motor gráfico 3D completo.
- [x] `src/ui/box_carousel.h`: geometria compartilhada com projeção inclinada, espessura física (lombada), seis caixas de cada lado, fileiras sobrepostas, hit-test e índices circulares.
- [x] `src/ui/app_ui_render.cpp`: usa capas reais sem cortes agressivos, faixa de plataforma baseada somente no metadado disponível, caixa central em foco, brilho discreto, sombras e placas de título. Não fabrica contracapas nem discos.
- [x] `src/ui/app_ui_input.cpp`: toque usa a mesma geometria das caixas, navegação esquerda/direita circular quando o carrossel está ativo.
- [ ] Validar CI do último commit, Android APK, regressões e desempenho em Android real.
- [ ] Comparar screenshots no aparelho com referência; ajustar profundidade, oclusão, tipo de arte, proporções, iluminação e fluidez.
- [ ] Atualizar ícones Android/TV estáticos e demais textos legados de marca em uma etapa específica de rebranding (o nome no topo já está corrigido).
- [ ] Fase 2 (rotação livre, verso, abertura e disco) **não iniciada**.

### Validação visual explícita
1. Mostrar capas menores parcialmente encobertas, semi inclinadas, em ambos os lados do item central mesmo no primeiro título (carrossel circular).
2. Caixa central maior com espessura e plataforma rotulada, sem confundir uma miniatura de catálogo com uma arte oficial de mídia física.
3. Tocar numa caixa lateral muda a seleção; tocar na central abre detalhes; R3 devolve a UI clássica.
4. Não executar merge do PR #1 antes da aceitação do usuário.

## Continuação — Caixa aberta, disco e animações (pedido com captura de tela, 2026-11-16)
Esta é uma implementação **experimental** da primeira interação da Fase 2, ainda dependente de homologação da Fase 1B no aparelho. Não representa a conclusão da Fase 2.

### Mudanças
- [x] `AppUi::drawNimboxOpenCase`: caixa virtual à direita do menu de detalhes, com tampa articulada à esquerda e bandeja do disco à direita, seguindo a composição da referência enviada.
- [x] Etiqueta do disco derivada da arte já disponível em `GameTile::tileUrl`, com anéis e furo centrais. **Não é a arte oficial de um disco físico.**
- [x] Animação automática de abertura ao entrar nos detalhes pelo carrossel Nimbox. Possível fechar/reabrir via R2 ou toque na caixa. A página clássica permanece inalterada.
- [x] `nimboxSlide_`: transição suave das caixas ao navegar com direcional ou selecionar por toque, usando o mesmo conjunto de coordenadas no render e no hit-testing.
- [x] Detalhes preservam título, metadados, Play, favoritos, wishlist, conquistas e ajustes; páginas de compra ainda exibem QR da loja em vez da nova caixa.
- [ ] Testar exatamente o commit desta etapa no Android; capturar vídeo do movimento e da abertura da caixa.
- [ ] Refinar efeitos de dobradiça, iluminação/reflexo do disco, fidelidade visual e desempenho, conforme avaliação em dispositivo real.

### Controle e fallback
- **Home Game Pass:** R3 ativa ou desativa carrossel experimental; seta esquerda/direita troca seleção animada.
- **Detalhes do jogo no modo Nimbox:** caixa abre automaticamente; R2 ou toque na área da caixa inverte a animação.
- **Modo clássico:** nenhum desenho de disco/caixa, nenhum novo gesto.
- Compilação do *commit anterior* `9bd6b16` passou em Android e Linux, mas Windows falhou em job específico; esta alteração tem seu próprio CI pendente e não deve ser confundida com o APK anterior.

### Critérios de aceite adicionais
1. A caixa não cobre o botão Jogar ou as informações essenciais do jogo.
2. Na tela de detalhes a tampa abre para a esquerda e o disco aparece à direita.
3. R2 ou toque abre/fecha; segurar botão não dispara jogo acidentalmente.
4. A animação de troca no carrossel mantém a seleção e o hit-test corretos.
5. O antigo visual continua reversível e o streaming não é modificado.


## Continuação — Polimento proposto e implementado em branch experimental (2026-11-16)

Objetivo: incorporar melhorias solicitadas após as imagens de referência sem antecipar que todas estejam prontas para publicação. A interface continua em Canvas 2,5D.

### Implementações desta iteração
- [x] **Contracapa interativa** nos detalhes por R1: imprime nome, editora, gênero, descrição e uma imagem da sessão/hero, quando disponível. Não inventa informações de varejo nem imagens oficiais de contracapas.
- [x] **Transições entre menus**: ao abrir detalhes, a caixa se desloca da posição central da biblioteca para a lateral e abre. Ao voltar, a caixa fecha e a tela retorna à biblioteca, sem iniciar o jogo por engano.
- [x] **Luz de ambiente do catálogo**: a tonalidade de fundo é derivada da cor dominante da capa, já calculada/cached pelo código existente, com limitação de intensidade para assegurar legibilidade.
- [x] **Coleções existentes visíveis**: título da linha e indicação de posição/quantidade; os controles cima/baixo continuam trocando as linhas da fonte original.
- [x] **Animações reduzidas**: R2 na biblioteca Nimbox ou botão touch superior alternam o modo de movimento reduzido na sessão. Animação da tampa e transição de entrada são omitidas nesse modo. Na página de detalhes R2 continua abrindo/fechando a caixa.
- [x] **Identidade Android Alpha**: ícone vetorial com monograma N e banner próprio na configuração debug; release mantém os recursos originais para evitar rebranding fora da branch experimental.

### Implementações anteriores preservadas
- [x] Carrossel de caixas em perspectiva, miniaturas reais, navegação circular.
- [x] Caixa que abre automaticamente com bandeja de disco virtual na tela de detalhes.
- [x] Branding NIMBOX na interface nativa experimental, sem remover créditos do projeto-base.

### Limitações e próximos critérios
- [ ] **Renderização GPU 3D real**: projeção com câmera, modelos e luzes físicas precisa ser projetada e avaliada à parte. O desenho atual é 2,5D por software e pode não atingir FPS ideais em aparelhos modestos.
- [ ] **Reflexos fotorealistas e sombras**: efeitos avançados dependem de análise de desempenho e talvez de backend gráfico novo.
- [ ] **Novas coleções por filtros**: ainda não reestruturar catálogo, favoritos ou dados de jogos; validar primeiro as categorias/linhas existentes.
- [ ] **Rebranding definitivo**: o nome de pacote release, ícones release e outros textos legados requerem decisão explícita de versão pública, fora deste PR experimental.
- [ ] **Teste do último commit no GitHub Actions**: Android build e artefato devem apontar para o SHA correto; Windows já possuía falha independente.
- [ ] **Teste no Samsung S23**: captura de tela/vídeo das transições; verificar R1, R2, toque, foco de capas, sem-capa, desempenho, login e iniciar jogo.
- [ ] Não mergear PR #1 antes de feedback visual e homologação funcional.

### Guia de teste Alpha
1. Instalar o novo APK debug com rótulo Nimbox Alpha ao lado do app existente.
2. Acessar Home Game Pass; ativar carrossel por R3.
3. Trocar coleções por cima/baixo e capas por esquerda/direita ou toque.
4. R2 na biblioteca altera movimento normal/reduzido; no modo reduzido repetir navegação para confirmar ausência de animação.
5. Abrir detalhes da caixa central; testar animação de abertura e fechamento por R2/toque.
6. R1 mostra contracapa; voltar com R1; voltar à biblioteca e verificar a animação de retorno.
7. Iniciar jogo, verificar o fluxo existente sem mudanças em streaming.



## Revisão da direção visual — Nimbox (novembro de 2026)

Esta seção SUBSTITUI as propostas antigas de contracapa e caixa articulada contidas anteriormente no plano. As quatro capturas enviadas pelo usuário motivaram a mudança.

### Implementado no código da branch
- [x] Rebranding visual: AppName e todos os textos PSBox nas traduções da UI dos seis idiomas foram trocados por Nimbox. Ícone, banner e rótulos Android são próprios da branch Nimbox. Créditos e licenças upstream permanecem.
- [x] Login: marca Nimbox, paleta ciano e painel escuro com transparência, sem alterar o login Microsoft por código.
- [x] Carrossel com pose física de estante: capas não selecionadas fortemente inclinadas, lombada visível e capa parcialmente exposta; ao navegar, a capa selecionada passa suavemente à vista frontal.
- [x] Remoção da linha escura horizontal na base; reflexos sutis do jogo central e vizinhos próximos.
- [x] Imagens: bilinear na projeção e maior solicitação de textura para capa central (512 px quando disponível); resolução de origem depende do xCloud e não é ampliada magicamente.
- [x] Tela de detalhes: contracapa, caixa aberta e controles extras removidos. O único objeto decorativo é um CD virtual giratório com arte do jogo. Play, informações e configurações do jogo permanecem intactos.
- [x] Modo de movimento reduzido na biblioteca (R2 ou toque) também interrompe a rotação contínua do CD.
- [x] Configurações: interface redesenhada em cartões de vidro fosco SIMULADO por gradientes e transparência, foco azul/ciano. A lógica e opções originais permanecem iguais; o Canvas não possui blur real.
- [x] Link de releases visível no desktop usa o repositório Nimbox, preservando créditos upstream.

### Testes pendentes — não publicar antes
- [ ] Confirmar CI Android do SHA HEAD e APK gerado por esta versão (não usar artefato de commits intermediários).
- [ ] Testar login, QR Microsoft, todas as configurações, QR Web Control, toque/gamepad e botão Play no Samsung S23.
- [ ] Avaliar FPS e uso de memória da rasterização bilinear/reflexos + rotação do CD.
- [ ] Comparar capturas reais com a referência e revisar pose, transição e leitura das lombadas.
- [ ] Revisar atualização automática/release público Nimbox separadamente; apontar o link de releases ao fork não publica novas versões automaticamente.
- [ ] Considerar backend GPU 3D apenas após medir limitações do atual renderizador 2,5D.

### Integridade do projeto
Nenhuma modificação de autenticação, protocolos Xbox ou núcleo de streaming foi realizada nesta revisão. Identificadores técnicos de compatibilidade e créditos/licenças do autor original permanecem, mas não devem aparecer como marca em menus ou login. O PR continua experimental e a main não recebe merge automático.
