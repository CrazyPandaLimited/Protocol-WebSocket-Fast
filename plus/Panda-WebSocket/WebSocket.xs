#include <panda/websocket.h>
#include <xs/websocket.h>
#include <iostream>

using namespace panda::websocket;
using namespace xs::websocket;
using panda::event::Loop;

using std::cout;

#define PWS_TRY(code)                                     \
    std::cout << "trying code\n"; \
    panda::websocket::Error abc("sdsd"); \
    abc = Error("dsfdsf"); \
    try { code; }                                         \
    catch (const Error& err) { cout << "HERE1\n"; croak_sv(error_sv(err, true)); } \
    catch (const std::exception& err) {                         \
    cout << "HERE2\n";\
        Error myerr(err.what());                          \
        croak_sv(error_sv(myerr, true));                  \
    } \
    catch (...) { \
        cout << "suka\n"; \
    } 

MODULE = Panda::WebSocket                PACKAGE = Panda::WebSocket
PROTOTYPES: DISABLE

INCLUDE: Server.xsi

MODULE = Panda::WebSocket                PACKAGE = Panda::WebSocket
PROTOTYPES: DISABLE