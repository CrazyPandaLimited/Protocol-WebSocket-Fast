#pragma once
#include <panda/websocket/HTTPResponse.h>

namespace panda { namespace websocket {

class ConnectResponse : public HTTPResponse {
public:
    string ws_protocol;

    ConnectResponse () : _ws_extensions_set(false) {}

    const HeaderValues& ws_extensions     () const { return _ws_extensions; }
    bool                ws_extensions_set () const { return _ws_extensions_set; }

    void ws_extensions (const HeaderValues& new_extensions) {
        _ws_extensions = new_extensions;
        _ws_extensions_set = true;
    }

    //virtual void _parse_header (StringRange range);

    friend class ServerParser;

protected:
    virtual void _to_string (string& str);

private:
    string       _ws_key;
    HeaderValues _ws_extensions;
    bool         _ws_extensions_set;
};

}}
