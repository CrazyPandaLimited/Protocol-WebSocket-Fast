#include <xs/websocket/error.h>
#include <typeinfo>
#include <cxxabi.h>
using namespace panda::websocket;
using namespace xs::websocket;

#include <iostream>

SV* xs::websocket::error_sv (const Error& err, bool with_mess) {
    dTHX;
    std::cout << "hello suka\n";
    int status;
    char* class_name = abi::__cxa_demangle(typeid(err).name(), NULL, NULL, &status);
    if (status != 0) croak("[Panda::WebSocket] !critical! abi::__cxa_demangle error");
    SV* stash_name = newSVpvs("Panda::WebSocket::");
    if (strstr(class_name, "panda::websocket::") == class_name) sv_catpv(stash_name, class_name + 14);
    free(class_name);

    HV* stash = gv_stashsv(stash_name, 0);
    if (!stash) croak("[Panda::WebSocket] !critical! no package: %s", SvPVX(stash_name));
    GV* new_gv = gv_fetchmeth_pv(stash, "new", 0, 0);
    CV* new_cv = new_gv ? GvCV(new_gv) : NULL;
    if (!new_cv) croak("[Panda::WebSocket] !critical! no 'new' method in package: %s", SvPVX(stash_name));

    const string what = err.what();
    HV* args = newHV();
    hv_stores(args, "what", newSVpvn(what.data(), what.size()));
    if (with_mess) {
        SV* svmess = mess("");
        SvREFCNT_inc(svmess);
        hv_stores(args, "mess", svmess);
    }

    dSP; ENTER; SAVETMPS;
    PUSHMARK(SP);
    EXTEND(SP, 2);
    mPUSHs(stash_name);
    mPUSHs(newRV_noinc((SV*)args));
    PUTBACK;
    int items = call_sv((SV*)new_cv, G_SCALAR);
    SPAGAIN;
    SV* err_obj = NULL;
    while (items--) err_obj = POPs;
    if (!(err_obj && SvOK(err_obj) && sv_isobject(err_obj)))
        croak("[Panda::WebSocket] !critical! 'new' method in package %s returned non-object", SvPVX(stash_name));
    SvREFCNT_inc_simple_void_NN(err_obj);
    PUTBACK;
    FREETMPS; LEAVE;
    return err_obj;
}
