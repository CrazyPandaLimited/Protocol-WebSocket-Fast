TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += src ../CPP-panda-lib/src

SOURCES += \
    src/panda/websocket/ClientParser.cc \
    src/panda/websocket/ConnectRequest.cc \
    src/panda/websocket/ConnectResponse.cc \
    src/panda/websocket/Frame.cc \
    src/panda/websocket/FrameHeader.cc \
    src/panda/websocket/HTTPPacket.cc \
    src/panda/websocket/HTTPRequest.cc \
    src/panda/websocket/HTTPResponse.cc \
    src/panda/websocket/Message.cc \
    src/panda/websocket/Parser.cc \
    src/panda/websocket/ServerParser.cc \
    src/panda/websocket/utils.cc \
    src/xs/websocket.cc

HEADERS += \
    src/panda/ranges/Joiner.h \
    src/panda/ranges/KmpFinder.h \
    src/panda/websocket/ClientParser.h \
    src/panda/websocket/ConnectRequest.h \
    src/panda/websocket/ConnectResponse.h \
    src/panda/websocket/Frame.h \
    src/panda/websocket/FrameHeader.h \
    src/panda/websocket/HTTPPacket.h \
    src/panda/websocket/HTTPRequest.h \
    src/panda/websocket/HTTPResponse.h \
    src/panda/websocket/inc.h \
    src/panda/websocket/iterator.h \
    src/panda/websocket/Message.h \
    src/panda/websocket/Parser.h \
    src/panda/websocket/ParserError.h \
    src/panda/websocket/ServerParser.h \
    src/panda/websocket/utils.h \
    src/panda/websocket.h \
    src/xs/websocket.h

DISTFILES += \
    ClientParser.xsi \
    ConnectRequest.xsi \
    ConnectResponse.xsi \
    Frame.xsi \
    FrameIterator.xsi \
    HTTPPacket.xsi \
    HTTPRequest.xsi \
    HTTPResponse.xsi \
    Message.xsi \
    MessageIterator.xsi \
    Parser.xsi \
    ServerParser.xsi \
    typemap \
    WebSocket.xs \
    t/00-Panda-WebSocket.t \
    t/ClientParser-connect.t \
    t/ClientParser-connect_request.t \
    t/Parser-get_frames.t \
    t/Parser-get_messages.t \
    t/Parser-get-mixed-mode.t \
    t/Parser-send_control.t \
    t/Parser-send_frame.t \
    t/Parser-send_message.t \
    t/ServerParser-accept.t \
    t/ServerParser-accept_error.t \
    t/ServerParser-accept_response.t \
    t/zleaks.t \
    Makefile.PL
