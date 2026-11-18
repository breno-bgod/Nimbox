# Nimbox — motor de layout C++ compartilhado com Preview HTML

Iniciado em 16/11/2026. Branch experimental `feature/nimbox-3d-library`; a `main` permanece intacta.

## Problema e decisão

O Preview 3.0/3.1 usa **CSS 3D com perspectiva de 1250 px**,
caixas 286 × 389 px, transformações `translate3d`, `rotateY`, `scale`,
reflexos, gradientes, sombras e transições. O APK usa atualmente outro modelo
de geometria: quadriláteros em software Canvas/C++ (`box_carousel.h`).
A aparência não é equivalente. Não descrever o APK atual como réplica do preview.

A estratégia passou a ser **motor C++ como fonte de verdade**, capaz de
compilar para as plataformas nativas e para **WebAssembly no navegador**.
Não substituir a UI por Android WebView: Android, Windows, Linux e consoles
devem receber a mesma interface nativa quando o renderizador estiver pronto.

## Primeira entrega verificável: poses C++/WASM (fase 1A)

- `src/ui/nimbox_pose_core.h`: medidas e poses oficiais copiadas
  do HTML aprovado; offset circular e distância máxima visível.
- `src/ui/nimbox_pose_core.cpp`: ABI simples exportada, sem SDL, Android
  ou APIs específicas; compilada como parte de `xcloud_ui` em todas as plataformas.
- `tools/nimbox/export_preview.py`: compila **esse mesmo C++** para WASM
  (clang / wasm-ld), embute o binário no HTML original **sem alterar o CSS**,
  e substitui o cálculo JS das poses, visibilidade e ordem de sobreposição
  por chamadas ao código C++ exportado.
- `tests/test_main.cpp`: testes nativos do contrato de poses e wrap.
- CI: o job `nimbox-preview-cpp-wasm` gera artefato
  `nimbox-cpp-wasm-preview`, uma página HTML autônoma, pronta para navegador.
- O script interrompe a exportação se largura do palco, proporções das
  caixas ou perspectiva CSS mudarem sem revisão do contrato.

Para reproduzir localmente:

```bash
python3 tools/nimbox/export_preview.py \
  docs/prototypes/nimbox-preview-3.1.html \
  Nimbox_Preview_3_1_CPP_WASM.html
```

Abrir o arquivo gerado em navegador moderno. O indicador técnico
`document.documentElement.dataset.nimboxEngine` deve ser
`cpp-wasm`. Os controles de navegação do Preview continuam funcionando.

## Limites desta fase — importante

**O HTML é a mesma apresentação CSS**, mas agora recebe posições e seleção
de uma compilação C++ real; não é um renderizador completo C++ no navegador.
O C++ nativo já compila o mesmo módulo, porém **o renderer atual do APK ainda
usa a geometria 2,5D antiga**. Por isso não há alegação de paridade visual
entre APK e HTML.

## Próximas fases (ainda não concluídas)

1. Implementar projeção/perspectiva e geometria de caixa 3D **em C++**
   que use o mesmo motor de cena. Preferir aceleração GPU/SDL/Vulkan,
   com fallback para os consoles e APIs compatíveis. Não converter CSS
   automaticamente em C++ com promessa de fidelidade sem teste.
2. Migrar luz, ambiente, laterais, reflexos, capa, moldura,
   animação `cubic-bezier(.21,.76,.16,1)` e hierarquia visual para
   primitivas/recursos **comuns**, não códigos independentes.
3. Fazer comparações de frames nas mesmas resoluções (1600×900 e Android),
   inclusive durante animações, e definir limites de divergência aprovados.
4. Preservar autenticação, catálogo real, controles, detalhes e streaming
   existentes; nenhuma funcionalidade falsa só para alcançar aparência.
5. Publicar APK de teste somente depois da captura nativa e revisão visual.
   Manter PR em draft, não mesclar `main` até homologação.

O objetivo final é uma única definição de layout no C++ e uma cadeia de
renderização nativa multiplataforma; a preview HTML serve de régua visual
e de harness de depuração, **não** de implementação Android exclusiva.
