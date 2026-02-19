
set -euo pipefail

echo

sudo pacman -Syu --noconfirm

PKGS=(
    base-devel          
    cmake               
    libwebsockets                
    portaudio
    sdl2
    sdl2_image
    sdl2_mixer
    mariadb-libs            
)

echo ">> Aşağıdaki paketler kuruluyor: ${PKGS[*]}"
sudo pacman -S --noconfirm "${PKGS[@]}"

echo
echo ">> Arch Linux bağımlılıkları başarıyla yüklendi."
