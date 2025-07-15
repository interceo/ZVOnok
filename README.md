# ZVOnok - Discord-like Voice Chat на C++

Проект представляет собой аналог Discord для голосового общения, реализованный на C++ с поддержкой как простой UDP передачи, так и современной WebRTC технологии.

## Возможности

### Базовая версия (UDP)
- Простая передача аудио через UDP
- Серверная ретрансляция аудио
- Использование PortAudio для захвата/воспроизведения звука

### WebRTC версия
- P2P аудио соединения с низкой задержкой
- Автоматическое сжатие аудио (Opus codec)
- NAT traversal через STUN серверы
- Встроенное шифрование
- Система комнат для группового общения
- Сигналинг сервер для установки соединений

## Зависимости

### Обязательные
- CMake >= 3.5
- C++20 совместимый компилятор
- PortAudio 2.0
- trantor

### Для WebRTC функциональности
- [libdatachannel](https://github.com/paullouisageneau/libdatachannel)
- jsoncpp

## Установка зависимостей

### macOS (с Homebrew)
```bash
brew install cmake portaudio jsoncpp
brew install paullouisageneau/datachannel/libdatachannel
```

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install cmake build-essential libportaudio2 libportaudio-dev libjsoncpp-dev

# Для libdatachannel нужна сборка из исходников
git clone https://github.com/paullouisageneau/libdatachannel.git
cd libdatachannel
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

## Сборка

### Быстрая сборка (все версии)
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Сборка только WebRTC версии
```bash
mkdir build && cd build
cmake .. -DBUILD_ORIGINAL=OFF -DBUILD_WEBRTC=ON
make -j$(nproc)
```

### Сборка только базовой версии
```bash
mkdir build && cd build
cmake .. -DBUILD_ORIGINAL=ON -DBUILD_WEBRTC=OFF
make -j$(nproc)
```

## Использование

### Базовая версия (UDP)

#### Запуск сервера
```bash
./build/server/server [port]
# По умолчанию порт 12345
```

#### Запуск клиента
```bash
./build/client/client
# Подключается к 127.0.0.1:12345
```

### WebRTC версия

#### 1. Запуск сигналинг сервера
```bash
./build/server/signaling_server [port]
# По умолчанию порт 12345
```

#### 2. Запуск клиентов
```bash
# Первый клиент
./build/client/client_webrtc [server_ip] [server_port] [room_id]

# Второй клиент (в той же комнате)
./build/client/client_webrtc 127.0.0.1 12345 default
```

#### Примеры использования
```bash
# Локальное тестирование
./build/server/signaling_server
./build/client/client_webrtc 127.0.0.1 12345 room1
./build/client/client_webrtc 127.0.0.1 12345 room1

# Удаленное подключение
./build/client/client_webrtc 192.168.1.100 12345 gaming_room
```

## Архитектура

### WebRTC Flow
```
[Клиент A] ←→ [Signaling Server] ←→ [Клиент B]
     ↓              STUN/TURN             ↓
     └──────── WebRTC P2P Audio ─────────┘
```

1. **Signaling Server** - обрабатывает WebRTC handshake и управление комнатами
2. **WebRTC P2P** - прямая передача аудио между клиентами
3. **STUN серверы** - для NAT traversal (Google STUN серверы)

### Компоненты

- `Audio.hpp/cpp` - обертка для PortAudio
- `WebRTCAudio.hpp/cpp` - WebRTC аудио класс
- `SignalingServer.hpp/cpp` - сигналинг сервер для WebRTC
- `main_webrtc.cpp` - WebRTC клиент с сигналинг протоколом

## Особенности реализации

### WebRTC Audio Pipeline
1. **Захват аудио** - PortAudio → PCM samples
2. **Кодирование** - автоматическое сжатие через libdatachannel
3. **Передача** - P2P через WebRTC DataChannel
4. **Декодирование** - автоматическое в libdatachannel
5. **Воспроизведение** - PCM samples → PortAudio

### Сигналинг протокол
```json
{
  "type": "offer|answer|ice_candidate|join_room",
  "client_id": "client_123456",
  "target": "client_789012",
  "data": { ... }
}
```

## Планы развития

- [x] Базовая WebRTC интеграция
- [ ] Поддержка видео
- [ ] Улучшенное управление качеством звука
- [ ] Web интерфейс для управления
- [ ] Поддержка TURN серверов
- [ ] Аутентификация пользователей
- [ ] Текстовые сообщения
- [ ] Запись разговоров

## Troubleshooting

### Проблемы с аудио
- Проверьте права доступа к микрофону
- Убедитесь, что PortAudio правильно установлен
- Проверьте, что аудио устройства не заняты другими приложениями

### Проблемы с WebRTC
- Убедитесь, что libdatachannel правильно установлен
- Проверьте сетевые настройки (файрволл, NAT)
- При проблемах с соединением используйте TURN сервер

### Проблемы сборки
```bash
# Проверка зависимостей
pkg-config --exists portaudio-2.0 && echo "PortAudio OK"
pkg-config --exists jsoncpp && echo "jsoncpp OK"

# Очистка сборки
rm -rf build && mkdir build && cd build && cmake ..
```

## Лицензия

MIT License - см. файл LICENSE для подробностей. 