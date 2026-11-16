# NIMBOX — DESIGN SYSTEM OFICIAL

**Versão:** 1.0.0 · **Fase:** 01 — Fundação visual · **Estado:** Proposta documentada para aprovação visual · **Data:** 16/11/2026

> **Escopo desta fase:** identidade visual, tokens, componentes, materiais, iluminação, tipografia, movimento, efeitos sonoros e navegação. **Nenhuma alteração** em login, autenticação, catálogo, streaming, servidor, controles técnicos ou protocolos. O design system não significa que o layout já esteja integrado ao aplicativo C++.

## 1. Norte criativo

**Conceito:** *Nimbox — seu universo de jogos, apresentado como uma coleção premium.* Interface de console cinematográfica, tátil, precisa, elegante e própria. A experiência deve comunicar uma biblioteca física em um estúdio infinito: jogos com presença e profundidade, o restante da interface com baixo ruído visual.

**Referência visual aprovada:** `Nimbox Preview Premium 3.0` (conservado como arquivo de referência; o HTML 3.1 é uma limpeza de controles fictícios). A aparência 3D do carrossel e o cenário contínuo são requisitos de fidelidade, não sugestões opcionais.

### Princípios

1. **O jogo é o protagonista.** A arte do jogo deve dominar; a interface nunca compete com ela.
2. **Profundidade com propósito.** Volume, reflexos, sombras e luz devem ajudar a percepção de seleção e distância.
3. **Leitura a distância.** Design utilizável em TV, monitor, celular e controle de jogo, sem depender de texto minúsculo.
4. **Navegação previsível.** O foco é sempre visível; uma ação tem sempre o mesmo significado.
5. **Movimento a serviço da orientação.** Transições informam de onde se veio e para onde se vai; sem animações gratuitas.
6. **Nada de funções falsas.** Um botão sem ação real não deve ser exibido como funcional.
7. **Identidade própria.** Nimbox não copia símbolos, nomes ou marcas de consoles. Marcas Xbox podem aparecer em conteúdos/capas licenciados e identificações necessárias, nunca como marca Nimbox.

## 2. Estado da referência e das decisões

- **Preservar do Preview 3.0:** direção artística, perspectiva das caixas, cenário Aurora claro com profundidade, piso reflexivo, topo azul-marinho, cromia ciano, hierarquia de arte/capa/título e sensação de fluidez.
- **Formalizar nesta fase, sujeito à validação:** grid, escala de espaçamento, estados, tipografia responsiva, parâmetros de motion, limites de luz, som de interface, especificações de acessibilidade.
- **Fora de escopo:** alterar implementações C++ existentes, motor gráfico, autenticação, integração xCloud, jogos reais, lógica de streaming, publicação de APK e alterações na `main`.

## 3. Sistema cromático

Valores-base retirados da identidade existente do HTML 3.0 quando indicado; o restante é padronização proposta.

| Token | Hex | Papel |
|---|---|---|
| `surface.void` | `#040B16` | Fundo exterior / blackout |
| `surface.base` | `#061A2B` | Base geral da interface |
| `surface.header` | `#051B2C` | Barra de navegação |
| `surface.panel` | `#09283D` | Painéis e menus |
| `surface.raised` | `#123B55` | Componentes elevados |
| `text.primary` | `#EFF9FF` | Títulos, ações e textos fortes |
| `text.secondary` | `#BDCEDA` | Metadados, descrições |
| `text.tertiary` | `#8EACBE` | Dicas secundárias |
| `accent.primary` | `#79DFFF` | Destaques e interação |
| `accent.bright` | `#97F5FF` | Anel de foco visível |
| `accent.secondary` | `#51CBDF` | Progresso e detalhes |
| `line.default` | `#5E9CB5` | Divisórias e bordas, com opacidade definida no componente |
| `state.success` | `#71E6A3` | Confirmação, conectado |
| `state.warning` | `#FFC978` | Atenção não destrutiva |
| `state.danger` | `#FF7C8D` | Erro e ações destrutivas |

**Regra de uso:** superfícies e estrutura em azul-marinho; ciano para foco e interação; o verde não é a cor primária da marca. Evitar grandes áreas de ciano e overlays opacos sobre as artes dos jogos. Estados de erro e sucesso não dependem apenas da cor.

### Temas de ambiente

- **Aurora (padrão aprovado):** céu `#8CBED5`, horizonte `#A7D5E8`, meio `#4F82A7`, base `#1C4764`, brilho `#70D8FF`.
- **Infinity Light:** opção clara de estúdio, com contraste ajustado para títulos.
- **Nimbox Blue:** opção azul com acento intensificado.
- **Emerald:** variante de ambiente, não da identidade principal.
- **Cinema Dark:** variante mais escura para salas e TV.

*O tema altera ambiente e acento secundário; não muda o significado de estados, ícones e ações.*

## 4. Materiais e iluminação

| Material | Aplicação | Regra |
|---|---|---|
| **Infinity Studio** | Home e vitrines | Gradiente contínuo céu → meio → piso; sem horizonte rígido |
| **Frosted Navy** | Cabeçalho, menus, modais | Fundo azul-escuro translúcido, contorno fino, desfoque somente quando suportado |
| **Game Case** | Carrossel principal | Frente texturizada, lombada, espessura, contornos, arte não distorcida |
| **Polished Floor** | Reflexo das caixas | Reflexo espelhado com blur e fade vertical; não competir com títulos |
| **Soft Halo** | Foco no item | Realce suave ao redor do item ativo, nunca permanente em todas as caixas |
| **Elevation Shadow** | Camadas sobrepostas | Sombras com origem coerente, sem glow excessivo |

**Luz:** preenchimento difuso central, borda de luz fria no item selecionado, lado oposto com sombra leve. Não ultrapassar saturação que prejudique a leitura da capa. Reflexo-base da referência: **44%** como parâmetro de partida, ajustável por tema. A iluminação pode ser simplificada em plataformas com menos recursos, sem alterar geometria, escala e composição da interface.

## 5. Tipografia

**Família de referência:** Inter, com fallback para sistema sans-serif. No aplicativo, usar a família somente se ela estiver devidamente incluída/licenciada nos ativos; não depender de download de fonte em execução.

| Nível | Desktop/TV (base 1600×900) | Peso | Uso |
|---|---:|---:|---|
| Display | 44 px | 800 | Título de destaque excepcional |
| Heading 1 | 34 px | 800 | Título de página / jogo em foco |
| Heading 2 | 24 px | 750 | Seção de conteúdo |
| Heading 3 | 19 px | 700 | Cartão ou painel |
| Body | 16 px | 450 | Descrição principal |
| Meta | 14 px | 500 | Categoria, fabricante, legenda |
| Caption | 12 px | 650 | Microlegenda não essencial |

**Tipografia responsiva:** remapear por densidade e distância de leitura, não apenas reduzir proporcionalmente a 1600×900. Texto funcional deve ser confortável em TV e celular; usar truncamento com opção de ver o texto completo.

## 6. Layout e componentes

**Artboard de referência:** 1600 × 900 (16:9). Sistema-base de espaçamento: 4, 8, 12, 16, 24, 32, 48 e 64. Margens horizontais de referência: **64–68 px** no desktop; adaptar para safe areas, ultrawide, TV overscan e Android landscape.

**Componentes oficiais a catalogar (DS-01):**

- `NX/AppShell`: barra principal, navegação, área de conteúdo, barra de dicas.
- `NX/TopNav`: abas Início, Biblioteca, Game Pass, Gêneros, Configurações; rótulos e indicação de estado atual.
- `NX/FocusRing`: anel de foco ciano, deslocado da borda do componente e bem contrastado.
- `NX/GameCase3D`: capa, espessura, lombada, estados normal/foco/ativo/inativo; preservação de proporção.
- `NX/CaseCarousel`: centro protagonista, laterais com escala e perspectiva, navegação cíclica configurável.
- `NX/GameRail`: trilho de jogos menores abaixo da vitrine principal.
- `NX/GameDetails`: arte, nome, metadados, ações reais, disco em rotação decorativa (quando aplicado).
- `NX/Button`: principal, secundário, discreto, crítico e indisponível.
- `NX/Chip`: categoria, status e filtro; não prometer funcionalidade inexistente.
- `NX/Panel`: cartão frosted navy, modal, confirmação e feedback de operação.
- `NX/SettingRow`: nome, descrição, valor, estado de seleção, alternância/controle real.
- `NX/Progress`: carregamento, conclusão, aviso e erro com texto acessível.
- `NX/InputHint`: ações por controle/teclado/touch conforme contexto.
- `NX/Toast`: mensagens curtas, não bloqueantes, sem substituir erros críticos.

**Estado obrigatório de cada controle:** padrão, hover (quando houver cursor), foco por controle/teclado, pressionado, selecionado, desabilitado e carregando (quando aplicável). Apenas controles realmente funcionais entram na interface final.

## 7. Animações e movimento

**Valores propostos para revisão visual; não significam que o aplicativo atual já os implemente.**

| Token | Duração | Curva/uso |
|---|---:|---|
| `motion.focus` | 120 ms | ease-out; realce e contorno |
| `motion.press` | 95 ms | ease-out; escala/elevação discretas |
| `motion.page` | 300 ms | ease-in-out; passagem entre telas |
| `motion.modal` | 220 ms | ease-out; entrada de painel |
| `motion.carousel` | 920 ms | cubic-bezier(0.21, 0.76, 0.16, 1); valor atual da referência |
| `motion.feedback` | 180 ms | ease-out; feedback local |
| `motion.disc` | 7.5 s/volta | linear contínuo, somente na tela de detalhes |

**Regras:** sem animações que bloqueiem comando; foco deve responder imediatamente; sequência de inputs pode interromper/transicionar o movimento anterior; nada de flashes agressivos. **Reduzir movimento** deve eliminar rotação/parallax não essenciais e trocar transições longas por troca imediata ou fade breve.

## 8. Sistema sonoro — Nimbox Sonic Identity (proposto)

**Som não é música de fundo.** Identidade discreta, eletrônica e limpa: assinatura curta, sem usar melodias ou efeitos de Xbox, PlayStation ou outros produtos.

| Evento | Direção sonora | Duração-alvo |
|---|---|---:|
| `ui.focus` | clique suave, levemente aerado | 35–70 ms |
| `ui.select` | confirmação luminosa, dois harmônicos curtos | 90–160 ms |
| `ui.back` | tom descendente discreto | 65–125 ms |
| `ui.open` | transição/abertura sutil | 130–210 ms |
| `ui.error` | aviso amortecido, sem estridência | 140–230 ms |
| `ui.ready` | confirmação de estado concluído | 140–220 ms |

**Regras:** sem reprodução contínua na navegação; limite de repetição em scroll rápido; volume de interface independente do áudio do jogo; respeitar silenciar, acessibilidade, áudio do SO e reduzir estímulos; sons somente após interação explícita e com opção de desligar. Os arquivos de SFX finais ainda **não foram produzidos ou integrados**.

## 9. Navegação e UX

1. **Mesmo modelo mental em todas as plataformas:** esquerda/direita percorre trilhos, cima/baixo muda zonas, confirmar abre, voltar retorna um nível. Touch, mouse e teclado reproduzem as ações equivalentes.
2. **Foco nunca desaparece.** Ao entrar em uma página, selecionar automaticamente o primeiro elemento relevante ou restaurar o último foco coerente.
3. **Voltar preserva contexto.** Após ver detalhes, retornar ao mesmo jogo, categoria, posição do trilho e filtros quando possível.
4. **Cada página tem uma ação prioritária.** No detalhe de jogo, priorizar Jogar somente quando a operação estiver realmente disponível.
5. **Feedback imediato:** estado visual em até um frame e som opcional; progresso verdadeiro para operações demoradas.
6. **Loading e erro com conteúdo:** mostrar etapa real, opção de retornar ou repetir; nunca spinner interminável sem diagnóstico.
7. **Não misturar navegação do sistema com jogo.** Entrar em streaming muda o contexto e as dicas de controle.
8. **Acessibilidade:** contraste de texto padrão alvo WCAG AA (≥ 4,5:1 quando aplicável), componentes essenciais com contraste ≥ 3:1, foco destacado, texto escalável, controle acessível, sem depender apenas de animação ou cor.
9. **Dispositivos:** respeitar área segura, dimensões de tela e taxa de atualização. Alvo de fluidez do design: 60 fps quando suportado, sem garantia prévia de hardware.

### Mapa das telas

| Página | Direção de layout | Componentes centrais |
|---|---|---|
| **Login** | Calmo e confiável, sem excesso de brilho | Identidade Nimbox, código legível, instruções, status real |
| **Home** | Vitrine cenográfica premium | `AppShell`, `GameCase3D`, `CaseCarousel`, `GameRail` |
| **Biblioteca** | Descoberta rápida e controle preciso | busca, filtros, grades/trilhos, foco e estados |
| **Detalhes** | Hero do jogo, informação e ação clara | arte, disco decorativo, metadados, ações reais |
| **Configurações** | Organização e leitura, menos efeitos | painéis, `SettingRow`, agrupamentos e feedback |
| **Streaming** | Imersão sem distração | overlay mínimo e temporário, controles essenciais |

## 10. Critérios de aceite do design

**Uma tela não está aprovada porque compila.** Para cada atualização visual será necessário:

- [ ] Captura real da tela de referência e da implementação no mesmo tamanho/aspect ratio.
- [ ] Composição de capa/volumes/reflexos compatível com a referência aprovada.
- [ ] Hierarquia, espaçamento, tipografia, contraste e alinhamento revisados.
- [ ] Navegação funcional por controle e teclado; toque quando aplicável.
- [ ] Estados de foco/seleção/voltar coerentes.
- [ ] Animação testada em execução real e modo reduzir movimento respeitado.
- [ ] Sem botão fictício, dado inventado ou rótulo de outro aplicativo.
- [ ] Sem regressão visual em Android/desktop/console selecionado.
- [ ] Nenhuma alteração nos fluxos técnicos fora do escopo autorizado.

## 11. Governança e próximas atualizações

- **Fase 01 — agora:** documento oficial + tokens + galeria visual; apenas design e documentação.
- **Fase 02 — próxima atualização, mediante aprovação:** biblioteca de componentes visuais e estados, começando pelo shell e pelo carrossel.
- **Fase 03:** motion, transições e protótipos sonoros; validação com controle.
- **Fase 04:** revisão visual tela a tela; aceitação por comparação de captura.

**Política de fonte única:** definições visuais futuras devem usar tokens e componentes padronizados. O HTML 3.0 é a **referência visual**, não uma implementação pronta em C++; não chamar a recriação nativa de equivalente sem validação lado a lado. O Design System v1 é um documento de decisão, e não uma modificação do motor gráfico.

---

**Fonte visual:** Preview Premium 3.0 / Preview 3.1 Limpo, fornecidos no projeto Nimbox. **Responsável pelo projeto:** Nimbox. **Proposta de sistema visual:** v1.0.0 — pendente de aceite dos tokens não presentes diretamente na referência.