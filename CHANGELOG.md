# Histórico de versões — NimboPlay

## 1.7.7 — atualização oficial do código (18/11/2026)

Esta versão consolida na linha principal o trabalho de interface, identidade e melhorias técnicas desenvolvido em branches anteriores. A versão anterior registrada no CMake era 1.7.6.

### Interface e navegação
- Marca pública **NimboPlay** e ícones de inicialização para Android, Windows, Linux e PS5; identificadores técnicos legados são preservados quando exigidos por compatibilidade.
- Tela Horizon com nome da coleção em destaque, contadores de posição, wallpaper contínuo e carrossel finito.
- Navegação por controle, teclado/mouse e toque compartilhando os limites da coleção. Reiniciar no outro extremo exige uma segunda ação deliberada após soltar a direção.
- Ordem do catálogo recebida do serviço preservada, inclusive em **Adicionados recentemente**, sem promoções manuais de jogos.
- Página de detalhes em estilo console e interface nativa em português do Brasil por padrão.

### Configurações
- Painel contextual fosco com a Home ou a página de detalhes desfocada ao fundo.
- Superfícies translúcidas e tratamento de contraste aprimorado.
- QR Code do painel remoto ampliável por R3 ou toque/clique, com leitura sobre fundo opaco.
- Fechar Configurações retorna à tela de origem.

### Compatibilidade
- A linha integrada inclui implementações existentes de mídia, controles e streaming, incluindo o terceiro caminho de decodificação Android Surface.
- O CMake e o conteúdo PS5 foram sincronizados com o número **1.7.7**. A versão Android é derivada automaticamente do CMake.

### Validação e distribuição
- Testes automatizados de UI, geometria do carrossel, renderização e CI são parte do processo de integração.
- **Versão oficial no código não equivale, por si só, a pacote PS5 assinado e publicado.** A distribuição PS5 exige build com o SDK, assinatura com a chave do projeto e publicação de uma Release.
- Builds Android de *debug* com sufixo de pacote experimental continuam separados da versão *release* do aplicativo; não confundir APK de CI com pacote oficial assinado.
- Validação real de compatibilidade/performance do PS5 e leitura com câmera de TV/portátil depende de testes em hardware.

A licença GPL e as atribuições às obras anteriores permanecem preservadas.
