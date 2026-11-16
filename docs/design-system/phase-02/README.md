# NIMBOX — FASE 02 · BIBLIOTECA DE COMPONENTES VISUAIS

**Edição:** v1.0.0 · **Estado:** proposta visual em revisão · **Base:** Design System Nimbox v1.0.0 e Preview Premium 3.0 / Preview 3.1 limpo · **Escopo:** exclusivamente design.

> Esta entrega **não altera o C++**, não substitui o Canvas, não usa WebView como solução obrigatória e não modifica login, catálogo, streaming, rede ou controles técnicos. O playground HTML é **um instrumento de revisão visual**, não prova de equivalência do renderizador nativo. Componentes que não têm ação real nunca devem ser apresentados como botões funcionais no aplicativo.

## 1. Objetivo da Fase 02

Definir aparência, componentes, estados, contratos de interação, geometrias e critérios de aceitação para as telas do Nimbox. Cada próxima mudança visual deve referenciar um identificador `NX/*` em vez de reinventar o componente na implementação. O Preview 3.0 aprovado é a referência de qualidade visual; o Preview 3.1 elimina botões sem função.

## 2. Componentes oficiais e prioridade

| ID | Prioridade | Aplicação | Estados mínimos |
|---|---|---|---|
| `NX/AppShell` | P0 | Barra, áreas seguras, zonas de navegação e rodapé contextual | base, foco, zona ativa |
| `NX/TopNav` | P0 | Início, Biblioteca, Game Pass, Detalhes, Configurações | padrão, foco, atual, desabilitado |
| `NX/FocusRing` | P0 | Indicação clara da seleção de controle/teclado | visível, deslocado, transição rápida |
| `NX/GameCase3D` | P0 | Capa física, espessura e lombada | normal, vizinho, foco, selecionado, arte ausente |
| `NX/CaseCarousel` | P0 | Vitrine cenográfica em perspectiva | parado, navegando, reduzir movimento, vazio |
| `NX/GameDetails` | P1 | Informações de jogo, arte e ação somente se disponível | padrão, consulta, disponível, indisponível |
| `NX/GameRail` | P1 | Trilho secundário de jogos | normal, foco, selecionado, carregando, vazio |
| `NX/Button` | P1 | Ações reais com hierarquia visual | padrão, hover, foco, pressionado, desabilitado |
| `NX/Chip` | P1 | Metadado, categoria ou filtro | padrão, selecionado, desabilitado |
| `NX/Panel` | P1 | Superfícies e diálogos | padrão, elevado, aberto |
| `NX/SettingRow` | P1 | Configuração existente, valor e estado | padrão, foco, alterado, desabilitado |
| `NX/Modal` | P1 | Confirmações e alertas | aberto, foco confinado, fechado |
| `NX/InputHint` | P1 | Instruções específicas do dispositivo | controle, teclado, mouse, toque |
| `NX/Toast` | P2 | Feedback não bloqueante | informação, sucesso, aviso, erro |

## 3. NX/AppShell e NX/TopNav

- **Estrutura:** identidade Nimbox à esquerda, ações de navegação centralizadas, contexto da conta à direita quando dados reais estiverem disponíveis. Conteúdo principal em painel contínuo, hints de ação na base apenas quando úteis.
- **Identidade:** utilizar marca N própria; não deixar PSBox como nome de produto visível. Créditos legais do projeto-base permanecem preservados em local apropriado.
- **Cores:** `surface.header` `#051B2C`; foco `accent.bright` `#97F5FF`; texto `text.primary` `#EFF9FF`; painel `surface.panel` `#09283D`.
- **Navegação:** item atual visualmente destacado; foco diferente de selecionado; ao retornar de Detalhes, restaurar a caixa selecionada, categoria e posição anterior.
- **Áreas seguras:** artboard de referência 1600×900; margens base 68 px no desktop/TV; respeitar overscan, bordas arredondadas e orientação do Android. Não reduzir a interface inteira a ponto de deixar textos críticos ilegíveis.
- **Não criar:** amigos, notificações, conta ou outros botões enquanto seu comportamento real não estiver conectado.

## 4. NX/GameCase3D — caixa física

Geometria de **referência visual aprovada** no HTML 3.0/3.1: caixa central de 286×389 px no artboard 1600×900, com lombada e face volumétrica (espessura 35 px), arte de capa sem distorção, brilho/escurecimento de bordas e sombra. O playground da Fase 02 usa uma escala de apresentação de 226×310 px para caber no editor, mas preserva o desenho de volume, lombada, capa e textura.

- **Normal:** face com textura e hierarquia do título; sem foco luminoso permanente.
- **Vizinho:** redução de escala, rotação em Y e profundidade. Arte ainda legível, evitando lombada como único elemento das caixas laterais.
- **Selecionado:** caixa principal em destaque, glow único e contextual. Apenas um item principal por vez.
- **Sem capa:** fallback estilizado com título real, não capa genérica apresentada como jogo existente.
- **Reflexo:** opacidade-base `0.44`, com desvanecimento vertical, blur e origem coerente com a face; reflexo subordinado à legibilidade.

**Dependência técnica futura:** o Canvas C++ atual desenha quads 2,5D; esses parâmetros não garantem volumetria 3D em runtime sem um renderizador compatível. A geometria da referência HTML deve ser preservada como critério de aceite de captura nativa.

## 5. NX/CaseCarousel — especificação de movimento e distância

Os valores abaixo provêm da referência HTML 3.0/3.1 ou são tokens de apresentação. São **parâmetros de design**, não alterações nativas nesta fase:

| Parâmetro | Proposta de referência |
|---|---|
| Perspectiva | 1250 px |
| Caixa central (1600×900) | 286 × 389 px |
| Deslocamento lateral por nível | 280, 450, 580, 690 px |
| Rotação lateral por nível | 48°, 57°, 64°, 67° |
| Escalas dos vizinhos | 90%, 80%, 72%, 65% |
| Reflexo-base | 44% |
| Transição | 920 ms |
| Curva | `cubic-bezier(0.21, 0.76, 0.16, 1)` |

- **Controle:** esquerda/direita movimenta a seleção; confirmar abre os detalhes; Voltar devolve ao jogo original.
- **Mouse/toque:** clique em uma caixa vizinha a seleciona, clique na ativa mostra os detalhes, swipe alterna uma posição.
- **Interrupção:** vários comandos rápidos podem substituir a transição atual, sem travar input.
- **Reduzir movimento:** remove a rotação/parallax animada não essencial, mantém a seleção imediata e o foco legível.
- **Responsividade:** na tela estreita, diminuir número/área de vizinhos visíveis, sem reduzir o card principal a uma miniatura ilegível.

## 6. Botões, chips, trilhos e configurações

### `NX/Button`

- **Primary:** reservada à ação principal realmente disponível (ex.: Jogar somente se jogo puder iniciar).
- **Secondary:** navegação/ação complementar; foco claro.
- **Quiet:** voltar, cancelar ou detalhes secundários.
- **Danger:** confirmação destrutiva, nunca presumida.
- **Disabled:** baixa ênfase, não dispara evento. `Loading` exibe o estado real e evita duplo acionamento.

### `NX/Chip`

Chip de informação não age como botão. Chip de filtro só vira interativo quando filtra o conteúdo de verdade; selecionado difere de hover e foco.

### `NX/GameRail`

Miniaturas proporcionais, título visível e foco consistente. Mostra metadados reais do jogo; não inventar horas jogadas, conexão de controle ou disponibilidade.

### `NX/SettingRow`

Alinha rótulo, descrição e valor/controle, com espaçamento suficiente em telas grandes; feedback de estado imediato. O painel de demonstração altera apenas o preview, não configurações reais.

## 7. Estados e contratos por entrada

| Ação | Controle | Teclado | Toque/mouse | Resultado visual |
|---|---|---|---|---|
| Selecionar vizinho | Direcional esquerda/direita | ← / → | toque no vizinho / swipe | capa central muda, título acompanha |
| Abrir detalhes | Confirmar | Enter | toque/click na capa ativa | detalhes da mesma capa |
| Voltar | Voltar do console | Escape | botão contextual | restaura seleção e trilho |
| Mudar aba | ombros ou foco por aba, conforme layout real | Tab + Enter | clique/tap na aba | aba atual visível |
| Fechar modal | Cancelar | Escape | botão Voltar | retorna foco ao disparador |
| Reduzir movimento | configurações acessíveis | configurações acessíveis | controle UI | sem movimento não essencial |

**Contratos adicionais:** um único foco por vez; estados diferenciáveis sem depender apenas de cor; sem navegação invisível; sem action hints que não funcionem; título e filtro persistem quando o usuário volta dos detalhes. Componentes ainda sem integração aparecem somente no playground e não no APK.

## 8. Mapeamento posterior para C++ (sem edições nesta fase)

| Componente | Pontos existentes de integração | Estado |
|---|---|---|
| `AppShell` / `TopNav` | `src/ui/app_ui_render.cpp`, `src/ui/app_ui_input.cpp` | especificação |
| `GameCase3D`, `CaseCarousel` | `src/ui/box_carousel.h`, `src/ui/app_ui_render.cpp`, `src/ui/app_ui_input.cpp` | especificação |
| `FocusRing` | `src/ui/app_ui_render.cpp` | especificação |
| `GameDetails` | `src/ui/app_ui_render.cpp`, `src/ui/app_ui_input.cpp` | especificação |
| `Panel`, `SettingRow` | `src/ui/app_ui_settings.cpp` | especificação |
| Desenho base | `src/ui/canvas.cpp` | auditoria de capacidade futura |

Não converter HTML em C++ por copiar e colar; nem aceitar recriação 2,5D como equivalente visual sem verificação. Futura implementação deve usar **parâmetros compartilhados e testes de fidelidade da imagem**.

## 9. Checklist de aprovação da Fase 02

- [ ] Home 1600×900: composição, identidade, caixas e reflexos aprovados visualmente.
- [ ] Home Android landscape: leitura das capas laterais e zona segura aprovadas.
- [ ] Foco por controle/teclado claramente visível; Back restaura o título.
- [ ] Estados selecionado, desabilitado, foco e pressed distinguíveis.
- [ ] Reduzir movimento respeitado e navegação sem bloqueio.
- [ ] Componentes documentados não se apresentam como funcionalidades conectadas.
- [ ] Tokens e contratos validados para próxima etapa.
- [ ] Quando integrado ao C++, comparar capturas reais e comportamento com o Preview 3.0; **não presumir paridade**.

## 10. Artefatos

- `Nimbox_Components_Fase_02_Interativo.html`: playground interativo e inspector visual com exportação JSON de parâmetros.
- `Nimbox_Components_Fase_02_Tokens.json`: catálogo estruturado de componentes, estados, parâmetros e mapeamento C++.
- `Nimbox_Components_Fase_02.md`: este documento de referência.

**Governança:** Fase 02 exclusivamente em branch de design, sem alterações na `main` ou na Fase 01 aprovada parcialmente. Próximo marco somente após revisão: Motion & Sonic Direction (Fase 03), também em design.