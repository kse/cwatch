# Maintainer: Kasper Sacharias Roos Eenberg <kse@mahavira.dk>

pkgname=cwatch-git
pkgrel=1
pkgver=20121210
arch=('x86_64' 'i686')
license=('MIT')
makedepends=('git')
provides=('cwatch')
url="http://github.com/kse/cwatch"
pkgdesc='Inotify watcher to execute command on file change'
source=()

_gitroot="git://github.com/kse/cwatch.git"
_gitname="build"

build() {
  cd "$srcdir/.."
  #msg "Connecting to GIT server..."
  #if [ -d $_gitname ] ; then
  #  cd $_gitname && git pull origin
  #  msg "The local files are updated."
  #else
  #  git clone --depth=1 $_gitroot $_gitname
  #  cd $_gitname
  #fi
  #msg "GIT checkout done or server timeout"

  rm -rf "$srcdir"
  git clone "$startdir" "$srcdir"
  cd "$srcdir"

  make
}

package() {
  cd "$srcdir"
  make DESTDIR="$pkgdir/" install
}

pkgver() {
  cd "$srcdir"
  echo $(git describe --always | sed 's/-/./g')
}

# vim:set ts=2 sw=2 et:
