#pragma once
#include <algorithm_perlsafe>
#include <xs/xs.h>
#include <panda/websocket.h>

namespace xs { namespace websocket {

using namespace panda::websocket;

void av_to_header_values (pTHX_ AV* av, HTTPPacket::HeaderValues* vals);
AV*  header_values_to_av (pTHX_ const HTTPPacket::HeaderValues& vals);

void http_packet_set_headers (pTHX_ HTTPPacket* p, HV* headers);
void http_packet_set_body    (pTHX_ HTTPPacket* p, SV* body);

}}
