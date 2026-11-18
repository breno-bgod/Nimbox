# Nimbox Coleção Viva — Arco 3D e lombadas em cascata (experimental)

## Intenção visual

A composição mantém **duas camadas**: (1) caixa física Xbox verde translúcida/fosca e (2) mídia impressa com arte do catálogo xCloud. A referência de posicionamento é uma estante arqueada com jogo central totalmente frontal e perfis progressivamente mais acentuados nas extremidades. **A referência Nintendo Wii U não muda a identidade Xbox do Nimbox.**

### Geometria aprovada para esta experimentação

- Oito vértices tridimensionais e a projeção de perspectiva original continuam em `box_carousel.h`; espessura, largura e altura da caixa não foram modificadas.
- As poses de `nimbox_pose_core.h` foram refinadas: 0° frontal, ~24° adjacentes, ~50° intermediárias, ~79° penúltimas e ~87° no perfil externo.
- Estes valores são **yaw aparente em relação à direção da câmera** (ponto de fuga). Como as caixas externas estão deslocadas em X, o yaw *global* da malha recebe compensação para que a frente realmente fique quase de perfil. Por isso os ângulos de rotação global usados no código são `0/36/67/99/108` graus; o ângulo projetado percebido é menor.
- Posições laterais em estágio `1600×900`: `0/275/420/520/580` px, espelhadas por lado. A compressão progressiva gera **sobreposição física em cascata**, mantendo partes distinguíveis das lombadas externas. O espaçamento das duas últimas posições foi refinado após os testes CI identificarem que a caixa da terceira lateral podia ficar totalmente encoberta; agora cada nível conserva uma faixa visível.
- Mantemos a interpolação de pose 3D entre seleções. Não transformamos caixas em simples trapézios nem alteramos as artes.

### Leitura de embalagem

- Centro: toda a frente visível, bandeira Xbox e encarte dinâmico.
- Vizinhas: parte da frente ainda dominante, com lombadas começando a aparecer.
- Externas: redução acentuada da área frontal, lombadas verdes em destaque e símbolo Xbox em escala reduzida no topo das lombadas.
- Mantemos acabamento fosco, as bordas moldadas e o encaixe da arte implementados no PR #10.

### Interações

O helper compartilhado `caseDrawOrder` devolve a ordem de renderização de trás para frente; o toque percorre essa mesma ordem invertida. Assim a caixa visualmente em primeiro plano é a selecionada em regiões de sobreposição, inclusive durante a animação. Os fluxos existentes de Detalhes/Play, login, catálogo e streaming continuam os mesmos.

### Ensaios

- Teste `testNimboxCascadeArc`: compara yaw aparente com o resultado da câmera, verifica diminuição da face, perfil extremo dominado pela lombada, sobreposição nas duas laterais e coordenadas finitas.
- Autoteste da Home usa **nove jogos sintéticos** para visualizar ±4 posições sem acesso à rede; o aplicativo real segue usando as capas do xCloud.
- Compilar CI nativo + WebAssembly, inspecionar capturas Home/Detalhes, validar foco/toque e, posteriormente, homologar em PS5 e Android.

**Limite técnico:** o modelo tem geometria 3D projetada, mas ainda usa o rasterizador de software Canvas. Não é renderização GPU nem garantia de equivalência total ao HTML. O Preview 3.1 original continua guardado como referência histórica; o exportador C++/WASM gera a geometria desta iteração sem sobrescrever o HTML original.

**Publicação:** somente branch e PR experimental. Não mesclar na `main` nem liberar atualização oficial antes da homologação do usuário.
