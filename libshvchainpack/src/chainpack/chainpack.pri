SOURCES += \
    $$PWD/datachange.cpp \
    $$PWD/rpc.cpp \
    $$PWD/rpcmessage.cpp \
    $$PWD/rpcvalue.cpp \
    $$PWD/rpcdriver.cpp \
    $$PWD/metatypes.cpp \
    $$PWD/exception.cpp \
    $$PWD/utils.cpp \
    $$PWD/abstractstreamreader.cpp \
    $$PWD/abstractstreamwriter.cpp \
    $$PWD/cponwriter.cpp \
    $$PWD/cponreader.cpp \
    $$PWD/chainpackwriter.cpp \
    $$PWD/cpon.cpp \
    $$PWD/chainpack.cpp \
    $$PWD/chainpackreader.cpp \
    $$PWD/chainpackreader1.cpp \
    $$PWD/metamethod.cpp \
    $$PWD/tunnelctl.cpp \
    $$PWD/irpcconnection.cpp \
    $$PWD/accessgrant.cpp \
    $$PWD/valuenotavailable.cpp

HEADERS += \
    $$PWD/datachange.h \
    $$PWD/rpc.h \
    $$PWD/rpcmessage.h \
    $$PWD/rpcvalue.h \
    $$PWD/rpcdriver.h \
    $$PWD/metatypes.h \
    $$PWD/exception.h \
    $$PWD/utils.h \
    $$PWD/abstractstreamreader.h \
    $$PWD/abstractstreamwriter.h \
    $$PWD/cponwriter.h \
    $$PWD/cponreader.h \
    $$PWD/chainpackwriter.h \
    $$PWD/cpon.h \
    $$PWD/chainpack.h \
    $$PWD/chainpackreader1.h \
    $$PWD/chainpackreader.h \
    $$PWD/metamethod.h \
    $$PWD/tunnelctl.h \
    $$PWD/irpcconnection.h \
    $$PWD/accessgrant.h \
    $$PWD/valuenotavailable.h

unix {
SOURCES += \
    $$PWD/socketrpcdriver.cpp \

HEADERS += \
    $$PWD/socketrpcdriver.h \
}
