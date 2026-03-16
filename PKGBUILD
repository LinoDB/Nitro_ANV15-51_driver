_pkgbase=nitro_anv15_51
pkgname=${_pkgbase}-dkms
pkgver=2
pkgrel=1
pkgdesc="DKMS package to automatically keep the Nitro ANV15-51 module updated"
arch=('x86_64')
license=('GPL3')
depends=('dkms')
source=(
    "Makefile"
    "dkms.conf"
    "misc.c"
    "misc.h"
    "nitro_anv15_51_module.c"
    "nitro_anv15_51_module.h"
    "nitro_battery_control.c"
    "nitro_battery_control.h"
    "nitro_power_profile.c"
    "nitro_power_profile.h"
)
sha256sums=(
    'ce85393c112070bcd8c455edb1dfed87695b056186d4d6d29ad4f65e10d14a48'
    '9da212125ec90af239371690cdc904d6dff6831cac1d09cc214e717d527a6325'
    '97b7fe515348c9a883bd825aa42bb966cd861bd5a86bacc5ef588b39470c3e77'
    '74a880a1b50dd5e841b55815b9e6a7d32a068d4ea6eb3635702bd42bc2128016'
    '91faaabd1ec7774e4dbde70cf6f211b056baa3b7152e6d8196751487db6331c8'
    '20d3c35b79ebb864a95cf99885dfcea90a87a8522139e190d4d56669f3e8a5ed'
    '492e7f1fe0d3746d866454702f6672aa2234c61eef8b2508e54129f3111d209b'
    '90a76b98023b8f3b27e61850ee366b5e9d37b7b974a045a070d8d82e734f00e0'
    '5b32990fd2509ffd43c7f5b8d7979258fa191deaee8dd32c936322005cb46907'
    '23cfa74100b2c66ff393256c260bd4e7d1189aacb02475463bf64b06e282f4e8'
)
package() {
    install -Dm644 dkms.conf "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/dkms.conf
	sed -e "s/@_PKGBASE@/${_pkgbase}/" \
      -e "s/@PKGVER@/${pkgver}/" \
      -i "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/dkms.conf
    cp *.c *.h Makefile "${pkgdir}"/usr/src/${_pkgbase}-${pkgver}/
}
