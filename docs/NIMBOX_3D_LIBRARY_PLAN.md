# Nimbox 3D Library — Plano técnico (Fase 0)
Status: protótipo experimental; NÃO substitui a interface nativa.
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
- [ ] Implementar layout de carrossel de capas com leve perspectiva na UI nativa, com foco central.
- [ ] Reusar dados reais de `GameTile` e imagem existente de `ImageCache`; placeholder para capas indisponíveis.
- [ ] Usar navegação de `NavInput` (esquerda/direita, aceitar, voltar) preservando a lógica de seleção já validada.
- [ ] Colocar atrás de alternância experimental; preservar modo clássico para rollback.
- [ ] Medir FPS, tempo de desenho e memória em Android.
### Fase 2 — Caixa 3D
- [ ] Definir renderer: textura de capa/lombada/verso, transformações, sombra e oclusão.
- [ ] Garantir desenho correto e suave em plataformas de destino, evitando regressões na UI nativa.
- [ ] Rotação, verso, abertura e disco virtual de forma incremental.
- [ ] Não fabricar informações de embalagem: gerar elementos gráficos genéricos e usar apenas metadados realmente disponíveis.
### Fase 3 — Tema Nimbox
- [ ] Marca, paleta, tipografia, estados de foco, ícones, tela de detalhes e configurações.
- [ ] Rever permissões/licença de artes e manter créditos e GPL-3.0-or-later.
### Fase 4 — APK Alpha
- [ ] Criar variant com applicationId próprio (ex.: `com.nimbox.alpha`) e rótulo distinto; sem modificar o app já instalado.
- [ ] Compilar no GitHub Actions e checar artefato.
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
