@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
title NimboPlay - Diagnostico xCloud (experimental)

if not exist "%~dp0xcloud-cli.exe" (
    echo ERRO: xcloud-cli.exe nao foi encontrado nesta pasta.
    echo Extraia o ZIP inteiro para uma pasta antes de executar este teste.
    pause
    exit /b 1
)

:menu
cls
echo ============================================================
echo   NIMBOPLAY - TESTE DE RESTRICAO DE DISPOSITIVO (XCloud)
echo ============================================================
echo.
echo   Este teste usa sua propria conta Microsoft/Xbox.
echo   Nao altera o dispositivo nem contorna autorizacoes.
echo   Nenhum video ou jogo sera iniciado.
echo.
echo   1. Killer Instinct 2 Classic  (teste principal)
echo   2. Grand Theft Auto VI      (quando estiver disponivel)
echo   3. Fortnite                 (controle de comparacao)
echo   4. Sair
echo.
choice /C 1234 /N /M "Escolha um jogo [1-4]: "
if errorlevel 4 goto fim
if errorlevel 3 set "GAME=Fortnite"
if errorlevel 2 if not errorlevel 3 set "GAME=Grand Theft Auto VI"
if errorlevel 1 if not errorlevel 2 set "GAME=Killer Instinct 2 Classic"

:modo
echo.
echo Jogo escolhido: %GAME%
echo.
echo   1. CONSULTAR: catalogo, direitos e dispositivos permitidos
echo   2. PROVAR SESSAO: pedir uma sessao e encerrar em seguida
echo      (somente se o servico confirmar direito de acesso)
echo.
choice /C 12 /N /M "Selecione o modo [1-2]: "
if errorlevel 2 goto confirmar

:consultar
echo.
echo Consultando Microsoft xCloud para: %GAME%
echo Quando solicitado, entre com sua propria conta.
echo.
"%~dp0xcloud-cli.exe" diagnose-title "%GAME%"
goto resultado

:confirmar
echo.
echo ATENCAO: a segunda opcao solicita uma sessao real de teste.
echo Ela sera encerrada sem conectar video, som ou controle.
choice /C SN /N /M "Confirma o teste de sessao? [S/N]: "
if errorlevel 2 goto modo
echo.
"%~dp0xcloud-cli.exe" diagnose-title "%GAME%" --probe-session

:resultado
echo.
echo ============================================================
echo  Diagnostico encerrado. Nao compartilhe capturas com XUID,
echo  gamertag, codigos de acesso ou outros dados de sua conta.
echo  Se o jogo nao apareceu, isso NAO comprova bloqueio total.
echo ============================================================
echo.
pause
goto menu

:fim
exit /b 0
