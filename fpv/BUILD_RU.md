# Сборка CDDA-FPV

Это исходный снимок CDDA 0.I-1 с изменениями движка; он не устанавливается в data/mods.
Общие зависимости и инструкции платформ см. в ../doc/COMPILING/ и ../README_UPSTREAM.md.

## Linux (обычная сборка)

Установите зависимости CDDA (компилятор C++17, make, SDL2/image/mixer/ttf, FreeType, gettext и зависимости из документации).
Из корня репозитория:

```sh
make -j2 TILES=1 SOUND=1 LOCALIZE=1 RELEASE=1
make -C lang LANGUAGES=ru
```

В настройках выберите русский. Не смешивайте объектные файлы сборок LOCALIZE=0 и LOCALIZE=1; используйте чистое дерево или разные BUILD_PREFIX.

## Windows x64, кросс-сборка из Linux

Нужны MinGW-w64 C++17, GNU make, gettext/msgfmt, pkg-config и Windows SDK зависимостей.
Проверенная комбинация: MinGW GCC 15.2.1, SDL 2.30.11, SDL_image 2.8.2, SDL_mixer 2.8.0, SDL_ttf 2.22.0, FreeType 2.13.3.
SDL_image/mixer здесь собраны/предоставлены с доступными в SDK встроенными кодеками; сторонних звуковых пакетов нет.

1. Разместите кросс-библиотеки в SDK_PREFIX/include, SDK_PREFIX/lib, SDK_PREFIX/bin.
2. Исправьте prefix в lib/pkgconfig/*.pc на абсолютный SDK_PREFIX. Статическая FreeType должна быть в SDK_PREFIX/lib.
3. Запустите:

```sh
SDK_PREFIX=/absolute/path/to/windows-sdk ./fpv/build_windows.sh
```

Скрипт собирает ru-cataclysm-tiles.exe с LOCALIZE=1 и официальный русский MO-каталог.
Для запуска переименуйте копию в cataclysm-tiles.exe и добавьте data/, gfx/, lang/mo/ru/ из исходников,
лицензии и DLL зависимостей из соответствующих SDK. В готовом ZIP всё уже включено.
Конкретный состав DLL зависит от SDK: проверяйте PE imports objdump -p, не копируйте только SDL2.dll.
Файл fpv/config-ru/options.json можно скопировать в config/options.json **только для новой установки**, не затирайте свои настройки.

## Минимальный тест рендера

```sh
c++ -std=c++17 -O1 fpv/tests/test_renderer.cpp -o /tmp/cdda-fpv-renderer-test
/tmp/cdda-fpv-renderer-test
```

Тест выполняет 19236 проверок, включая 19200 попиксельных проверок NaN; это не 19236 отдельных игровых сценариев.
Для тестовой сцены см. extract_fixture_tiles.py (нужен Pillow). SDL smoke-test требует SDL2 и запускается отдельно.
Тесты не заменяют запуск полной игры и проверку на целевой Windows.
