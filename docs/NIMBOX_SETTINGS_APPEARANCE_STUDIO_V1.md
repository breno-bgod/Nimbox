# Nimbox — Painel de Configurações e Ambiente

## Referências visuais
- O usuário forneceu captura do painel `Configurações` do Preview Premium 3.0/3.1.
- O HTML `docs/prototypes/nimbox-preview-3.1.html` contém cinco temas `aurora`, `ice`, `ocean`, `emerald`, `cinema`, controles de reflexo e movimento reduzido.
- A paleta final da **interface do aplicativo** é carvão/grafite com linhas discretas esverdeadas; os cenários da coleção usam as paletas por escolha. Não reproduzir literalmente o azul do HTML nem trocar a identidade Nimbox.

## Layout
- Cabeçalho original Nimbox e botão Voltar permanecem. Nenhuma navegação fictícia é acrescentada.
- Coluna esquerda: todos os ajustes reais do PSBox, em grupos visuais (conta e região, transmissão e imagem, controles e acessibilidade, manutenção, aparência). Os IDs, valores, modais e comandos originais são mantidos. A lista rola com as mesmas entradas.
- Coluna direita: cinco amostras clicáveis **Aurora / Infinity Light / Nimbox Blue / Emerald / Cinema Dark**, ajuste **Reflexo do piso (0–100%)**, **Movimento reduzido**, e o **QR real do controle remoto** compactado na parte inferior.
- Em dispositivos com controle, as três novas preferências estão nas linhas 28, 29 e 30 no final da lista de Configurações, operadas com a navegação já existente. Em touchscreen os cartões/slider/interruptor do painel direito aplicam imediatamente.
- Não adicionar opções de servidor, resolução ou bitrate que não estejam conectadas à configuração funcional já existente.

## Comportamento efetivo
- O tema altera o gradiente de céu/piso atrás das capas **no C++ nativo**, mantendo a arte hero do xCloud.
- Reflexo controla o alfa das imagens especulares no piso (0 elimina, 100 aumenta de modo limitado). Não toca em shaders de streaming.
- Movimento reduzido controla a animação do carrossel/disco e é sincronizado com o atalho R2 já existente. A preferência é salva.
- Três campos só visuais `nimboxTheme` (0..4, padrão 0), `nimboxReflection` (0..100, padrão 44) e `nimboxReducedMotion` (bool, padrão false) são serializados em `settings.json` sem alterar o protocolo de nuvem. Instalações anteriores ganham os padrões e os valores malformados são limitados.
- A comunicação continua pelo `SettingsChanged` já existente, gerenciado pelo mesmo fluxo de gravação. Os demais campos de sessão/login/transmissão/controles mantêm seus valores.

## Restrições e validação
- Não alterar `src/xcloud/`, `src/auth/`, `src/stream/`, `src/input/`, `src/media/`, decodificação, streaming, sessão nem controle de jogo.
- Alterações de `app_ui_input.cpp` tratam **somente do painel Configurações** e da persistência do atalho de movimento reduzido preexistente.
- Testar serialização, ida-e-volta de preferências, cliques em amostras/slider, movimento reduzido, arquivos de temas, diferenças de imagem entre Aurora e Cinema Dark.
- Gerar capturas Settings e Home com dois temas no autoteste; verificar escala e legibilidade Android/PS5 antes de qualquer publicação.
- PR experimental, sem merge em `main` e sem release.
