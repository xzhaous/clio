//------------------------------------------------------------------------------
/*
    This file is part of clio: https://github.com/XRPLF/clio
    Copyright (c) 2023, the clio developers.

    Permission to use, copy, modify, and distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL,  DIRECT,  INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <rpc/RPCHelpers.h>
#include <rpc/handlers/Random.h>

#include <ripple/beast/utility/rngfill.h>
#include <ripple/crypto/csprng.h>

namespace rpc {

RandomHandler::Result
RandomHandler::process([[maybe_unused]] Context const& ctx)
{
    ripple::uint256 rand;
    beast::rngfill(rand.begin(), ripple::uint256::size(), ripple::crypto_prng());

    return Output{ripple::strHex(rand)};
}

void
tag_invoke(boost::json::value_from_tag, boost::json::value& jv, RandomHandler::Output const& output)
{
    jv = {
        {JS(random), output.random},
    };
}

}  // namespace rpc
