/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2019-07-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#define MXS_MODULE_NAME "cache"
#include "lrustoragemt.hh"

using maxscale::SpinLockGuard;

LRUStorageMT::LRUStorageMT(Storage* pstorage, uint64_t max_count, uint64_t max_size)
    : LRUStorage(pstorage, max_count, max_size)
{
    spinlock_init(&lock_);

    MXS_NOTICE("Created multi threaded LRU storage.");
}

LRUStorageMT::~LRUStorageMT()
{
}

LRUStorageMT* LRUStorageMT::create(Storage* pstorage, uint64_t max_count, uint64_t max_size)
{
    LRUStorageMT* plru_storage = NULL;

    MXS_EXCEPTION_GUARD(plru_storage = new LRUStorageMT(pstorage, max_count, max_size));

    return plru_storage;
}

cache_result_t LRUStorageMT::get_info(uint32_t what,
                                      json_t** ppInfo) const
{
    SpinLockGuard guard(lock_);

    return LRUStorage::do_get_info(what, ppInfo);
}

cache_result_t LRUStorageMT::get_value(const CACHE_KEY& key,
                                       uint32_t flags,
                                       GWBUF** ppvalue)
{
    SpinLockGuard guard(lock_);

    return do_get_value(key, flags, ppvalue);
}

cache_result_t LRUStorageMT::put_value(const CACHE_KEY& key,
                                       const GWBUF* pvalue)
{
    SpinLockGuard guard(lock_);

    return do_put_value(key, pvalue);
}

cache_result_t LRUStorageMT::del_value(const CACHE_KEY& key)
{
    SpinLockGuard guard(lock_);

    return do_del_value(key);
}

cache_result_t LRUStorageMT::get_head(CACHE_KEY* pKey,
                                      GWBUF** ppValue)
{
    SpinLockGuard guard(lock_);

    return LRUStorage::do_get_head(pKey, ppValue);
}

cache_result_t LRUStorageMT::get_tail(CACHE_KEY* pKey,
                                      GWBUF** ppValue)
{
    SpinLockGuard guard(lock_);

    return LRUStorage::do_get_tail(pKey, ppValue);
}

cache_result_t LRUStorageMT::get_size(uint64_t* pSize) const
{
    SpinLockGuard guard(lock_);

    return LRUStorage::do_get_size(pSize);
}

cache_result_t LRUStorageMT::get_items(uint64_t* pItems) const
{
    SpinLockGuard guard(lock_);

    return LRUStorage::do_get_items(pItems);
}