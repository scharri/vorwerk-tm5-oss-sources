DESCRIPTION = "WebSocket Library"
HOMEPAGE = "http://nohomepage.org"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://LICENSE;md5=041a1dec49ec8a22e7f101350fd19550"

DEPENDS = "zlib openssl"

SRC_URI = "https://libwebsockets.org/git/libwebsockets/snapshot/libwebsockets-1.5-chrome47-firefox41.tar.gz \
           file://websockets.patch"
SRC_URI[md5sum] = "2b48aa76f35354fc65160ec116bdd36d"
SRC_URI[sha256sum] = "27f3e2dbd04b8375923f6353536c741559a21dd4713f8c302b23441d6fe82403"

S="${WORKDIR}/libwebsockets-1.5-chrome47-firefox41"

# Customize cmake invocation to prevent to build test applications
EXTRA_OECMAKE = " \
-DLWS_WITHOUT_TESTAPPS=ON \
-DLWS_WITHOUT_TEST_SERVER=ON \
-DLWS_WITHOUT_TEST_SERVER_EXTPOLL=ON \
-DLWS_WITHOUT_TEST_PING=ON \
-DLWS_WITHOUT_TEST_CLIENT=ON \
-DLWS_WITHOUT_TEST_FRAGGLE=ON \
"

inherit cmake pkgconfig
BBCLASSEXTEND = "native nativesdk"
