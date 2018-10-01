#pragma once
#include <panda/uri.h>
#include <panda/refcnt.h>
#include <panda/websocket/HTTPRequest.h>

namespace panda { namespace websocket {

static const int supported_ws_versions[] = {13};

class ConnectRequest : public HTTPRequest {
public:
    string       ws_key;
    int          ws_version;
    string       ws_protocol;

    ConnectRequest () : ws_version(0), _ws_version_supported(true) {
        max_body_size = -1;
    }

    const HeaderValues& ws_extensions        () const { return _ws_extensions; }
    bool                ws_version_supported () const { return _ws_version_supported; }

    void ws_extensions (const HeaderValues& new_extensions) { _ws_extensions = new_extensions; }

protected:
    virtual void _parse_header (StringRange range);
    virtual void _to_string    (string& str);

private:
    HeaderValues _ws_extensions;
    bool         _ws_version_supported;
};

typedef panda::shared_ptr<ConnectRequest> ConnectRequestSP;

}}
