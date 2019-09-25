#pragma once
#include <panda/uri.h>
#include <panda/refcnt.h>
#include <panda/protocol/websocket/HTTPRequest.h>
#include <panda/protocol/websocket/DeflateExt.h>
#include <iostream>

namespace panda { namespace protocol { namespace websocket {

static const int supported_ws_versions[] = {13};

struct ConnectRequest : HTTPRequest {
    string       ws_key;
    int          ws_version;
    string       ws_protocol;

    string error;

    ConnectRequest () : ws_version(0), _ws_version_supported(true) {
        method = Request::Method::GET;
    }

    ~ConnectRequest();

    virtual void process_headers ();

    const HeaderValues& ws_extensions        () const { return _ws_extensions; }
    bool                ws_version_supported () const { return _ws_version_supported; }

    void ws_extensions (const HeaderValues& new_extensions) { _ws_extensions = new_extensions; }

    void add_deflate(const DeflateExt::Config& cfg);

    string to_string();

    http::ResponseSP create_response() const override;

protected:
//    virtual void _to_string    (string& str);

private:
    HeaderValues _ws_extensions;
    bool         _ws_version_supported;
};

using ConnectRequestSP = panda::iptr<ConnectRequest>;

}}}
