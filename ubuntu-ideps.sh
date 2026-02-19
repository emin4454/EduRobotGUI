

set -euo pipefail

echo


sudo apt update

PKGS=(
    build-essential     
    cmake               
    pkg-config          
    libwebsockets-dev      
    portaudio19-dev
    sdl2
    sdl2_image
    libssl-dev
    libmariadb-dev
)

echo ">> Aşağıdaki paketler kuruluyor: ${PKGS[*]}"
sudo apt install -y "${PKGS[@]}"

echo
echo ">> Ubuntu/Debian bağımlılıkları başarıyla yüklendi."
