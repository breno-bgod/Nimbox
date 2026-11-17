# Nimbox Coleção Viva — Geometria 3D nativa, fase experimental 1

**Branch:** `feature/colecao-viva-mesh3d-v1`  
**Origem:** Coleção Viva `feature/colecao-viva-visual-v1`.  
**Estado:** experimento em C++; não é a conclusão da paridade visual HTML/nativa.

## Objetivo

Substituir os trapézios 2,5D definidos manualmente por caixas físicas com geometria tridimensional, usando os mesmos contratos de pose e câmera aprovados para o Preview 3.0/3.1 e compilados em C++/WASM. **Não** criar outro aplicativo e **não** mudar streaming ou autenticação.

## O que esta fase realmente implementa

- `src/ui/box_carousel.h`: caixa rígida (8 cantos em 3D, dimensões `286 × 389 × 34` no palco lógico); rotação em Y, escala, deslocamento XYZ e projeção perspectiva calculados em C++.
- Reutilização de `previewPose` e de `kPerspective = 1250` do motor C++ original. Mapeamento de `1600×900` do preview para a tela lógica nativa de `1920×1080`.
- Face frontal, uma lateral visível e topo obtidos da projeção dos 8 cantos, não de números de trapézios por distância.
- Interpolação dos *parâmetros da pose 3D*, evitando a troca abrupta dos cantos 2D durante a seleção.
- `hit()` cobre frente, lateral e topo projetados; seleção, retorno e Play originais não são alterados.
- `src/ui/app_ui_render.cpp` desenha essas faces com materiais provisórios no Canvas já existente, mantendo capas provenientes do catálogo real, reflexos, foco e acessibilidade.
- `tests/test_main.cpp` verifica dimensões, profundidade, animação, coordenadas finitas e acerto do toque em cena.
- Os autotestes desktop já existentes continuam produzindo capturas da Home e dos Detalhes. Sem rede e sem contas em teste.

## Limite técnico, sem prometer GPU inexistente

Esta fase é **geometria 3D real projetada em 2D por C++**, mas **a rasterização continua no Canvas de software**; não há z-buffer de GPU, correção perspectiva das texturas por fragmento, shading PBR ou sombreamento de materiais por shader. A geometria é compartilhada com o Preview, mas o visual renderizado ainda poderá diferir do CSS. Não chamar esta versão de renderizador 3D acelerado por GPU.

A próxima decisão técnica deve ser tomada **após revisar as capturas e o desempenho**, auditando os backends reais do PS5, Android/SDL3, Windows e Linux. Se adotarmos GPU, preservar o mesmo contrato matemático, com backend(s) por plataforma e fallback documentado. Não adicionar biblioteca nova sem verificar licenças, suporte aos consoles e desempenho.

## Critérios de comparação visual

1. Home em resolução 1920×1080, mesma seleção e sem efeitos de animação: comparar geometria, tamanho, posição, clipping e legibilidade contra o Preview 3.1 em estágio equivalente.
2. Estado intermediário de carrossel e vizinhos ±1/±4: sem esticamento arbitrário de caixas ou saltos de posição; a ação Select e o toque devem abrir detalhes.
3. Verificar imagens reais e fallback quando `tileUrl` ou `heroUrl` não estiver disponível.
4. Confirmar controle, touch, foco, redução de movimento e perfis de CPU nas plataformas reais. CI não substitui homologação PS5.
5. **Não fazer merge para `main` ou publicar release sem aprovação visual explícita.**

## Referências de qualidade — três skills instaladas

- **Better Design:** geometria medida e compartilhada, interação previsível, material físico e movimento com causa.
- **UI UX Pro Max:** acerto de toque, foco acessível, consistência entre dimensões, desempenho e testes em plataformas.
- **Frontend Design:** jogo e capa como protagonistas, restrição do brilho e assinatura visual Coleção Viva.

A tela **ainda não está homologada**; esta entrega é a primeira base 3D verificável para aperfeiçoamento incremental.
