#include <panda/websocket/Parser.h>
#include <iostream>

namespace panda { namespace websocket {

using std::cout;
using std::endl;

Parser::Parser () : established(false), max_buffers_size(0), max_message_size(0), bufs_bytes(0) {
    cout << "Parser[ctor]\n";
}

bool Parser::add_buffer (const string& buf) {
    //cout << "Parser[add_buffer]: " << buf << endl;
    if (max_buffers_size && (bufs_bytes + buf.length() > max_buffers_size)) return false;
    if (buf.length()) {
        bufs.push_back(buf);
        bufs_bytes += buf.length();
    }
    return true;
}

void Parser::reset () {
    bufs.clear();
    bufs_bytes = 0;
    established = false;
}

Parser::~Parser () {
    cout << "~Parser\n";
}

}}
