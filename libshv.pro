TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    3rdparty \
    libshvchainpack \
    libshvcore \
    libshvcoreqt \
    libshviotqt \
    libshvbroker \
    utils \

qtHaveModule(gui) {
SUBDIRS += \
    libshvvisu \
}

SUBDIRS += \
    samples \

CONFIG(debug, debug|release) {
SUBDIRS += \
    tests \
}
