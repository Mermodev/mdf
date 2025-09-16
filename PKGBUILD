# Maintainer: Mermod <mermodev@gmail.com>
pkgname=mdf
pkgver=1.0.0
pkgrel=1
pkgdesc="Mermodev File Manager - A terminal-based file manager with tagging system"
arch=('x86_64')
url="https://github.com/Mermod/mdf"
license=('MIT')
depends=('gcc-libs' 'filesystem' 'nvim')
makedepends=('git')
source=("$pkgname::git+https://github.com/Mermod/mdf.git")
sha256sums=('SKIP')

build() {
  cd "$srcdir/$pkgname"
  g++ -std=c++17 -o mdf mdf.cpp
}

package() {
  cd "$srcdir/$pkgname"
  install -Dm755 mdf "$pkgdir/usr/bin/mdf"
  install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
}
