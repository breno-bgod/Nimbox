# Nimbox — referência técnica PSBox testada

## Fonte e decisão de escopo

O proprietário forneceu `psbox-cloud-gaming-main (1).zip` e informou que seu funcionamento já foi testado. O CMake do ZIP declara **versão 1.7.6** (SHA-256 do arquivo: `9b256ef5be49235604988b539e72969a0278041cda9dbb80993813385be7c75c`).

**O PSBox é referência de comportamento e código, não de aparência.** Preservar o visual próprio do Nimbox, seu carrossel, seus elementos de interface e as decisões visuais aprovadas.

Base de comparação: `feature/nimbox-3d-library` no Nimbox (versão CMake 1.7.6). O ZIP antigo fornecido anteriormente declarava 1.7.1 e não substitui este ZIP novo.

## Resultado da comparação por conteúdo (Git blob SHA-1)

Arquivos presentes no ZIP **idênticos byte a byte** à base 3D Nimbox:

| Subsistema | Arquivos conferidos |
|---|---|
| Controles PS5 e USB | `src/input/controller_ps5.cpp`, `src/input/controller_common.cpp`, `src/input/pad_usb.cpp`, `src/input/pad_usb_ps5.cpp`, `src/input/pad_usb_hid.cpp` |
| Biblioteca e atualização | `src/app/library.cpp`, `src/app/library_sync.cpp`, `src/app/worker.cpp` |
| Reprodução e streaming | `src/app/worker_stream.cpp`, `src/app/stream_player.cpp`, `src/app/stream_screen.cpp` |
| Serviços xCloud | `src/xcloud/catalog.cpp`, `src/xcloud/gssv.cpp`, `src/xcloud/social.cpp`, `src/xcloud/regions.cpp` |

**Conclusão:** copiar esses módulos do ZIP para o Nimbox não produziria correção alguma, pois já são iguais. A leitura estática anterior identificou *condições possíveis*, não comprovou bugs nesses módulos. A informação do proprietário de que o PSBox foi testado reforça a necessidade de reproduzir qualquer falha especificamente no Nimbox antes de corrigi-la.

## Divergências de integração relevantes

Os arquivos `src/ui/app_ui_input.cpp`, `src/ui/app_ui.cpp`, `src/ui/app_ui_render.cpp`, `src/ui/app_ui.h` e `src/app/app_core.cpp` não são todos iguais ao ZIP. Parte das diferenças pertence à identidade e ao carrossel do Nimbox.

Comparação funcional direcionada:
- `Core::dispatchUiAction` e `Core::frame` em `app_core.cpp`: mesmas funções byte a byte no ZIP e na base Nimbox, portanto o despacho do comando de jogar e o loop principal foram preservados.
- `AppUi::showHome`, `AppUi::isEffectivePlayableLocked`, `AppUi::drawTabs` e `AppUi::onTouch`: idênticas no recorte verificado.
- `AppUi::handleHome`: a base Nimbox contém atalhos/rotação exclusivos do carrossel e navegação horizontal circular, que não devem ser apagados por cópia da UI PSBox.
- `AppUi::openDetails`: o Nimbox inclui animação de abertura/retorno à tela de detalhes; o restante do fluxo de seleção é correspondente.
- `tests/test_main.cpp` da base 3D inclui testes de navegação circular e seleção da capa; também verifica o comando `Action::Play` após abrir os detalhes.

## Regras de trabalho

1. Não copiar visual ou arquivos inteiros de UI do PSBox.
2. Não substituir nenhum módulo funcional sem demonstrar divergência, erro reproduzido e necessidade da alteração.
3. Testar ponta a ponta seleção de capa, abertura de detalhes, comando Play, inicialização do stream, retorno e input de controle por plataforma.
4. CI automatizado não equivale a teste físico de PS5. Não afirmar homologação sem evidência do console.
5. Qualquer mudança em login, streaming, controle, biblioteca ou navegação exige escopo explícito e validação; preservar a `main` e fazer alterações isoladas.
6. O PR #7 permanece focado em apresentação visual das abas e nesta documentação; **nenhuma correção funcional foi necessária ou aplicada após esta comparação**, porque os módulos testados eram idênticos.

## Validação

- [x] Nova referência 1.7.6 inspecionada e hashes dos módulos críticos conferidos
- [x] Origem funcional do Nimbox preservada
- [x] Diferenças de integração visual identificadas
- [x] Fluxos existentes cobertos por testes automatizados no CI da revisão anterior
- [ ] Teste físico e evidências de controle/streaming no PS5
- [ ] Qualquer problema real a ser corrigido precisa ser reproduzido no Nimbox
- [ ] Aprovação do proprietário antes de integrar/publicar
