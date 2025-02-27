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

#include <etl/SystemState.h>
#include <util/Profiler.h>
#include <util/log/Logger.h>

#include <ripple/beast/core/CurrentThreadName.h>

#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

namespace etl::detail {

/**
 * @brief Extractor thread that is fetching GRPC data and enqueue it on the DataPipeType
 */
template <typename DataPipeType, typename NetworkValidatedLedgersType, typename LedgerFetcherType>
class Extractor {
    util::Logger log_{"ETL"};

    std::reference_wrapper<DataPipeType> pipe_;
    std::shared_ptr<NetworkValidatedLedgersType> networkValidatedLedgers_;
    std::reference_wrapper<LedgerFetcherType> ledgerFetcher_;
    uint32_t startSequence_;
    std::optional<uint32_t> finishSequence_;
    std::reference_wrapper<SystemState const> state_;  // shared state for ETL

    std::thread thread_;

public:
    Extractor(
        DataPipeType& pipe,
        std::shared_ptr<NetworkValidatedLedgersType> networkValidatedLedgers,
        LedgerFetcherType& ledgerFetcher,
        uint32_t startSequence,
        std::optional<uint32_t> finishSequence,
        SystemState const& state
    )
        : pipe_(std::ref(pipe))
        , networkValidatedLedgers_{std::move(networkValidatedLedgers)}
        , ledgerFetcher_{std::ref(ledgerFetcher)}
        , startSequence_{startSequence}
        , finishSequence_{finishSequence}
        , state_{std::cref(state)}
    {
        thread_ = std::thread([this]() { process(); });
    }

    ~Extractor()
    {
        if (thread_.joinable())
            thread_.join();
    }

    void
    waitTillFinished()
    {
        assert(thread_.joinable());
        thread_.join();
    }

private:
    void
    process()
    {
        beast::setCurrentThreadName("ETLService extract");

        double totalTime = 0.0;
        auto currentSequence = startSequence_;

        while (!shouldFinish(currentSequence) && networkValidatedLedgers_->waitUntilValidatedByNetwork(currentSequence)
        ) {
            auto [fetchResponse, time] = ::util::timed<std::chrono::duration<double>>([this, currentSequence]() {
                return ledgerFetcher_.get().fetchDataAndDiff(currentSequence);
            });
            totalTime += time;

            // if the fetch is unsuccessful, stop. fetchLedger only returns false if the server is shutting down, or
            // if the ledger was found in the database (which means another process already wrote the ledger that
            // this process was trying to extract; this is a form of a write conflict). Otherwise, fetchDataAndDiff
            // will keep trying to fetch the specified ledger until successful.
            if (!fetchResponse)
                break;

            // TODO: extract this part into a strategy perhaps
            auto const tps = fetchResponse->transactions_list().transactions_size() / time;
            LOG(log_.info()) << "Extract phase time = " << time << "; Extract phase tps = " << tps
                             << "; Avg extract time = " << totalTime / (currentSequence - startSequence_ + 1)
                             << "; seq = " << currentSequence;

            pipe_.get().push(currentSequence, std::move(fetchResponse));
            currentSequence += pipe_.get().getStride();
        }

        pipe_.get().finish(startSequence_);
    }

    bool
    isStopping() const
    {
        return state_.get().isStopping;
    }

    bool
    hasWriteConflict() const
    {
        return state_.get().writeConflict;
    }

    bool
    shouldFinish(uint32_t seq) const
    {
        // Stopping conditions:
        // - if there is a write conflict in the load thread, the ETL mechanism should stop.
        // - if the entire server is shutting down - this can be detected in a variety of ways.
        // - when the given sequence is past the finishSequence in case one is specified
        return hasWriteConflict() || isStopping() || (finishSequence_ && seq > *finishSequence_);
    }
};

}  // namespace etl::detail
