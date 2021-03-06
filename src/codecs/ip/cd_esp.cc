/*
** Copyright (C) 2002-2013 Sourcefire, Inc.
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
// cd_esp.cc author Josh Rosenbaum <jrosenba@cisco.com>



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "framework/codec.h"
#include "snort.h"
#include "codecs/codec_events.h"
#include "protocols/protocol_ids.h"
#include "codecs/ip/ip_util.h"

#define CD_ESP_NAME "esp"
#define CD_ESP_HELP "support for encapsulating security payload"

namespace
{

static const RuleMap esp_rules[] =
{
    { DECODE_ESP_HEADER_TRUNC, "truncated encapsulated security payload header" },
    { 0, nullptr }
};


static const Parameter esp_params[] =
{
    { "decode_esp", Parameter::PT_BOOL, nullptr, "false",
      "enable for inspection of esp traffic that has authentication but not encryption" },

    { nullptr, Parameter::PT_MAX, nullptr, nullptr, nullptr }
};

class EspModule : public CodecModule
{
public:
    EspModule() : CodecModule(CD_ESP_NAME, CD_ESP_HELP, esp_params) {}

    const RuleMap* get_rules() const override
    { return esp_rules; }


    bool set(const char*, Value& v, SnortConfig* sc) override
    {
        if ( v.is("decode_esp") )
            sc->enable_esp = v.get_bool();
        else
            return false;

        return true;
    }
};


class EspCodec : public Codec
{
public:
    EspCodec() : Codec(CD_ESP_NAME){};
    ~EspCodec(){};


    void get_protocol_ids(std::vector<uint16_t>& v) override;
    bool decode(const RawData&, CodecData&, DecodeData&) override;
};


/* ESP constants */
constexpr uint32_t ESP_HEADER_LEN = 8;
constexpr uint32_t ESP_AUTH_DATA_LEN = 12;
constexpr uint32_t ESP_TRAILER_LEN = 2;

} // anonymous namespace



void EspCodec::get_protocol_ids(std::vector<uint16_t>& v)
{ v.push_back(IPPROTO_ID_ESP); }



/*
 * Function: DecodeESP(const uint8_t *, uint32_t, Packet *)
 *
 * Purpose: Attempt to decode Encapsulated Security Payload.
 *          The contents are probably encrypted, but ESP is sometimes used
 *          with "null" encryption, solely for Authentication.
 *          This is more of a heuristic -- there is no ESP field that specifies
 *          the encryption type (or lack thereof).
 *
 */
bool EspCodec::decode(const RawData& raw, CodecData& codec, DecodeData& snort)
{
    const uint8_t *esp_payload;
    uint8_t pad_length;

    if (!ScESPDecoding())
        return false;
    

    /* The ESP header contains a crypto Initialization Vector (IV) and
       a sequence number. Skip these. */
    if (raw.len < (ESP_HEADER_LEN + ESP_AUTH_DATA_LEN + ESP_TRAILER_LEN))
    {
        /* Truncated ESP traffic. Bail out here and inspect the rest as payload. */
        codec_events::decoder_event(codec, DECODE_ESP_HEADER_TRUNC);
        return false;
    }

    /* The Authentication Data at the end of the packet is variable-length.
       RFC 2406 says that Encryption and Authentication algorithms MUST NOT
       both be NULL, so we assume NULL Encryption and some other Authentication.

       The mandatory algorithms for Authentication are HMAC-MD5-96 and
       HMAC-SHA-1-96, so we assume a 12-byte authentication data at the end. */
    uint32_t guessed_len = raw.len - (ESP_HEADER_LEN + ESP_AUTH_DATA_LEN + ESP_TRAILER_LEN);

    codec.lyr_len = ESP_HEADER_LEN;
    esp_payload = raw.data + ESP_HEADER_LEN;
    pad_length = *(esp_payload + guessed_len);
    codec.next_prot_id = *(esp_payload + guessed_len + 1);


    // must be called AFTER setting next_prot_id
    if (snort.ip_api.is_ip6())
    {
        if ( snort_conf->hit_ip6_maxopts(codec.ip6_extension_count) )
        {
            codec_events::decoder_event(codec, DECODE_IP6_EXCESS_EXT_HDR);
            return false;
        }


        ip_util::CheckIPv6ExtensionOrder(codec, IPPROTO_ID_ESP);
        codec.proto_bits |= PROTO_BIT__IP6_EXT;
        codec.ip6_csum_proto = codec.next_prot_id;
        codec.ip6_extension_count++;
    }


    // TODO:  Leftover from Snort. Do we really want thsi?
    const_cast<uint32_t&>(raw.len) -= (ESP_AUTH_DATA_LEN + ESP_TRAILER_LEN);

    /* Adjust the packet length to account for the padding.
       If the padding length is too big, this is probably encrypted traffic. */
    if (pad_length < raw.len)
    {
        const_cast<uint32_t&>(raw.len) -= pad_length;
    }
    else
    {
        snort.decode_flags |= DECODE_PKT_TRUST;
        codec.lyr_len = ESP_HEADER_LEN;  // we want data to begin at (pkt + ESP_HEADER_LEN)
        codec.next_prot_id = FINISHED_DECODE;
        return true;
    }


    /* Attempt to decode the inner payload.
       There is a small chance that an encrypted next_header would become a
       different valid next_header. The DECODE_UNSURE_ENCAP flag tells the next
       decoder stage to silently ignore invalid headers. */

    // highest valid protocol id == 255.
    if (codec.next_prot_id > 0xFF)
    {
        codec.next_prot_id = FINISHED_DECODE;
        snort.decode_flags |= DECODE_PKT_TRUST;
    }
    else
    {
        codec.codec_flags |= CODEC_ENCAP_LAYER;
    }

    return true;
}


//-------------------------------------------------------------------------
// api
//-------------------------------------------------------------------------

static Module* mod_ctor()
{ return new EspModule; }

static void mod_dtor(Module* m)
{ delete m; }

static Codec* ctor(Module*)
{ return new EspCodec(); }

static void dtor(Codec *cd)
{ delete cd; }

static const CodecApi esp_api =
{
    {
        PT_CODEC,
        CD_ESP_NAME,
        CD_ESP_HELP,
        CDAPI_PLUGIN_V0,
        0,
        mod_ctor,
        mod_dtor,
    },
    nullptr, // pinit
    nullptr, // pterm
    nullptr, // tinit
    nullptr, // tterm
    ctor, // ctor
    dtor, // dtor
};


#ifdef BUILDING_SO
SO_PUBLIC const BaseApi* snort_plugins[] =
{
    &esp_api.base,
    nullptr
};
#else
const BaseApi* cd_esp = &esp_api.base;
#endif
