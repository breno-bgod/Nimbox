# Nimbox — PSBox apenas como referência técnica

## Decisão do proprietário

**A aparência e a experiência visual são exclusivamente do Nimbox.**
O arquivo fornecido `psbox-cloud-gaming-main.zip` é referência de código para investigar e corrigir erros específicos do aplicativo; NÃO é modelo visual, de organização das páginas ou da quantidade de menus.

- Fonte de verdade do produto: projeto Nimbox e seus layouts próprios, incluindo a proposta visual premium e o carrossel.
- Fonte técnica auxiliar: ZIP PSBox 1.7.1 (versão indicada por `CMakeLists.txt` do arquivo).
- Base técnica corrente: Nimbox `feature/nimbox-3d-library` em 1.7.6. Não substituir componentes atuais em massa pelos componentes do ZIP antigo.

## Fluxo obrigatório para qualquer correção baseada no ZIP

1. Identificar o defeito **reproduzível** no Nimbox (plataforma, passos e resultado esperado/observado).
2. Comparar apenas a implementação responsável no Nimbox com a implementação equivalente do PSBox, sem copiar a interface do PSBox.
3. Explicar a causa e propor uma correção **mínima e isolada**. Obter aprovação específica antes de modificar lógica de autenticação, controles, navegação, catálogo, abertura de jogos, streaming ou serviços.
4. Preservar integralmente o layout e os recursos visuais Nimbox; testes de regressão devem cobrir selecionar jogo, abrir detalhes, iniciar jogo, retornar ao menu e troca de abas conforme a implementação Nimbox aprovada.
5. Validar CI das plataformas, compilar e testar no PS5 físico quando aplicável; nunca alegar homologação do console sem captura e evidência real.
6. Publicar/mesclar apenas após aprovação do proprietário.

## Situação do PR #7

Após a reversão das alterações funcionais indevidas, o diff do PR em relação à branch 3D contém **somente** uma pequena personalização de cores e bordas das abas nativas em `src/ui/app_ui_render.cpp`, além deste documento. Ela não copia o layout do ZIP PSBox.

**Nenhuma correção funcional específica derivada do ZIP foi aplicada neste PR.** A utilidade do ZIP é investigar falhas concretas quando forem identificadas; não justificar mudanças de design.

## Critérios de aceite

- [x] Distinção entre referência técnica PSBox e design próprio do Nimbox documentada
- [x] Mudanças funcionais experimentais anteriores removidas do diff do PR #7
- [ ] Problemas específicos reproduzidos e classificados antes de correção
- [ ] CI aprovado para qualquer alteração técnica futura
- [ ] Homologação visual e funcional no PS5 real quando necessária
- [ ] Aprovação expressa para merge ou release
