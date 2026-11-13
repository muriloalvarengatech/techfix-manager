@echo off
rem ======================================================================
rem compilar.bat - Compila o TechFix Manager no Windows (gcc / MinGW)
rem
rem Uso: compilar.bat compila e espera Enter no final
rem compilar.bat vscode compila sem pausa (usado pelo VS Code)
rem compilar.bat debug compila com informacoes de depuracao (-g)
rem
rem Requer o gcc no PATH. O Dev-C++ instala o gcc em:
rem C:\Program Files (x86)\Dev-Cpp\MinGW64\bin
rem ======================================================================
setlocal
set MODO=%1

rem Opcoes do compilador: padrao C99, todos os avisos e as pastas de cabecalhos.
rem __USE_MINGW_ANSI_STDIO=1 faz o printf aceitar "%%lld" (long long) no MinGW.
set FLAGS=-std=c99 -D__USE_MINGW_ANSI_STDIO=1 -Wall -Wextra -Iinclude -Ilib\sqlite
if /i "%MODO%"=="debug" (set FLAGS=%FLAGS% -g -O0) else (set FLAGS=%FLAGS% -O1)

rem Modulos usados pelo sistema e pelos testes (tudo, menos a interface).
set COMUM=src\util\erro.c src\util\validacao.c src\util\io.c src\util\sha256.c src\regras\regras.c src\regras\seguranca.c src\dados\db.c src\dados\esquema.c src\dados\cadastros.c src\dados\estoque.c src\dados\ordens.c src\dados\relatorios.c
rem Interface de terminal e programa principal.
set INTERFACE=src\ui\ui_comum.c src\ui\menu_cadastros.c src\ui\menu_ordens.c src\ui\menu_estoque.c src\ui\menu_admin.c src\main.c

where gcc >nul 2>nul
if errorlevel 1 (
echo [ERRO] gcc nao encontrado. Adicione a pasta bin do MinGW ao PATH.
goto erro
)

rem O SQLite e compilado uma unica vez (leva cerca de 1 minuto).
if not exist obj mkdir obj
if not exist obj\sqlite3.o (
echo Compilando o SQLite pela primeira vez...
gcc -O1 -Ilib\sqlite -c lib\sqlite\sqlite3.c -o obj\sqlite3.o || goto erro
)

echo Compilando o TechFix Manager...
gcc %FLAGS% %COMUM% %INTERFACE% obj\sqlite3.o -o TechFixManager.exe || goto erro

echo Compilando os testes...
gcc %FLAGS% tests\testes.c %COMUM% obj\sqlite3.o -o TestesTechFix.exe || goto erro

echo.
echo [OK] Pronto. Execute TechFixManager.exe (sistema) ou TestesTechFix.exe (testes).
goto fim

:erro
echo.
echo [ERRO] A compilacao falhou. Veja as mensagens acima.
if /i not "%MODO%"=="" exit /b 1
pause
exit /b 1

:fim
if "%MODO%"=="" pause
exit /b 0
