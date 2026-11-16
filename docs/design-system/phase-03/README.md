# NIMBOX — FASE 03 · MOTION DESIGN E IDENTIDADE SONORA

**Versão:** 1.0.0 · **Data:** 16/11/2026 · **Estado:** proposta visual em revisão · **Escopo:** somente design.

> Base obrigatória: **Design System Nimbox v1.0.0 (Fase 01)**, **biblioteca de 14 componentes NX/ (Fase 02)**, **Preview Premium 3.0 / Preview 3.1 limpo**. Esta fase não implementa áudio nativo, não altera C++, login, gamepad, catálogo, streaming ou configuração funcional. O HTML é um laboratório, não uma compilação C++ e nem prova de equivalência multiplataforma.

## 1. Objetivo

Estabelecer uma assinatura de comportamento que torne a UI previsível, fluida e distinta: **movimentos físicos**, sem exageros, com resposta imediata, transições interrompíveis e sons originais opcionais. Os componentes e a paleta vêm das fases 01–02; esta fase apenas detalha ritmo, som e feedback.

## 2. Linguagem de movimento — tokens propostos

| Token | Alvo | Comportamento | Regras |
|---|---:|---|---|
| `motion.focus` | 120 ms | contorno e realce | foco responde no mesmo frame; não deslocar o alvo |
| `motion.press` | 95 ms | compressão breve | sem bloquear acionamento |
| `motion.page` | 300 ms | fade/slide suave | preservar histórico, filtros, seleção e foco |
| `motion.modal` | 220 ms | elevação leve | prender foco no modal; restaurá-lo ao fechar |
| `motion.carousel` | 920 ms | translação de caixa física e rotação Y | *easing* `cubic-bezier(.21,.76,.16,1)`; aceitar inputs interrompendo o movimento |
| `motion.feedback` | 180 ms | brilho transitório | não substituir mensagem de erro real |
| `motion.disc` | 7.500 ms/volta | movimento decorativo circular | somente Detalhes, não obrigatório e pausado em reduzir movimento |

**Importante:** a duração de 920 ms foi obtida da referência HTML; os demais tokens já estavam propostos na Fase 01. Não supor que o Canvas 2,5D C++ atual execute essas transições com fidelidade.

### Direção coreográfica

- **Entrada:** revelar hierarquia, não atrasar o foco. Quando possível, compor movimento do item central e painéis com transição física, mantendo texto legível.
- **Navegação lateral:** permitir repetição rápida de D-pad/teclado, sem fila de animações. Interromper o destino anterior de forma controlada.
- **Modal:** aparecer acima do plano base, reduzir estímulos em segundo plano e devolver o foco ao botão originador.
- **Detalhes:** proteger clareza de metadados; disco decorativo com opção de pausa/redução.
- **Erros:** evitar flashes e vibração inesperada; estado explícito e mensagem acessível.

## 3. Identidade sonora — protótipos originais

A identidade sonora **não** é música de fundo. Os eventos são pequenos sinais eletrônicos, criados por síntese procedural original no navegador e destinados a avaliação estética — não são arquivos `.wav` finais nem recursos instalados no aplicativo.

| Evento | Esboço | Duração-alvo | Prioridade |
|---|---|---:|---|
| `ui.focus` | toque aerado/curto | 56 ms | baixa |
| `ui.select` | dois harmônicos de confirmação | 126 ms | média |
| `ui.back` | tom descendente | 94 ms | média |
| `ui.open` | abertura suavizada | 170 ms | baixa |
| `ui.error` | dois pulsos amortecidos | 182 ms | alta |
| `ui.ready` | conclusão suave | 190 ms | média |

### Comportamento e acessibilidade do som

1. **Desativado por padrão** no playground; nenhum autoplay ou áudio de fundo.
2. Sons reproduzidos somente após habilitação explícita, com volume independente do jogo.
3. **Limiter de foco:** no mínimo 125 ms entre sons de foco; suprimir feedback duplicado em scroll rápido.
4. Persistir *mute* só quando houver decisão de design aprovada; neste playground, o estado **não é salvo**.
5. Respeitar áudio do sistema, perfil de acessibilidade e redução de estímulos na implementação real.
6. Sons são desenhos originais: **não copiar efeitos do Xbox, PlayStation ou qualquer outra marca**.

## 4. Matriz de UX: eventos de interface

| Interação | Movimento | Som opcional | Critério de teste |
|---|---|---|---|
| Mover para a capa adjacente | `motion.carousel` | `ui.focus` | novo título visível; movimento substituível; sem lag de input |
| Entrar em Detalhes | `motion.page` | `ui.open` | capa e título correspondem ao selecionado |
| Voltar à Home | `motion.page` | `ui.back` | restaura capa, trilho e contexto |
| Confirmar ação disponível | `motion.press` | `ui.select` | ativação ocorre sem esperar animação |
| Abrir/fechar modal | `motion.modal` | `ui.open` / `ui.back` | foco confinado e restaurado |
| Concluir operação verdadeira | `motion.feedback` | `ui.ready` | sinal não substitui status real |
| Erro verdadeiro | `motion.feedback` | `ui.error` | não simular erros inexistentes |

## 5. Reduzir movimento e fallback

- Nenhuma rotação/parallax decorativa quando `reducedMotion` estiver ativo; tempo de carrossel/página/modal pode chegar a 0 ms, desde que a mudança de estado seja acessível.
- Foco continua nítido sem depender da animação e não se perde.
- A implementação deve adaptar custo de iluminação, reflexos e partículas ao desempenho real da plataforma.
- Não declarar qualidade nativa ou latência antes de capturas e medições no Android, desktop e consoles.

## 6. Biblioteca visual incluída

O playground da Fase 03 **herda a cena 3D e as capas da Fase 02**, os 14 componentes, os quatro exemplos de tela e o inspector original. A página acrescenta: laboratório para os 7 movimentos, régua comparativa de tempos, 6 botões de amostra sonora, mute, volume e exportação de dados JSON.

A navegação com teclado e mouse/toque continua sendo **uma demonstração local**. Nenhum botão apresentado aciona xCloud, login ou streaming real.

## 7. QA e critérios de aprovação

- [ ] Movimento de foco, press, página, modal, carrossel, feedback e disco aprovados estética e funcionalmente na prévia.
- [ ] Áudio original testado em navegador com gesto explícito, botão de mute e volume.
- [ ] Testar dinâmica com áudio desativado e com preferências de redução de movimento do SO.
- [ ] Controle remoto, teclado e toque conservam um único foco e não travam em animações.
- [ ] Verificar legibilidade e desempenho na TV 16:9, Android landscape e desktop, sem cortar itens essenciais.
- [ ] Validar captura e transições nativas por plataforma antes de chamar a implementação aprovada.
- [ ] Nenhum componente não funcional foi confundido com ação de sistema.
- [ ] Revisar possíveis avisos sonoros antes de enviar para o app real (SFX final ainda não exportado).

## 8. Entregáveis

- `Nimbox_Motion_Sonic_Fase_03_Interativo.html`: página autônoma offline, derivada dos componentes da Fase 02.
- `Nimbox_Motion_Sonic_Fase_03_Tokens.json`: contratos e parâmetros para implementação posterior.
- `Nimbox_Motion_Sonic_Fase_03.md`: esta especificação.

**Próxima etapa proposta:** Fase 04 — auditoria visual por tela e protocolo de paridade (Home, Biblioteca, Detalhes, Login e Configurações), ainda sob foco de design.