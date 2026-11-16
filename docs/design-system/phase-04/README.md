# NIMBOX — FASE 04 · AUDITORIA VISUAL TELA A TELA

**Versão:** 1.0.0 · **Estado:** Plano de design em revisão · **Escopo:** somente design, sem alterações ao aplicativo funcional.

## 1. Objetivo e fontes

Esta fase transforma o Design System em **critérios explícitos por tela**, sem transformar mockups em código de produção. Os arquivos de referência são os screenshots e os contratos das Fases 01 a 03. **HTML é ferramenta de observação, não renderizador C++ comprovado.**

**Fontes lidas:** `docs/design-system/v1.0.0/README.md`, `tokens.json`, `phase-02/README.md`, `components.json`, `phase-03/README.md`, `motion-sound.json`, e as capturas de referência do Preview 3.0/3.1 presentes no projeto.

## 2. Critérios transversais

| Aspecto | Contrato de design |
|---|---|
| Identidade | Nimbox em todas as telas de produto; preservar créditos de software em documentação/licenças |
| Cores | Base `#061A2B`, cabeçalho `#051B2C`, texto `#EFF9FF`, acento `#79DFFF`, foco `#97F5FF` |
| Composição | Base de projeto 1600×900; manter safe areas; adaptar sem encolhimento cego para Android/TV |
| Capa central | `NX/GameCase3D` 286×389 px (referência), espessura visual, reflexão 44%, perspectiva 1250 px |
| Hierarquia | O jogo é o protagonista. Letras, dicas, chips e ações nunca devem cobrir a arte |
| Movimento | `focus` 120 ms, `press` 95 ms, `page` 300 ms, `modal` 220 ms, `carousel` 920 ms, `disc` 7,5 s/volta |
| Áudio | seis eventos originais opcionais, desativados por padrão na prévia; cooldown foco 125 ms |
| Input | foco permanente, Back preserva origem, navegação interrompe animações, touch/teclado/controle equivalentes |
| Integridade | Sem botão demonstrativo disfarçado de função; sem números/horas de jogos inventados no produto real |

### Matriz de referência e aprovação

| Tela | Referência visual | Estado | Prioridade |
|---|---|---|---|
| Home | Preview 3.1 Home (referência principal) | reference-approved | P0 |
| Biblioteca | Preview 3.0 Biblioteca (com elementos demonstrativos a revisar) | concept-provisional | P0 |
| Detalhes | Preview 3.0 Detalhes: hierarquia e disco apenas; botão Jogar do conceito não é válido como função | concept-provisional | P1 |
| Login | Preview de Login anterior ao 3.0 como direção visual, não fluxo testado | concept-provisional | P0 |
| Configurações | Preview 3.1 Configurações (apenas ajustes do protótipo) | concept-provisional | P0 |
| Streaming | Diretrizes funcionais e Design System 1.0; não existe mock de streaming aprovado | no-approved-visual-reference | P0 |

## 3. Pacotes de revisão por tela

### 04.1 — Home (P0)

**Direção:** Coleção física no estúdio infinito

**Base:** Preview 3.1 Home (referência principal). **Estado:** Home é a âncora visual aprovada; revisões de microdetalhes ainda possíveis.

**Composição:** Aurora luminoso, cabeçalho navy, 1 caixa hero com quatro vizinhas por lado, reflexos contínuos no piso e títulos subordinados à arte. Referência central 286×389 px a 1600×900, perspectiva 1250 px.

**Navegação:** D-pad esquerda/direita muda seleção; confirmar abre detalhes; cima/baixo troca zona; voltar preserva contexto. Toque e mouse sem botões fantasmas.

**Movimento:** Carrossel 920 ms cubic-bezier(.21,.76,.16,1); interrupção imediata em inputs consecutivos; reduzir movimento sem rotação/parallax.

**Som:** ui.focus com cooldown 125 ms; ui.open ao abrir detalhes somente se o áudio estiver ativado.

**Componentes:** NX/AppShell, NX/TopNav, NX/CaseCarousel, NX/GameCase3D, NX/GameRail, NX/FocusRing, NX/InputHint.

**Riscos conhecidos:**
- Canvas 2,5D atual não preserva volume e iluminação da referência
- Lombadas dominam quando o espaçamento é insuficiente
- Reflexo e brilho podem esconder texto e diminuir fluidez

**Aceite visual por tela:**
- [ ] Hero permanece dominante e centrado, sem recortes
- [ ] Caixas laterais têm espessura, perspectiva, espaçamento e ângulo coerentes
- [ ] Piso e reflexos são contínuos e não competem com capas
- [ ] Hierarquia entre capa, nome do jogo e trilho inferior fica legível
- [ ] O jogo real selecionado acompanha ações e volta sem perder posição

### 04.2 — Biblioteca (P0)

**Direção:** Encontrar o jogo antes de admirar a capa

**Base:** Preview 3.0 Biblioteca (com elementos demonstrativos a revisar). **Estado:** Composição de grid provisória; dados fictícios e filtros dependem de funções existentes.

**Composição:** Painel navy com grid de capas alinhadas, busca e filtros em linha superior, contraste e espaçamentos regulares. Sem competir com a Home cinematográfica.

**Navegação:** Foco passa de busca/filtros à grade; filtros só aparecem ativos quando de fato conectados; retorno restaura busca, ordem e posição.

**Movimento:** Transições de página 300 ms; foco 120 ms; sem parallax na grade.

**Som:** ui.focus / ui.select discretos sob as regras do sistema sonoro.

**Componentes:** NX/AppShell, NX/TopNav, NX/FocusRing, NX/Button, NX/Chip, NX/InputHint, NX/Toast.

**Riscos conhecidos:**
- O preview 3.0 exibe jogos e tempos de uso ilustrativos
- Grid pode cortar títulos em telas menores
- Destaques podem ser indistinguíveis do item selecionado

**Aceite visual por tela:**
- [ ] Busca, filtros e ordenação correspondem às funções existentes
- [ ] Grade reflow sem perda de proporção das capas
- [ ] Capa sem arte possui placeholder próprio, não espaço vazio
- [ ] Foco identificável sem depender apenas da cor
- [ ] Voltar restaura consulta, filtro e último jogo selecionado

### 04.3 — Detalhes (P1)

**Direção:** Uma capa, uma história, uma ação real

**Base:** Preview 3.0 Detalhes: hierarquia e disco apenas; botão Jogar do conceito não é válido como função. **Estado:** Revisar o detalhe sem ações simuladas; a captura antiga tem botão Jogar demonstrativo.

**Composição:** Composição 2 colunas: título/metadados/ações à esquerda, disco decorativo à direita. Área de texto de fácil leitura, sem efeito cobrindo informações.

**Navegação:** Ação Jogar visível somente quando a reprodução está disponível e realmente conectada; voltar retorna ao mesmo item da origem.

**Movimento:** Página 300 ms; disco 7.500 ms/volta apenas decorativo, pausado em reduzir movimento.

**Som:** ui.open ao entrar e ui.back ao retornar, quando habilitados.

**Componentes:** NX/GameDetails, NX/Button, NX/Panel, NX/FocusRing, NX/InputHint.

**Riscos conhecidos:**
- Preview antigo contém botão Jogar que não reproduz nada
- Imagem 3.1 de detalhes capturada com sobreposição escura não é baseline de aprovação
- Descrição e título podem colidir com arte ao redimensionar

**Aceite visual por tela:**
- [ ] Jogar não é exibido se a ação real não existe
- [ ] Título, dados e descrição legíveis nas larguras previstas
- [ ] Disco nunca parece um botão se for apenas decorativo
- [ ] A navegação de volta preserva o ponto de origem
- [ ] Casos indisponíveis têm linguagem visual e texto específico

### 04.4 — Login (P0)

**Direção:** Autenticidade, clareza e confiança

**Base:** Preview de Login anterior ao 3.0 como direção visual, não fluxo testado. **Estado:** Composição conceitual; exibir sempre status e código verdadeiros vindos do núcleo.

**Composição:** Duas colunas com marca Nimbox e instrução no lado principal; código de dispositivo com alto contraste; painel de orientação discreto. Evitar informações decorativas excessivas.

**Navegação:** Código real com expiração/renovação, status real, copiar/abrir navegador somente se suportados; transição para carregamento sem marca PSBox.

**Movimento:** Entrada suave sem atrasar leitura do código; progresso conforme eventos reais.

**Som:** ui.ready somente ao concluir autenticação; erro apenas para erro real.

**Componentes:** NX/AppShell, NX/Panel, NX/Button, NX/Progress, NX/FocusRing.

**Riscos conhecidos:**
- O screenshot do preview usa CÓDIGO DE EXEMPLO: não pode ser reutilizado como login funcional
- Identidade da base aparecia nos testes históricos
- Código de 8 caracteres precisa de contraste e espaçamento robustos

**Aceite visual por tela:**
- [ ] Nimbox aparece em todas as telas de login e carregamento
- [ ] Código fictício nunca aparece no aplicativo real
- [ ] Instruções, URL e status reais são legíveis a distância
- [ ] Erro, expiração e nova tentativa têm textos próprios
- [ ] Autenticação não é alterada por este planejamento

### 04.5 — Configurações (P0)

**Direção:** Clareza operacional com acabamento premium

**Base:** Preview 3.1 Configurações (apenas ajustes do protótipo). **Estado:** Reutilizar linguagem visual, não substituir lista de ajustes nativos por opções demonstrativas.

**Composição:** Navegação por categorias e linhas escaneáveis: nomes, valores à direita e descrições; painéis Frosted Navy com separação coerente. Menos efeitos visuais que a Home.

**Navegação:** Toda linha respeita comportamento existente; ações de suporte apontam ao projeto Nimbox; modais prendem foco; Back devolve a linha originadora.

**Movimento:** Foco 120 ms e modal 220 ms; nada de transições pesadas em mudanças frequentes.

**Som:** ui.select em alterações confirmadas; ui.error apenas quando há falha.

**Componentes:** NX/AppShell, NX/TopNav, NX/SettingRow, NX/Panel, NX/Modal, NX/FocusRing, NX/InputHint.

**Riscos conhecidos:**
- O screenshot 3.1 só mostra ajustes do preview
- Lista real possui mais categorias e opções de controles/rede
- QR e atualização não devem referenciar projeto original

**Aceite visual por tela:**
- [ ] Todas as configurações existentes continuam disponíveis
- [ ] Valores e foco alinhados nas linhas e grupos
- [ ] Suporte e atualizações identificados como Nimbox
- [ ] Ações destrutivas têm confirmação clara
- [ ] Nenhum controle de demo é apresentado como ajuste real

### 04.6 — Streaming (P0)

**Direção:** O jogo ocupa a tela; a UI só aparece quando necessária

**Base:** Diretrizes funcionais e Design System 1.0; não existe mock de streaming aprovado. **Estado:** Primeira composição de overlay ainda precisa de aprovação visual separada.

**Composição:** Vídeo com prioridade absoluta e overlays pequenos, temporários e legíveis; progresso de conexão e fila com informação real; estado de erro distinto.

**Navegação:** O overlay respeita o contexto de controles da sessão, não rouba foco nem envia inputs duplicados para a partida. Nenhuma mudança na pipeline.

**Movimento:** Entrada 180–220 ms, saída discreta; feedback de conexão sem efeitos intrusivos.

**Som:** Sinal de conclusão/erro somente em eventos de sistema reais, com mute/ganho independente.

**Componentes:** NX/AppShell, NX/Progress, NX/Toast, NX/InputHint, NX/Modal.

**Riscos conhecidos:**
- Overlay pode competir com gameplay
- Redirecionamento involuntário de controles ao menu
- Sem baseline visual aprovado

**Aceite visual por tela:**
- [ ] Gameplay fica visível sem interferência permanente
- [ ] Overlay abre e fecha de maneira previsível
- [ ] Estados conectado, fila, falha e reconexão distinguíveis
- [ ] Controle volta ao jogo sem ações duplicadas
- [ ] Visual deve ser aprovado antes de implementação

## 4. Ordem do trabalho — somente design

| Bloco | Entrega de aprovação | Dependência |
|---|---|---|
| A · Home | Paridade composicional do carrossel, materiais, foco, hierarquia | NX/GameCase3D, NX/CaseCarousel |
| B · Biblioteca | Grid, busca/filtro, estados e responsividade | NX/FocusRing, NX/Chip |
| C · Detalhes | Hierarquia sem botão falso, estados de ações reais | NX/GameDetails, NX/Button |
| D · Login | Identidade, instrução, código e estados reais | NX/Progress, NX/Panel |
| E · Configurações | Categorias, ações existentes, modais e feedback | NX/SettingRow, NX/Modal |
| F · Streaming | Propor overlay mínimo, estados sem interferir no jogo | NX/Progress, NX/InputHint |

Cada bloco é uma **unidade de revisão de design**, não uma autorização automática para modificar C++.

## 5. Protocolo de comparação

1. **Travar cenário:** mesma viewport, mesmo jogo/capa, mesmo tema e tela, mesmos dados e estado de foco.
2. Capturar a referência visual e a execução **nativa** em PNG sem reescalar uma delas isoladamente.
3. Verificar composição: posição/escala das caixas, visibilidade de lombadas, proporção do hero, ângulo, reflexo, espaço livre e texto.
4. Executar navegação por controle, teclado e toque; registrar vídeo e prova de retorno ao mesmo item.
5. Verificar legibilidade, contraste, estados vazio/carregando/erro, modo reduzir movimento e áudio desligado.
6. Atribuir ao relatório um estado: **proposta**, **revisão pendente**, **aprovado em design**, **reprovado nativo**, ou **validado nativo**. A aprovação do preview não aprova automaticamente o C++.

## 6. Cobertura mínima

- 1600×900 (baseline das capturas); 1920×1080; 1280×720.
- Android paisagem em ao menos uma tela 20:9; desktop/Steam Deck em 16:10.
- TVs/consoles: capturar área segura e navegabilidade quando houver binário e dispositivo disponível.
- Os requisitos de PlayStation permanecem critérios-alvo; **não declarar compatibilidade sem execução testada**.

## 7. Não fazer nesta fase

- Não alterar login, tokens, rede, WebRTC, código de streaming, input técnico, servidor ou catálogo.
- Não introduzir WebView, novos motores gráficos ou reescrever o C++ como parte deste PR de design.
- Não chamar de UI aprovada uma captura antiga de APK com carrossel 2,5D.
- Não usar botão Jogar demonstrativo, login fictício, amigos falsos ou configurações de teste como se fossem funcionais.
- Não publicar APK, fundir com `main`, nem aprovar paridade sem captura lado a lado.

## 8. Entregáveis e decisão final

- `README.md`: este plano e os critérios de aceite.
- `screens.json`: contratos de tela, estados, riscos e checklists em estrutura reutilizável.
- `review.html`: painel offline de auditoria visual e carregamento **local** de captura nativa, sem envio de arquivos.

**Estado:** planejamento documentado. Aprovação visual e implementação nativa ainda pendentes.