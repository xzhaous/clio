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

#include <data/BackendCounters.h>

#include <util/prometheus/Prometheus.h>

namespace data {

using namespace util::prometheus;

BackendCounters::BackendCounters()
    : tooBusyCounter_(PrometheusService::counterInt(
          "backend_too_busy_total_number",
          Labels(),
          "The total number of times the backend was too busy to process a request"
      ))
    , writeSyncCounter_(PrometheusService::counterInt(
          "backend_operations_total_number",
          Labels({Label{"operation", "write_sync"}}),
          "The total number of times the backend had to write synchronously"
      ))
    , writeSyncRetryCounter_(PrometheusService::counterInt(
          "backend_operations_total_number",
          Labels({Label{"operation", "write_sync_retry"}}),
          "The total number of times the backend had to retry a synchronous write"
      ))
    , asyncWriteCounters_{"write_async"}
    , asyncReadCounters_{"read_async"}
{
}

BackendCounters::PtrType
BackendCounters::make()
{
    struct EnableMakeShared : public BackendCounters {};
    return std::make_shared<EnableMakeShared>();
}

void
BackendCounters::registerTooBusy()
{
    ++tooBusyCounter_.get();
}

void
BackendCounters::registerWriteSync()
{
    ++writeSyncCounter_.get();
}

void
BackendCounters::registerWriteSyncRetry()
{
    ++writeSyncRetryCounter_.get();
}

void
BackendCounters::registerWriteStarted()
{
    asyncWriteCounters_.registerStarted(1u);
}

void
BackendCounters::registerWriteFinished()
{
    asyncWriteCounters_.registerFinished(1u);
}

void
BackendCounters::registerWriteRetry()
{
    asyncWriteCounters_.registerRetry(1u);
}

void
BackendCounters::registerReadStarted(std::uint64_t const count)
{
    asyncReadCounters_.registerStarted(count);
}

void
BackendCounters::registerReadFinished(std::uint64_t const count)
{
    asyncReadCounters_.registerFinished(count);
}

void
BackendCounters::registerReadRetry(std::uint64_t const count)
{
    asyncReadCounters_.registerRetry(count);
}

void
BackendCounters::registerReadError(std::uint64_t const count)
{
    asyncReadCounters_.registerError(count);
}

boost::json::object
BackendCounters::report() const
{
    boost::json::object result;
    result["too_busy"] = tooBusyCounter_.get().value();
    result["write_sync"] = writeSyncCounter_.get().value();
    result["write_sync_retry"] = writeSyncRetryCounter_.get().value();
    for (auto const& [key, value] : asyncWriteCounters_.report())
        result[key] = value;
    for (auto const& [key, value] : asyncReadCounters_.report())
        result[key] = value;
    return result;
}

BackendCounters::AsyncOperationCounters::AsyncOperationCounters(std::string name)
    : name_(std::move(name))
    , pendingCounter_(PrometheusService::gaugeInt(
          "backend_operations_current_number",
          Labels({{"operation", name_}, {"status", "pending"}}),
          "The current number of pending " + name_ + " operations"
      ))
    , completedCounter_(PrometheusService::counterInt(
          "backend_operations_total_number",
          Labels({{"operation", name_}, {"status", "completed"}}),
          "The total number of completed " + name_ + " operations"
      ))
    , retryCounter_(PrometheusService::counterInt(
          "backend_operations_total_number",
          Labels({{"operation", name_}, {"status", "retry"}}),
          "The total number of retried " + name_ + " operations"
      ))
    , errorCounter_(PrometheusService::counterInt(
          "backend_operations_total_number",
          Labels({{"operation", name_}, {"status", "error"}}),
          "The total number of errored " + name_ + " operations"
      ))
{
}

void
BackendCounters::AsyncOperationCounters::registerStarted(std::uint64_t const count)
{
    pendingCounter_.get() += count;
}

void
BackendCounters::AsyncOperationCounters::registerFinished(std::uint64_t const count)
{
    assert(pendingCounter_.get().value() >= static_cast<std::int64_t>(count));
    pendingCounter_.get() -= count;
    completedCounter_.get() += count;
}

void
BackendCounters::AsyncOperationCounters::registerRetry(std::uint64_t count)
{
    retryCounter_.get() += count;
}

void
BackendCounters::AsyncOperationCounters::registerError(std::uint64_t count)
{
    assert(pendingCounter_.get().value() >= static_cast<std::int64_t>(count));
    pendingCounter_.get() -= count;
    errorCounter_.get() += count;
}

boost::json::object
BackendCounters::AsyncOperationCounters::report() const
{
    return boost::json::object{
        {name_ + "_pending", pendingCounter_.get().value()},
        {name_ + "_completed", completedCounter_.get().value()},
        {name_ + "_retry", retryCounter_.get().value()},
        {name_ + "_error", errorCounter_.get().value()}};
}

}  // namespace data
