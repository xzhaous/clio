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

#include <atomic>

namespace etl {

/**
 * @brief Represents the state of the ETL subsystem.
 */
struct SystemState {
    /**
     * @brief Whether the process is in strict read-only mode.
     *
     * In strict read-only mode, the process will never attempt to become the ETL writer, and will only publish ledgers
     * as they are written to the database.
     */
    bool isReadOnly = false;

    std::atomic_bool isWriting = false;     /**< @brief Whether the process is writing to the database. */
    std::atomic_bool isStopping = false;    /**< @brief Whether the software is stopping. */
    std::atomic_bool writeConflict = false; /**< @brief Whether a write conflict was detected. */

    /**
     * @brief Whether clio detected an amendment block.
     *
     * Being amendment blocked means that Clio was compiled with libxrpl that does not yet support some field that
     * arrived from rippled and therefore can't extract the ledger diff. When this happens, Clio can't proceed with ETL
     * and should log this error and only handle RPC requests.
     */
    std::atomic_bool isAmendmentBlocked = false;
};

}  // namespace etl
