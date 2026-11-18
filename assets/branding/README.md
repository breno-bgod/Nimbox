# NimboPlay — duas identidades visuais

## Marca interna (interface)
`assets/branding/nimboplay_internal.png` é a **logo transparente** usada pelo Canvas
C++ nos menus, login, configurações e avisos. O CMake incorpora essa imagem ao
executável. Não deve ser usada como ícone de instalação.

## Ícone externo (launcher)
`assets/branding/nimboplay_launcher_256.png` é a versão otimizada, derivada do
**PNG quadrado azul fornecido pelo usuário**. Essa é a origem única para:
- **Android:** recursos `mipmap-*/ic_launcher*.png`, ícone adaptativo com fundo
  azul e `drawable-nodpi/nimboplay_launcher.png` para o banner de Android TV
- **Windows:** `nimboplay_launcher.ico` gerado no diretório de build, embutido
  em `psbox.exe` por `windows/psbox.rc`
- **Linux:** `psbox.png` do pacote/AppImage, copiado da origem única
- **PS5:** `sce_sys/icon0.png` **RGBA de 512 × 512 px**, gerado durante
  `tools/ps5/package.sh` e em `tools/ps5/home-art.sh`

O utilitário `tools/branding/make_launcher_assets.py` usa apenas a biblioteca
padrão de Python, sem dependências externas. A partir da raiz do repositório:

```sh
python3 tools/branding/make_launcher_assets.py \
  --source assets/branding/nimboplay_launcher_256.png \
  --ps5 ps5/sce_sys/icon0.png --ico /tmp/nimboplay_launcher.ico
python3 -m unittest discover -s tools/branding -p 'test_*.py' -v
```

A geração não modifica a logo interna. Para atualizar `pic0.dds` e
`pic1.dds` da tela inicial do PS5, execute `tools/ps5/home-art.sh`
no ambiente de desenvolvimento com PS5_Vulkan e dependências de BC7.
**A alteração dos ícones não valida o app em um console PS5 físico.**

## Compatibilidade
Nome visível ao usuário: **NimboPlay**. Permanecem os identificadores de
migração, entre eles `psbox.exe`, `com.rafaelngp.psbox`, o sufixo debug
`.nimboxalpha` e o title ID PS5 `PPSA99810`. Alterar esses IDs requer um
projeto de migração específico para evitar perda de dados e conflitos de
instalação. Não alterar automaticamente autenticação, streaming ou controles.
