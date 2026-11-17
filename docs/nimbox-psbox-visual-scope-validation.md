# Escopo visual Nimbox — referência PSBox

Referência fornecida pelo proprietário: `psbox-cloud-gaming-main.zip` (CMake 1.7.1). Base do Nimbox: `feature/nimbox-3d-library` (CMake 1.7.6).

## Limite da correção

As interfaces do ZIP de referência possuem três abas (Game Pass, Meus jogos, Consoles), navegação por L1/R1, botão de configurações, pesquisa e abertura de detalhes. A base 3D do Nimbox conserva esses fluxos. **A versão antiga 1.7.1 não deve substituir arquivos posteriores 1.7.6**; apenas orienta a preservação funcional.

Nesta correção, o PR #7 volta a usar exatamente o código C++ de `feature/nimbox-3d-library` para todos os fluxos de navegação, autenticação, catálogo, controles, streaming e testes, removendo as páginas e rotas funcionais adicionadas sem aprovação. Somente o desenho (fundo/borda/contraste) das três abas nativas foi modificado, mantendo a geometria e os alvos de toque originais.

## Checklist para lançamento

- [x] Código funcional recolocado conforme a base 3D atual, sem copiar módulos antigos do ZIP
- [x] Os nomes e a quantidade de abas continuam os originais
- [x] Nenhuma mudança em streaming, autenticação, controle, compra ou associação de jogos
- [ ] CI da revisão de escopo visual aprovado
- [ ] Compilação específica do PS5 com SDK do projeto
- [ ] Capturas reais da tela do PS5 analisadas (safe-area, bordas, foco, legibilidade)
- [ ] Teste físico de abertura de jogos e retorno à biblioteca
- [ ] Aprovação do usuário antes de integrar/publicar

**Não é homologação PS5.** O CI comum não compila o alvo final do console.
