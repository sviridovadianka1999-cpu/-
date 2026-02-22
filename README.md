# ESP8266 LED Matrix Kit (45x8 WS2812B) + Windows Controller

Готовый проект «под ключ» из двух частей:
- `firmware/` — прошивка ESP8266 NodeMCU v3 (Arduino/PlatformIO).
- `pc_app/` — мини-приложение Windows (Python + Tkinter).

## Структура
- `firmware/include/config.h` — железо, лимиты питания и Wi-Fi дефолты.
- `firmware/include/matrix_map.h` — `XY(x,y)` для вашей змейки 45x8.
- `firmware/include/effects.h`, `firmware/src/effects.cpp` — эффекты.
- `firmware/include/api.h`, `firmware/src/api.cpp` — HTTP/WS API.
- `firmware/src/main.cpp` — запуск, Wi-Fi STA/AP, автозагрузка состояния.
- `pc_app/app.py` — GUI управление с пресетами.

## Важно про безопасность питания
1. Яркость жестко ограничена 30% (`BRIGHTNESS_LIMIT_255 = 76`).
2. Установлен лимит потребления через FastLED: `setMaxPowerInVoltsAndMilliamps(5, 3000)`.
3. Безопасные дефолты на старте (умеренная яркость и не-белый режим).

## Подключение проводов
Для NodeMCU v3 + WS2812B:
1. `D2 (GPIO4)` -> `DIN` ленты.
2. `GND NodeMCU` -> `GND` ленты и `GND` блока питания (общая земля обязательно).
3. `VIN(5V)` NodeMCU можно питать от 5V (через USB удобнее для прошивки), сама лента питается от отдельного 5V БП 10A.
4. Рекомендуется резистор 330–470 Ом в разрыв DATA и конденсатор 1000 мкФ на питание ленты.

## Шаг 1. Установка CH340 драйвера (Windows)
1. Подключите NodeMCU по USB.
2. Установите драйвер CH340 (официальный пакет WCH CH34x).
3. В «Диспетчер устройств» проверьте, что появился COM-порт (например COM5).

## Шаг 2. Прошивка NodeMCU

### Вариант A (PlatformIO, рекомендован)
1. Установите VS Code + расширение PlatformIO.
2. Откройте папку проекта.
3. Откройте `firmware/` как PlatformIO Project.
4. Нажмите **Build**.
5. Нажмите **Upload** (выберите ваш COM-порт).

CLI:
```bash
cd firmware
pio run
pio run -t upload
```

### Вариант B (Arduino IDE)
Можно импортировать `.cpp/.h` из `firmware/`, установить библиотеки FastLED, ArduinoJson, WebSockets, LittleFS и собрать для ESP8266 NodeMCU 1.0.

## Шаг 3. Первый запуск Wi‑Fi
- Дефолтный STA Wi‑Fi:
  - SSID: `RT-GPON-39C2`
  - Password: `USYHb7EGae`
- Если роутер недоступен, поднимается fallback AP:
  - SSID: `LED-MATRIX-SETUP`
  - Password: `12345678`
  - Откройте `http://192.168.4.1/wifi` и введите новый SSID/пароль.

## Как узнать IP устройства
1. Через Serial Monitor (115200) — в логе печатается `STA IP`.
2. Через роутер (список DHCP клиентов).
3. Через кнопку **Автопоиск** в ПК-приложении.

## API (HTTP + WebSocket)
- HTTP:
  - `GET /api/status`
  - `POST /api/apply`
  - `GET/POST /wifi`
- WebSocket:
  - `ws://<ip>:81/` — принимает те же JSON-команды, что и `/api/apply`.

### Пример команды
```json
{
  "set_mode": "text",
  "set_brightness": 60,
  "set_speed": 90,
  "set_text": "HELLO",
  "set_text_speed": 100,
  "set_text_direction_ltr": false,
  "set_text_color": "#FF0000",
  "set_colors": ["#0000FF", "#8000FF", "#FF00FF"],
  "save_config": true
}
```

### Поддерживаемые ключи
- `set_mode`: `off|color|gradient|rainbow|text|fire|matrix|bounce`
- `set_brightness` (авто-кламп до 30%)
- `set_speed`
- `set_text`
- `set_text_speed`
- `set_text_direction_ltr`
- `set_text_color`
- `set_colors` (массив HEX)
- `wifi_config`: `{ "ssid":"...", "password":"..." }`
- `save_config`: `true/false`

## Шаг 4. Запуск приложения на ПК
```bash
cd pc_app
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python app.py
```

## Сборка .exe (PyInstaller)
```bash
cd pc_app
pip install -r requirements.txt
pyinstaller --onefile --windowed app.py --name led_matrix_controller
```
Готовый EXE будет в `pc_app/dist/led_matrix_controller.exe`.

## Функции приложения
- Подключение по IP.
- Режимы: Цвет, Градиент, Переливание, Текст, Огонь, Матрица, Прыгающий пиксель.
- Регулировки: яркость (не выше 30%), скорость, цвета, параметры текста.
- Кнопки: Применить, Выключить, Сохранить/Загрузить пресет.
- Автопоиск по подсети /24.

## Первый тест после прошивки
1. Подайте питание на матрицу и ESP.
2. Убедитесь, что устройство в сети (или fallback AP).
3. Запустите `pc_app/app.py`.
4. Введите IP, нажмите «Подключиться».
5. Выберите режим `Текст`, введите текст, нажмите «Применить».
6. Смените на `Огонь` и `Матрица`, проверьте скорость.

## Примеры пресетов

### 1) Красный текст
```json
{
  "set_mode":"text",
  "set_brightness":50,
  "set_text":"HELLO",
  "set_text_color":"#FF0000",
  "set_text_speed":100,
  "save_config":true
}
```

### 2) Сине-фиолетовый градиент
```json
{
  "set_mode":"gradient",
  "set_brightness":60,
  "set_speed":80,
  "set_colors":["#0000FF", "#4B0082", "#8000FF"],
  "save_config":true
}
```

### 3) Огонь медленный
```json
{
  "set_mode":"fire",
  "set_brightness":45,
  "set_speed":40,
  "save_config":true
}
```

### 4) Matrix rain быстрый
```json
{
  "set_mode":"matrix",
  "set_brightness":55,
  "set_speed":200,
  "save_config":true
}
```

## Типичные проблемы и решения
1. **Устройство не появляется в сети**
   - Проверьте SSID/пароль.
   - Проверьте питание и общую землю.
   - Подключитесь к AP `LED-MATRIX-SETUP` и заново задайте Wi‑Fi.
2. **Не работает текст**
   - Используйте короткий текст для теста (`HELLO`).
   - Проверяйте UTF-8 ввод.
   - Поддержка кириллицы базовая (uppercase с упрощенными глифами).
3. **Мерцание/глюки**
   - Обязательно общая GND.
   - Поставьте резистор на DATA и конденсатор по питанию.
   - Не увеличивайте лимит яркости и power-limit.
4. **Не подключается приложение**
   - Проверьте IP и что `/api/status` открывается в браузере.
   - Отключите VPN/файрвол для локальной сети.

## Принятые допущения
1. Power-limit выбран 3000mA как безопасный софт-лимит.
2. Кириллица реализована частично (uppercase + приближенные глифы), латиница/цифры работают полноценно.
3. Для простоты ПК-приложение использует HTTP (WebSocket в прошивке также доступен для расширений).
4. Автопоиск — быстрый скан /24 сети.
