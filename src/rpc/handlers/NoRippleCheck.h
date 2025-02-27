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

#pragma once

#include <data/BackendInterface.h>
#include <rpc/RPCHelpers.h>
#include <rpc/common/JsonBool.h>
#include <rpc/common/MetaProcessors.h>
#include <rpc/common/Modifiers.h>
#include <rpc/common/Types.h>
#include <rpc/common/Validators.h>

#include <set>

namespace rpc {

/**
 * @brief The noripple_check command provides a quick way to check the status of the Default Ripple field for an account
 * and the No Ripple flag of its trust lines, compared with the recommended settings.
 *
 * For more details see: https://xrpl.org/noripple_check.html
 */
class NoRippleCheckHandler {
    std::shared_ptr<BackendInterface> sharedPtrBackend_;

public:
    static auto constexpr LIMIT_MIN = 1;
    static auto constexpr LIMIT_MAX = 500;
    static auto constexpr LIMIT_DEFAULT = 300;

    struct Output {
        std::string ledgerHash;
        uint32_t ledgerIndex{};
        std::vector<std::string> problems;
        // TODO: use better type than json
        std::optional<boost::json::array> transactions;
        bool validated = true;
    };

    struct Input {
        std::string account;
        bool roleGateway = false;
        std::optional<std::string> ledgerHash;
        std::optional<uint32_t> ledgerIndex;
        uint32_t limit = LIMIT_DEFAULT;
        JsonBool transactions{false};
    };

    using Result = HandlerReturnType<Output>;

    NoRippleCheckHandler(std::shared_ptr<BackendInterface> const& sharedPtrBackend)
        : sharedPtrBackend_(sharedPtrBackend)
    {
    }

    static RpcSpecConstRef
    spec([[maybe_unused]] uint32_t apiVersion)
    {
        static auto const rpcSpecV1 = RpcSpec{
            {JS(account), validation::Required{}, validation::AccountValidator},
            {JS(role),
             validation::Required{},
             meta::WithCustomError{
                 validation::OneOf{"gateway", "user"},
                 Status{RippledError::rpcINVALID_PARAMS, "role field is invalid"}}},
            {JS(ledger_hash), validation::Uint256HexStringValidator},
            {JS(ledger_index), validation::LedgerIndexValidator},
            {JS(limit),
             validation::Type<uint32_t>(),
             validation::Min(1u),
             modifiers::Clamp<int32_t>{LIMIT_MIN, LIMIT_MAX}}};

        static auto const rpcSpec = RpcSpec{
            rpcSpecV1,
            {
                {JS(transactions), validation::Type<bool>()},
            }};

        return apiVersion == 1 ? rpcSpecV1 : rpcSpec;
    }

    Result
    process(Input input, Context const& ctx) const;

private:
    friend void
    tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Output const& output);

    friend Input
    tag_invoke(boost::json::value_to_tag<Input>, boost::json::value const& jv);
};
}  // namespace rpc
