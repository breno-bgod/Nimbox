# Horizon universal — integração de serviços (etapas incrementais)

## Decisão de arquitetura

**Existe somente um Horizon:** o renderizador nativo C++/SDL do NimboPlay em `src/ui/`. Xbox Cloud Gaming e Boosteroid são **provedores de dados e ações**, não implementações de interface.

Não substituir autenticação, catálogo, comunicação, controles, decodificação ou streaming existentes. Xbox mantém o backend atual; Boosteroid mantém o backend funcional do OpenStroid. Criar apenas adaptadores de apresentação/comandos quando necessários.

## Situação da branch `feat/horizon-service-login`

- O seletor de serviço já é renderizado pela UI nativa (`Screen::Services`).
- **Correção inicial:** o atalho LT+RB do seletor precisa ser processado antes de RB alternar abas. A Home existente do Xbox permanece intacta.
- O pacote Windows ainda inicia `OpenStroid.exe` como processo Electron e ainda expõe `HorizonHome.tsx`/`HorizonLayout.tsx`. **São provisórios; não são o Horizon universal.**
- A conexão real do login e da sessão Boosteroid à interface nativa ainda é uma pendência. Não considerar a integração concluída enquanto a segunda interface continuar visível.

## Contrato de integração a implementar

A UI nativa recebe dos provedores apenas informação normalizada, com identificação de origem:

| Área do Horizon C++ | Xbox (existente) | Boosteroid (adaptador, sem reimplementar motor) |
| --- | --- | --- |
| Autenticação | fluxo existente Xbox | estado/login do OpenStroid existente |
| Home/carrossel | catálogo e imagens Xbox | jogos/capas fornecidos pelo OpenStroid |
| Catálogo e biblioteca | listas Xbox | listas e jogos instalados do OpenStroid |
| Detalhes | dados e ação Xbox | dados e ação do mesmo OpenStroid |
| Jogar | comando Xbox existente | comando de iniciar stream do OpenStroid existente |
| Configurações | preferências Xbox | preferências oferecidas pelo OpenStroid |
| Sessão | player Xbox existente | player Boosteroid existente |

**Regras obrigatórias da ponte:**

1. O estado do serviço escolhido é separado do estado da conta de cada provedor. Jamais misturar IDs, cache, login ou biblioteca entre os serviços; prefixar identificadores do catálogo por provedor.
2. Não chamar operações do serviço na thread de renderização SDL. Resultados e falhas entram no modelo da interface por eventos/filas assíncronas.
3. O backend Boosteroid pode ter processo separado por razões técnicas; **nenhuma Home, catálogo, login ou configurações visualmente paralelos** deve substituir o Horizon nativo. Caso o login exija navegador/QR, apresentar o estado e as ações no Horizon, mantendo o mecanismo existente de autenticação.
4. Preservar o motor de gameplay do OpenStroid e usar uma superfície dedicada somente quando a reprodução exigir. Não criar streaming, token exchange ou WebRTC novos.
5. Se o adaptador Boosteroid falhar, o Horizon permanece navegável e apresenta erro/retentativa, sem interromper o caminho Xbox.
6. Preservar disponibilidade por plataforma: Boosteroid integrado atualmente apenas no pacote Windows. Não anunciar suporte a Android, Linux ou PS5 sem backend testado.
7. Antes de remover os componentes React provisórios, **concluir e validar** o adaptador que entrega biblioteca e ações ao Horizon nativo; nunca retirar o único caminho funcional do usuário.

## Critérios de aceite

- Mesmas telas e mesmo código de UI para Home, carrossel, catálogo, detalhes, login visual e configurações, seja Xbox ou Boosteroid.
- Troca de serviço via entrada e Home nativas; LT+RB funciona sem mudar a aba.
- Login Boosteroid real, retorno ao catálogo e launch de jogo validados no Windows (não apenas uma janela vazia).
- O processo Boosteroid pode falhar/encerrar sem travar o NimboPlay.
- Testes de regressão do Xbox + builds Windows/Linux/Android preservados.
- Nenhum merge para `main` antes desses critérios serem atendidos.
