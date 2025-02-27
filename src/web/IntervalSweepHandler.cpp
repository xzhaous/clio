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

#include <web/IntervalSweepHandler.h>

#include <util/Constants.h>
#include <web/DOSGuard.h>

#include <algorithm>
#include <ctime>

namespace web {

IntervalSweepHandler::IntervalSweepHandler(util::Config const& config, boost::asio::io_context& ctx)
    : sweepInterval_{std::max(
          1u,
          static_cast<uint32_t>(
              config.valueOr("dos_guard.sweep_interval", 1.0) * static_cast<double>(util::MILLISECONDS_PER_SECOND)
          )
      )}
    , ctx_{std::ref(ctx)}
    , timer_{ctx.get_executor()}
{
}

IntervalSweepHandler::~IntervalSweepHandler()
{
    boost::asio::post(ctx_.get(), [this]() { timer_.cancel(); });
}

void
IntervalSweepHandler::setup(web::BaseDOSGuard* guard)
{
    assert(dosGuard_ == nullptr);
    dosGuard_ = guard;
    assert(dosGuard_ != nullptr);

    createTimer();
}

void
IntervalSweepHandler::createTimer()
{
    timer_.expires_after(sweepInterval_);
    timer_.async_wait([this](boost::system::error_code const& error) {
        if (error == boost::asio::error::operation_aborted)
            return;

        dosGuard_->clear();
        boost::asio::post(ctx_.get(), [this] { createTimer(); });
    });
}

}  // namespace web
