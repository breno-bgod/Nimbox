# Skills de design do Nimbox para Codex

Instalação **local ao projeto** (revisão isolada, sem modificar binários):

| Skill | Repositório de origem | Versão fixada (commit) |
|---|---|---|
| Better Design | https://github.com/Moddingflow/better-design | `c373ad7fa122b8c0d63b1be756443cc93a56e71d` |
| UI UX Pro Max | https://github.com/nextlevelbuilder/ui-ux-pro-max-skill | `e59ff376a41f7ca47b7541ad5c116495cbade3ee` |
| Frontend Design | https://github.com/anthropics/skills/tree/main/skills/frontend-design | `main` (cópia auditada na data de integração) |

Todos os arquivos de `skills/better-design/` (49), de `.claude/skills/ui-ux-pro-max/` (78) e de `skills/frontend-design/` (2) foram copiados para as pastas correspondentes em `.agents/skills/`, preservando referências, scripts e dados de consulta, com licenças. O SKILL.md da UI UX Pro Max substitui o caminho `${CLAUDE_PLUGIN_ROOT}/.claude/skills/.../scripts/search.py` pelo caminho local do projeto.

## Como utilizar

Abra o projeto Nimbox no Codex utilizando a branch que contém estas pastas. Para começar uma revisão exclusivamente visual, use esta solicitação:

> Use as skills locais ui-ux-pro-max, frontend-design e better-design em conjunto para auditar e propor o design Nimbox Coleção Viva. Preserve carrossel de caixas 3D, CD giratório, a identidade visual original e os fluxos existentes do PSBox. Faça inicialmente só análise e propostas visuais; não modifique código, lógica dos jogos ou serviços sem aprovação.

A skill de UI UX Pro Max inclui uma base de pesquisa local opcional:

```bash
python3 .agents/skills/ui-ux-pro-max/scripts/search.py "cloud gaming console UI" --design-system -p Nimbox
python3 .agents/skills/ui-ux-pro-max/scripts/search.py "focus gamepad navigation" --domain ux
```

Para o design nativo PS5/C++ não presuma que recomendações CSS/React são implementáveis diretamente. Traduzir os princípios para os componentes `src/ui/`, respeitando a arquitetura da aplicação.

## Segurança e escopo

- Nenhum módulo de controle, catálogo, streaming, autenticação ou visual foi modificado nesta instalação.
- Nenhum serviço de IA externo, API paga, pacote runtime, plug-in do app ou componente de navegador é adicionado ao Nimbox.
- As skills orientam o Codex; o uso do Codex é sujeito ao plano/limites do usuário.
- Nunca executar scripts de terceiros nem adicionar ferramentas ao pipeline de build sem revisão, e nunca usar links externos para enviar dados privados do Nimbox.
- Para código nativo, PR visual isolado e validação conforme `AGENTS.md` antes de merge/release.
