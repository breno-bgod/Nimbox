# Nimbox Premium Preview 3.0 / 3.1 — restauração estrutural nativa

**Origem visual analisada:** `Nimbox_Preview_3_0_Premium.html` fornecido pelo usuário. O Preview 3.1 e o carrossel em arco do PR #11 continuam referências complementares. O HTML era um protótipo visual offline e NÃO era fonte de autenticação, streaming, biblioteca ou controles.

## Regra obrigatória: visual sem modificação de sistemas

Esta PR altera apenas `src/ui/app_ui_render.cpp`, `src/ui/app_ui_internal.h`, arquivos de layout visual, autoteste e testes. **Não editar `src/ui/app_ui_input.cpp`, `src/xcloud`, `src/auth`, `src/stream`, `src/input`, `src/media`, `src/display`, roteamento de páginas, sessão, APIs ou lógica de controle.** O único ajuste de geometria de clique é no token `kCollectionDetailsCta`, compartilhado previamente entre render e input. Isso move o botão **junto com sua área de toque existente**, sem editar o handler.

## Mapeamento direto da estrutura HTML para as telas C++ existentes

| Preview HTML | Componente nativo | Resultado e limite |
|---|---|---|
| `#topbar`, `.topnav` e `#footer` | `drawTopBar`, `drawTabs`, `drawHints` | Barra fixa, identidade, pills, menu de configurações e rodapé. Somente as 3 abas **reais** (Game Pass, Seus jogos, Consoles), sem botões fingindo abrir páginas que não existem. |
| `#view-home #hero`, `#casefield` | `drawNimboxCarousel` | Carrossel PR #11 preservado: arco, espessura, material Xbox, arte dinâmica, animações e foco. Topo agora é um estúdio luminoso, não uma página inteiramente ocupada por caixas. |
| `#home-shelves`, `.shelf-section`, `.tile-row` | Rail no rodapé de `drawNimboxCarousel` | Se o aplicativo tem playtime real, mostra **Continue jogando**; caso contrário, uma prévia da coleção em foco. Usa **somente** `GameRow.tiles`, `GameTile.tileUrl` e `SettingsChoice.titlePlaytime`, nunca dados inventados. É prévia **somente visual**: os comandos existentes cima/baixo continuam escolhendo a coleção, e os cartões do rail não fingem ter toque próprio. |
| `#view-library`, `.lib-grid` | `drawLibrary` | Cabeçalho e organização em grade do Preview, mantendo listas reais de adquiridos, compráveis e ocultos, ordenação e área de seleção originais. |
| `#view-gamepass` | Aba Game Pass na Home | Utiliza catálogo e linhas reais; **não cria uma página Game Pass duplicada** só porque existia na maquete. |
| `#view-details`, `.detail-contents` | `drawDetails` | Coluna de informação em vidro fosco + jogo/case/disco à direita; Play, achievements, queue, preferências e histórico intactos. |
| `#view-settings`, `.settings-layout` | `drawSettings` em `app_ui_settings.cpp` | Já possui dois painéis nativos, configurações reais à esquerda e web/QR real à direita. Não copiar opções de demonstração para o sistema. Nenhuma linha ou interação de Settings alterada. |
| `#view-login`, `.login-box` | `drawSignIn` | Dois painéis e breadcrumb seguindo o Preview; URL/código Microsoft, QR, botões e fluxos de login originais preservados. |
| `#view-profile`, `#view-friends`, `#view-genres` | Sem páginas correspondentes independentes no app atual | **Não criar perfis, amigos, chats ou filtros falsos**; são demonstrações no HTML e exigiriam produto/roteamento próprios. Dados realmente existentes continuam disponíveis nas telas nativas que já os suportam. |

## Composição de 1920×1080

- Cabeçalho nativo até cerca de Y=122 (coordenadas reais de foco/tabs mantidas).
- Estúdio 3D já aprovado no PR #11, sem reescrever seus oito vértices ou poses.
- Título do jogo em Y=711, metadados em Y=765 e botão Detalhes compartilhado Y=806..858.
- Rail com cabeçalho após Y=869, capas em Y=925..1016, footer em Y=1018..1080.
- Na altura de uma tela **apenas o primeiro rail fica visível**, como na área inicial do preview. O app nativo ainda não tem scroll vertical dedicado a essas vitrines. Não simular uma página rolável sem implementar navegação depois e sem autorização.
- Playtime visto no autoteste é **sintético e restrito à captura offline**. No aplicativo real, `Continue jogando` só aparece quando `settings_.titlePlaytime` registra atividade.

## Testar e homologar

1. Rodar GitHub CI, C++ unit, desktop selftest, Windows, Linux, Android e preview WebAssembly. Validar screenshot Home com rail **e** Details.
2. Verificar que o CTA visual abre o mesmo jogo por toque depois da mudança de coordenadas; testar abas, busca, biblioteca, Settings, Login, navegação e Play pelo fluxo original.
3. Comparar capturas 1920×1080 contra Preview 3.0/3.1, procurando clipping, hierarquia, escala e proporção entre hero e rail.
4. Validar fisicamente PS5 e Android antes de promover qualquer branch para `main`.
5. Não afirmar paridade pixel-perfect: o Preview usa CSS/HTML; o Nimbox continua com Canvas C++ software. Não adicionar painéis sem função real.

**Critério de aceite:** a Home NÃO pode ser apenas um carrossel isolado. Deve conservar navegação, vitrine, trilhos de acervo e rodapé da identidade aprovada, sem regressões nos sistemas existentes.
