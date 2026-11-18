# Nimbox — Home: caixa fechada; Detalhes: disco revelado

## Contrato visual aprovado

Esta alteração trata **somente da apresentação visual** após o PR #12, que preserva o carrossel 3D, a biblioteca inferior do Preview 3.0/3.1 e as páginas reais.

- **Home / Coleção Viva:** carrossel com caixas Xbox fechadas, encartes reais do catálogo, sem disco exposto.
- **Abertura de Detalhes:** a capa central já é um alvo acessível a toque e controle; não há botão redundante `VER DETALHES`.
- **Detalhes do jogo:** sem caixa Xbox duplicada; somente disco com arte do próprio jogo, hub, brilho moderado, transição de entrada e rotação contínua.
- **Animação do carrossel:** a mesma posição/projeção 3D do PR #11 tem interpolação de tempo desacelerada com `1-exp(-6.7*dt)`, mantendo velocidade independente da taxa de quadros. **Não alterar o mapeamento de joystick/controle nem comandos**.
- **Modo de movimento reduzido:** nenhuma rotação contínua do disco; carrossel passa diretamente para o estado final.
- **Cores:** títulos e linhas de suporte agora têm acento ciano menos saturado; a ação `Jogar` de Detalhes acompanha a cor Xbox, enquanto as caixas preservam o acabamento verde fosco.
- **Desempenho:** sem CD animado na Home = sem redraw constante a 12 fps; disco de Detalhes continua limitado a cerca de 24 fps.
- O antigo retângulo de ação está com dimensões `0×0` no token visual compartilhado, de modo que **nenhuma área invisível continua clicável**; o handler de entradas não é alterado.

## Limites de escopo e não-regressão

Não alterar `src/ui/app_ui_input.cpp`, `src/auth/`, `src/stream/`, `src/input/`, `src/xcloud/`, `src/media/`, login, comandos, Play, filtros, carregamento das imagens, nem a geometria física do carrossel. O foco nativo e o toque na caixa projetada continuam funcionando.

## Verificações

1. Compilar CI Windows/Linux/Android; passar testes unitários e o preview C++/WASM.
2. No autoteste offline, clicar no **centro da capa** e abrir Detalhes, sem CTA.
3. Inspecionar capturas: **Home sem CD** e **Detalhes sem caixa**, com CD exibido.
4. Confirmar que os trilhos inferiores, o menu e as opções reais não desapareceram.
5. Verificar fluidez em Android/PS5 físico, taxa de quadros, movimento reduzido e Play antes de qualquer merge.

PR experimental dependente do PR #12. Não publicar na `main` sem aprovação visual explícita.
