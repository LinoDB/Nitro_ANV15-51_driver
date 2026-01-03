_pkgbase=nitro_anv15_51
pkgname=${_pkgbase}-dkms
pkgver=1
pkgrel=1
pkgdesc="DKMS package to automatically keep the Nitro ANV15-51 module updated"
arch=('x86_64')
license=('GPL3')
depends=('dkms')
source=(
    "misc.c"
    "misc.h"
    "nitro_anv15_51_module.c"
    "nitro_anv15_51_module.h"
    "nitro_battery_control.c"
    "nitro_battery_control.h"
    "dkms.conf"
    "Makefile"
)
sha256sums=(
    'defd68919b030a643c10747c3742a0cfbe5a7dffe5f385e79f2d2f2e88b79c8c'
    '783bf0b6304375efc9fb022fd8f18354375b5aeb9553d5614734a497e9e12b65'
    'b30e3e93b80e253a3e3c5880e5d6931522f92a8e8d7ba86315c96e976a3726fe'
    '20d3c35b79ebb864a95cf99885dfcea90a87a8522139e190d4d56669f3e8a5ed'
    'a3d266825dd41e91a4c33caad6ea293b17161668db0867d7654d0884b59eb40f'
    '7aa82ab8dd3268b6aa8bd8106190290971b499e1084c81da480704e71cc55dfb'
    'a221b643fed6eadbd259e87f72b68e213724fc32e865296d2b285bac05c4619f'
    '05767fa2be8ded6710a7b09512e5fee02c1e23b7fa14de0e6a3a764b767e1fcf'
)
package() {
    install -Dm644 dkms.conf "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/dkms.conf
	sed -e "s/@_PKGBASE@/${_pkgbase}/" \
      -e "s/@PKGVER@/${pkgver}/" \
      -i "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/dkms.conf
    cp *.c *.h Makefile "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/
}
