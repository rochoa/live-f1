require 'formula'
 
class LiveF1 < Formula
  homepage 'https://launchpad.net/live-f1'
  version '0.2.12'
  url 'https://github.com/rochoa/live-f1/archive/0.2.12.tar.gz'
  sha1 'de3e4f0ded19d67b0a12b4a15ec5fc0ef714798e'

  head "https://github.com/rochoa/live-f1.git"
 
  depends_on 'autoconf'   => :build
  depends_on 'automake'   => :build
  depends_on 'pkg-config' => :build
  depends_on 'gettext'
  depends_on 'neon'

  def install
    system "autoreconf -i"
    system "./configure", "--disable-debug", "--disable-dependency-tracking",
                          "--prefix=#{prefix}"
    system "make install"
  end
end