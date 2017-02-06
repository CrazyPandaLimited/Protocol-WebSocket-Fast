#pragma once
#include <panda/refcnt.h>
#include <panda/websocket/HTTPResponse.h>

namespace panda { namespace websocket {

class ConnectResponse : public HTTPResponse {
public:
    string ws_protocol;

    ConnectResponse () : _ws_extensions_set(false) {}

    const string& ws_accept_key () const { return _ws_accept_key; }
    const string& ws_versions   () const { return _ws_versions; }

    const HeaderValues& ws_extensions     () const { return _ws_extensions; }
    bool                ws_extensions_set () const { return _ws_extensions_set; }

    void ws_extensions (const HeaderValues& new_extensions) {
        _ws_extensions = new_extensions;
        _ws_extensions_set = true;
    }

    friend class ServerParser;
    friend class ClientParser;

protected:
    virtual void _parse_header (StringRange range);
    virtual void _to_string    (string& str);

private:
    string       _ws_key;
    HeaderValues _ws_extensions;
    bool         _ws_extensions_set;
    string       _ws_accept_key;
    string       _ws_versions;

    string _calc_accept_key (string ws_key);
};

typedef panda::shared_ptr<ConnectResponse> ConnectResponseSP;

}}
