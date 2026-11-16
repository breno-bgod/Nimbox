# Nimbox — Preview 3.1 aprovado e integração nativa

Atualização solicitada após revisão do Preview 3.0 (16 de novembro de 2026).

## Escopo autorizado
- Conservar apenas controles realmente operacionais; não simular login, chat, notificações, jogo executável, conta de usuário ou alteração de configurações de streaming.
- A visualização HTML é um artefato independente fornecido na conversa: `Nimbox_Preview_3_1_Limpo.html`. Na visualização, apenas navegação, seleção de capa, busca/filtros locais, favoritos locais, temas, reflexos e movimento reduzido estão clicáveis.
- Não confundir esses dados ilustrativos de catálogo com informações reais de conta, classificação ou disponibilidade.

## Continuidade no aplicativo nativo
- A biblioteca visual Nimbox agora é o modo inicial da aba Game Pass; R3 retorna à interface clássica.
- Caixa central destacada e caixas laterais mais largas, com arte reconhecível, lombada e perspectiva aproximada por quad no Canvas.
- Fundo 'infinity room' mais iluminado e reflexos de capa mais visíveis sob o carrossel.
- As alterações anteriores de login Nimbox, detalhes com CD giratório e redesign de configurações continuam.
- A UI nativa **não** é uma cópia da versão HTML: a versão HTML usa transformações CSS 3D e o aplicativo C++ ainda usa renderer 2,5D por software.
- Não foram adicionadas ações falsas, contas fictícias, login demonstrativo ou funções sociais inexistentes ao aplicativo. O catálogo, as configurações reais e os handlers de Play permanecem os da base funcional.

## Critérios de validação
- [ ] Android Debug APK compila no commit HEAD após estas mudanças.
- [ ] Login Microsoft por código, abertura de detalhes, jogo Play e stream testados em dispositivo.
- [ ] Touch, direcional, R3, R2 e bibliotecas do catálogo verificadas.
- [ ] FPS e temperatura/consumo de memória do 2,5D e reflexos avaliados em Android.
- [ ] Usuário compara screenshots reais de APK com HTML Preview 3.1; ajustar diferenças.
- [ ] Decisão de merge em main somente após testes de regressão. Falha Windows anterior precisa ser classificada separadamente.

## Preservação do projeto
Branch: `feature/nimbox-3d-library` (PR #1). `main` preservada até homologação; recursos de licença e identificadores nativos de compatibilidade não alterados.
