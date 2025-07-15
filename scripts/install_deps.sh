#!/bin/bash

# Скрипт для автоматической установки зависимостей
# Поддерживает macOS (Homebrew) и Ubuntu/Debian

set -e

echo "=== ZVOnok Dependencies Installation Script ==="

# Определяем операционную систему
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if [ -f /etc/debian_version ]; then
        OS="debian"
    elif [ -f /etc/redhat-release ]; then
        OS="redhat"
    else
        OS="linux"
    fi
else
    OS="unknown"
fi

echo "Detected OS: $OS"

install_macos() {
    echo "Installing dependencies for macOS..."
    
    # Проверяем наличие Homebrew
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    echo "Installing packages via Homebrew..."
    brew update
    brew install cmake portaudio jsoncpp pkg-config
    
    # Устанавливаем libdatachannel
    echo "Installing libdatachannel..."
    if ! brew list paullouisageneau/datachannel/libdatachannel &> /dev/null; then
        brew tap paullouisageneau/datachannel
        brew install libdatachannel
    else
        echo "libdatachannel already installed"
    fi
    
    echo "macOS dependencies installed successfully!"
}

install_debian() {
    echo "Installing dependencies for Debian/Ubuntu..."
    
    # Обновляем пакеты
    sudo apt update
    
    # Устанавливаем основные зависимости
    echo "Installing basic dependencies..."
    sudo apt install -y \
        cmake \
        build-essential \
        pkg-config \
        libportaudio2 \
        libportaudio-dev \
        libjsoncpp-dev \
        git \
        curl
    
    # Проверяем наличие libdatachannel
    if ! pkg-config --exists datachannel; then
        echo "Installing libdatachannel from source..."
        
        # Создаем временную директорию
        TEMP_DIR=$(mktemp -d)
        cd "$TEMP_DIR"
        
        # Клонируем и собираем libdatachannel
        git clone https://github.com/paullouisageneau/libdatachannel.git
        cd libdatachannel
        git submodule update --init --recursive
        
        mkdir build && cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
        sudo make install
        
        # Обновляем линкер кеш
        sudo ldconfig
        
        # Очищаем временные файлы
        cd ~
        rm -rf "$TEMP_DIR"
        
        echo "libdatachannel installed successfully!"
    else
        echo "libdatachannel already installed"
    fi
    
    echo "Debian/Ubuntu dependencies installed successfully!"
}

install_redhat() {
    echo "Red Hat/CentOS/Fedora installation not fully supported"
    echo "Please install the following packages manually:"
    echo "  - cmake"
    echo "  - gcc-c++"
    echo "  - portaudio-devel"
    echo "  - jsoncpp-devel"
    echo "  - libdatachannel (build from source)"
    exit 1
}

# Выполняем установку в зависимости от ОС
case $OS in
    "macos")
        install_macos
        ;;
    "debian")
        install_debian
        ;;
    "redhat")
        install_redhat
        ;;
    *)
        echo "Unsupported operating system: $OS"
        echo "Please install dependencies manually:"
        echo "  - CMake >= 3.5"
        echo "  - PortAudio 2.0"
        echo "  - jsoncpp"
        echo "  - libdatachannel"
        exit 1
        ;;
esac

echo
echo "=== Installation Complete ==="
echo "You can now build the project:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

# Проверяем установку
echo
echo "Verifying installation..."

check_command() {
    if command -v "$1" &> /dev/null; then
        echo "✓ $1 found"
    else
        echo "✗ $1 not found"
    fi
}

check_pkg_config() {
    if pkg-config --exists "$1"; then
        echo "✓ $1 found ($(pkg-config --modversion "$1"))"
    else
        echo "✗ $1 not found"
    fi
}

check_command cmake
check_command make
check_pkg_config portaudio-2.0
check_pkg_config jsoncpp

# Проверяем libdatachannel особым образом
if pkg-config --exists datachannel 2>/dev/null || \
   find /usr/local /usr -name "libdatachannel*" 2>/dev/null | grep -q .; then
    echo "✓ libdatachannel found"
else
    echo "⚠ libdatachannel might not be properly installed"
fi

echo
echo "Installation verification complete!"
echo "If any dependencies are missing, please install them manually." 