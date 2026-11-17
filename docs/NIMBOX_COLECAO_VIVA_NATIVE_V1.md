# NIMBOX — Coleção Viva · UI nativa v1

> **Escopo:** implementação inicial no C++ nativo, sem alteração do catálogo, streaming, autenticação, descoberta de jogos, mapeamento de controles, transições de telas ou regras de assinatura. Protótipos gráficos anteriores são referências visuais, não layouts pixel-perfect de produção.

## Direção de UI/UX

Visão combinada de três skills de design consultivas:

- **UI UX Pro Max:** hierarquia, contraste, espaçamento, design system e foco acessível.
- **Frontend Design:** identidade visual própria e acabamento intencional; evitar copiar diretamente PS5 ou Xbox.
- **Better Design:** transições, motion reduzido, feedback visual e validação real de telas.

Identidade: *Coleção Viva* — caixas de jogos em perspectiva, mídia óptica digital animada e arte de cada jogo como cenário. A tela de biblioteca continua eficiente para grandes catálogos; elementos decorativos não mudam ações.

## Fonte única dos dados

- `GameTile.name`, `tileUrl`, `heroUrl`, `publisher`, `categories` e `playable`: provenientes da integração já existente com o PSBox/xCloud.
- Não inventar valores de 4K, HDR, progresso, duração, plataforma, instalação ou assinatura. As imagens conceituais usam dados fictícios; estes não são inseridos no catálogo real.
- `Select` no carrossel continua abrindo os detalhes do título. `Jogar` nos detalhes executa o **mesmo** fluxo original. O CTA visual do carrossel diz “Ver detalhes”, pois é a ação atual; o botão Jogar permanece nos detalhes quando o título está disponível.
- Não adicionar eventos, filtros ou botões que aparentem ser clicáveis sem terem ação real.
- O CTA “VER DETALHES” da Home agora é uma área real de clique/toque e chama `openDetails` do jogo selecionado, assim como Select no controle. A área de desenho e a de toque compartilham `kCollectionDetailsCta`.
- A caixa exibe a marca Nimbox; o rótulo Xbox só identifica uma geração quando `GameTile.platform` realmente a informa. Não desenhar símbolo Xbox improvisado nem presumir plataforma para `platform` vazio.

## Tokens base desta primeira etapa

| Uso | Token |
|---|---|
| Fundo nativo | `rgba(8,17,31)` |
| Painel | `rgba(17,36,53)` |
| Placeholder | `rgba(26,45,63)` |
| Acento visual | ciano `rgba(79,225,247)` |
| Texto principal | branco / `kWhite` |
| Texto secundário | `kGray` e cores azul-acinzentadas |
| Foco por gamepad | duas bordas cianas de intensidades distintas |

Aplicar iluminação com moderação. O jogo selecionado é protagonista; menus e adornos ficam em segundo plano.

## Páginas e situação real

| Tela | V1 efetivamente implementado | Próxima revisão |
|---|---|---|
| Home — Game Pass | Fundo de arte real, carrossel físico 2.5D original, reflexos existentes, CD novo no carrossel, título do jogo e indicação da ação verdadeira | Refinar perspectiva e animação com GPU quando testada |
| Detalhes | Caixa física com capa real, disco rotativo reposicionado, realce ciano no botão Jogar, preservando textos/achievements/compra | Revisar escalas, clipping e safe areas |
| Biblioteca | Paleta, foco e cabeçalho alinhados; grade e ordenação originais preservadas | Redesign de layout da grade com testes de controle/touch separados |
| Consoles e configurações | Fundo e header partilhados, sem alterar controles | Reformular cards e listas em PR posterior |

## Plataformas e testes

A interface continua em C++/Canvas, dentro do pipeline de apresentação existente (PS5 ou SDL3 desktop/Android). A primeira etapa **não** adiciona navegador web, JS ou dependência 3D.

- O novo disco na Home é atualizado a **até ~12 FPS** para reduzir carga da CPU; o disco de detalhes mantém ~24 FPS. Na v1.1, seu raio foi reduzido para não disputar a atenção com as capas.
- A preferência **R2 de movimento reduzido** continua interrompendo o giro.
- Quando imagens do catálogo estão ausentes ou ainda carregando, a interface usa fundos e caixas com cores de reserva. Nenhum dado é fabricado.
- Capturas para análise em CI desktop: `6-colecao-viva-home.png` e `6b-colecao-viva-details.png`, produzidas pelo `psbox --selftest` sem conta ou rede.
- Necessária validação humana em tela e teste de controle/Android/PS5 real. CI desktop passando **não valida** PS5 nem demonstra fidelidade à imagem de conceito.

## Não interferência

Arquivos alterados: `src/ui/app_ui_render.cpp`, `src/ui/app_ui_internal.h`, `src/app/desktop_selftest.cpp`, além deste documento. Não alterar `src/xcloud/`, `src/input/`, `src/media/`, `src/app/library_sync.cpp` ou runtime de autenticação.

Não fazer merge ou release sem aprovação do usuário.
