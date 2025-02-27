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

#include <rpc/Errors.h>
#include <util/Expected.h>

#include <ripple/basics/base_uint.h>

#include <boost/asio/spawn.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>

namespace etl {
class LoadBalancer;
}  // namespace etl
namespace web {
struct ConnectionBase;
}  // namespace web
namespace feed {
class SubscriptionManager;
}  // namespace feed

namespace rpc {

class Counters;
struct RpcSpec;
struct FieldSpec;
class AnyHandler;

/**
 * @brief Return type used for Validators that can return error but don't have
 * specific value to return
 */
using MaybeError = util::Expected<void, Status>;

/**
 * @brief The type that represents just the error part of @ref MaybeError
 */
using Error = util::Unexpected<Status>;

/**
 * @brief Return type for each individual handler
 */
template <typename OutputType>
using HandlerReturnType = util::Expected<OutputType, Status>;

/**
 * @brief The final return type out of RPC engine
 */
using ReturnType = util::Expected<boost::json::value, Status>;

/**
 * @brief An alias for a const reference to @ref RpcSpec.
 */
using RpcSpecConstRef = RpcSpec const&;

/**
 * @brief An empty type used as Output for handlers than don't actually produce output.
 */
struct VoidOutput {};

/**
 * @brief Context of an RPC call.
 */
struct Context {
    boost::asio::yield_context yield;
    std::shared_ptr<web::ConnectionBase> session = {};
    bool isAdmin = false;
    std::string clientIp = {};
    uint32_t apiVersion = 0u;  // invalid by default
};

/**
 * @brief Result type used to return responses or error statuses to the Webserver subsystem.
 */
using Result = std::variant<Status, boost::json::object>;

/**
 * @brief A cursor object used to traverse nodes owned by an account.
 */
struct AccountCursor {
    ripple::uint256 index;
    std::uint32_t hint{};

    std::string
    toString() const
    {
        return ripple::strHex(index) + "," + std::to_string(hint);
    }

    bool
    isNonZero() const
    {
        return index.isNonZero() || hint != 0;
    }
};

/**
 * @brief Interface for the provider of RPC handlers.
 */
class HandlerProvider {
public:
    virtual ~HandlerProvider() = default;

    virtual bool
    contains(std::string const& method) const = 0;

    virtual std::optional<AnyHandler>
    getHandler(std::string const& command) const = 0;

    virtual bool
    isClioOnly(std::string const& command) const = 0;
};

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& jv, VoidOutput const&)
{
    jv = boost::json::object{};
}

}  // namespace rpc
