# pillarEngine
pillarEngine is C++ 3D engine. You can compile games!

#instructions
# PillarEngine

Полноценный 3D игровой движок на C++ / OpenGL с редактором, скриптами, VPK-архивами и системой сборки игр.

---

## Что умеет движок

| Система | Возможности |
|---|---|
| **Рендерер** | OpenGL 4.5, Blinn-Phong освещение, направленный + точечные источники (до 8), сетка редактора |
| **Примитивы** | Куб, сфера, капсула, плоскость — создаются в один клик |
| **Модели** | GLB, GLTF, OBJ, FBX через Assimp |
| **Текстуры** | PNG, JPG, BMP, TGA, HDR |
| **Аудио** | WAV, MP3, OGG, FLAC, OPUS через miniaudio |
| **Сцены** | Формат `.pilevel` (JSON), Open/Save/New как в Unity |
| **Скрипты** | C++, базовый класс `ScriptComponent`, макрос регистрации |
| **ECS** | entt — быстрый Entity-Component-System |
| **VPK архивы** | Упаковка и распаковка ассетов (совместимый формат) |
| **Редактор** | ImGui, Dockspace, Hierarchy, Inspector, Viewport, Content Browser, Console |
| **Камера редактора** | RMB + WASD + Scroll для навигации |
| **Сборка игры** | Упаковывает ассеты в VPK, выдаёт готовый `.exe` |

---

## Требования

- **Visual Studio 2022** (v17+) с поддержкой C++20
- **CMake 3.20+**
- **Git** (для FetchContent зависимостей)
- **OpenGL 4.5** совместимая видеокарта

---

## Сборка (Visual Studio)

```bat
git clone <repo_url> PillarEngine
cd PillarEngine

# Создать build директорию и сгенерировать VS solution
cmake -B build -G "Visual Studio 17 2022" -A x64

# Открыть в Visual Studio
start build\PillarEngine.sln
```

В Visual Studio:
1. Выбрать стартовый проект: **PillarEditor** (или PillarGame / PillarPacker)
2. Конфигурация: **Debug** или **Release**
3. Нажать **F5** для запуска

> Первая сборка скачает все зависимости (GLFW, GLM, ImGui, Assimp, EnTT, miniaudio и др.) через CMake FetchContent. Нужен интернет.

### Настройка glad

`glad` нужно сгенерировать вручную (один раз):
1. Зайти на https://glad.dav1d.de/
2. Language: **C/C++**, Specification: **OpenGL**, API gl: **Version 4.5**, Profile: **Core**
3. Скачать архив
4. Положить `glad.c` в `vendor/glad/src/`
5. Положить `glad.h` в `vendor/glad/include/glad/`

Или установить через vcpkg:
```bat
vcpkg install glad
```

---

## Структура проекта

```
PillarEngine/
├── Engine/                  ← Ядро движка (статическая библиотека)
│   └── src/
│       ├── Core/            ← Application, Log, Input, UUID
│       ├── Renderer/        ← Renderer, Camera, ModelLoader, MeshRegistry
│       ├── Scene/           ← Scene, Entity, Components
│       ├── Scripting/       ← ScriptEngine, ScriptComponent
│       ├── Audio/           ← AudioSystem (miniaudio)
│       └── VPK/             ← VPKArchive (pack/unpack)
├── Editor/                  ← Редактор (ImGui)
│   └── src/
│       ├── EditorApp.cpp    ← Точка входа
│       └── EditorLayer.cpp  ← Весь UI редактора
├── Game/                    ← Ваша игра
│   ├── src/
│   │   └── GameApp.cpp      ← Точка входа игры
│   ├── scripts/
│   │   ├── PlayerController.cpp
│   │   └── EnemyAI.cpp
│   └── assets/
│       └── levels/
│           └── main.pilevel ← Демо-сцена
├── Tools/
│   └── Packer/              ← CLI инструмент для VPK
└── vendor/                  ← Все зависимости (FetchContent)
```

---

## Редактор — управление

### Камера редактора
| Действие | Клавиша |
|---|---|
| Летать | ПКМ + WASD |
| Подъём / опускание | ПКМ + Space / Shift |
| Zoom | Колесо мыши |

### Горячие клавиши
| Действие | Клавиша |
|---|---|
| Новая сцена | Ctrl+N |
| Открыть сцену | Ctrl+O |
| Сохранить | Ctrl+S |
| Сохранить как | Ctrl+Shift+S |
| Play / Stop | F5 |

### Создание объектов
Меню **Create** → Cube / Sphere / Capsule / Plane / Point Light / Directional Light / Camera / Empty Entity

---

## Формат сцены `.pilevel`

JSON файл. Пример:

```json
{
    "name": "My Level",
    "version": 1,
    "entities": [
        {
            "tag": "Player",
            "uuid": 12345,
            "active": true,
            "transform": {
                "position": [0, 0, 5],
                "rotation": [0, -90, 0],
                "scale":    [1, 1,  1]
            },
            "meshRenderer": {
                "meshPath":    "__capsule__",
                "texturePath": "",
                "color":       [0.2, 0.6, 1.0]
            },
            "scripts": ["PlayerController"]
        }
    ]
}
```

**Встроенные meshPath:**
- `__cube__` — куб
- `__sphere__` — сфера
- `__capsule__` — капсула
- `__plane__` — плоскость 10×10
- `path/to/model.glb` — любая модель

---

## Написание скриптов

Создай файл `Game/scripts/MyScript.h`:

```cpp
#pragma once
#include <Scripting/ScriptComponent.h>
#include <Scene/Components.h>
#include <Core/Input.h>

class MyScript : public Pillar::ScriptComponent {
public:
    void OnStart() override {
        PL_INFO("MyScript started!");
    }
    void OnUpdate(float dt) override {
        auto& tc = GetComponent<Pillar::TransformComponent>();
        tc.Position.y += dt;  // медленно летит вверх
    }
};
```

Создай `MyScript.cpp`:

```cpp
#include "MyScript.h"
PILLAR_REGISTER_SCRIPT(MyScript)
```

Добавь `#include "../scripts/MyScript.cpp"` в `GameApp.cpp`.

В редакторе: выбери объект → Inspector → **Add Component** → **Attach Script...** → выбери `MyScript`.

---

## Встроенные скрипты

### PlayerController
- **WASD** — движение
- **Мышь** — камера (FPS стиль)
- **Space** — прыжок (если есть Rigidbody)
- **Escape** — разблокировать курсор
- `MoveSpeed = 8.0f`, `MouseSensitivity = 0.12f`

### EnemyAI
- Ищет entity с именем `"Player"`
- Идёт на него со скоростью `MoveSpeed = 6.0f`
- Останавливается на расстоянии `CatchRadius = 1.2f`

---

## VPK архивы

### Через редактор
**File** → **Build Game (VPK)** — упакует `assets/` в `build/game.vpk`.

### Через CLI инструмент (PillarPacker)

```bat
# Упаковать папку в VPK
PillarPacker.exe pack assets/ game.vpk

# Распаковать VPK
PillarPacker.exe unpack game.vpk extracted/

# Посмотреть содержимое
PillarPacker.exe list game.vpk
```

### Из кода
```cpp
// Упаковать
Pillar::VPKArchive::Pack("assets", "game.vpk");

// Читать файл из VPK
Pillar::VPKArchive ar;
ar.Open("game.vpk");
auto data = ar.ReadFile("levels/main.pilevel");
```

---

## Сборка игры в .exe

1. В редакторе: **File** → **Build Game (VPK)** — создаёт `game.vpk`
2. Собери в Visual Studio в конфигурации **Release**: проект **PillarGame**
3. В папке `build/bin/Release/` будет:
   - `PillarGame.exe`
   - `game.vpk` (или папка `assets/`)
   - `shaders/`
4. Раздай пользователям эти три объекта

---

## Добавление своих ассетов

Положи файлы в `Game/assets/`:
```
Game/assets/
├── levels/      ← .pilevel сцены
├── models/      ← .glb .obj .fbx
├── textures/    ← .png .jpg
└── audio/       ← .wav .mp3 .ogg
```

В редакторе:
- Content Browser автоматически покажет файлы
- Двойной клик на `.pilevel` — открывает сцену
- В Inspector → Mesh Renderer → поле **Mesh** вписать путь `models/mychar.glb`
- В Inspector → Audio Source → поле **Clip** вписать путь `audio/music.ogg`

---

## Зависимости (все скачиваются автоматически)

| Библиотека | Назначение |
|---|---|
| GLFW 3.4 | Окно и OpenGL контекст |
| glad (генерируется) | OpenGL загрузчик |
| GLM 1.0.1 | Математика |
| Dear ImGui 1.90.5 | UI редактора |
| EnTT 3.13.2 | Entity-Component-System |
| Assimp 5.3.1 | Загрузка 3D моделей |
| miniaudio 0.11.21 | Аудио |
| stb_image | Текстуры |
| nlohmann/json 3.11.3 | Сериализация сцен |

---

## Лицензия

MIT — используй в своих проектах свободно.
