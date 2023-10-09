#include "proto/rpc_client.hpp"

#include <mutex>
#include "lock.hpp"

namespace rpc_client {

void getCurrentWriteData(ClientContext& client_context, ClientToDaemonConnection& daemon_connection,
                         GetCurrentWriteDataRequest& req,
                         ResponseHandle<GetCurrentWriteDataReply>& resp_handle) {
    resp_handle.Init(req.dio_write_size);
    auto& reply = resp_handle.Get();

    memcpy(reply.data, req.dio_write_buf, req.dio_write_size);
}

void getPagePastAccessFreq(ClientContext& client_context,
                           ClientToDaemonConnection& daemon_connection,
                           GetPagePastAccessFreqRequest& req,
                           ResponseHandle<GetPagePastAccessFreqReply>& resp_handle) {
    page_id_t oldest_page = invalid_page_id;
    uint64_t last_time = UINT64_MAX;

    // m_page中存的是该CN访问过的所有Page的id
    client_context.m_tcache_mgr.foreach_all([&](PageThreadLocalCache& tcache) {
        for (auto& p : tcache.page_cache_table.table) {
            auto page_cache = tcache.page_cache_table.FindCache(p.second);
            if (page_cache == nullptr) {
                continue;
            }
            auto page_id = p.first;

            uint64_t last = page_cache->stats.last();
            if (last_time > last) {
                last_time = last;  // 越小，越旧
                oldest_page = page_id;
            }
            // DLOG("CN: %u getPagePastAccessFreq: find page %lu's cache.",
            //      client_context.m_client_id, page_id);
        }
        return true;
    });

    // DLOG("CN: %u getPagePastAccessFreq: finished.", client_context.m_client_id);
    resp_handle.Init();
    auto& reply = resp_handle.Get();
    reply.last_access_ts = last_time;
    reply.oldest_page_id = oldest_page;
}

void removePageCache(ClientContext& client_context, ClientToDaemonConnection& daemon_connection,
                     RemovePageCacheRequest& req,
                     ResponseHandle<RemovePageCacheReply>& resp_handle) {
    client_context.m_tcache_mgr.foreach_all([&](PageThreadLocalCache& tcache) {
        auto page_cache_meta = tcache.page_cache_table.FindOrCreateCacheMeta(req.page_id);
        auto page_cache = tcache.page_cache_table.FindCache(page_cache_meta);
        if (page_cache == nullptr) {
            return;
        }

        std::unique_lock<Mutex> cache_lock(page_cache_meta->ref_lock);

        tcache.page_cache_table.RemoveCache(page_cache_meta);
    });

    // DLOG("CN %u: Del page %lu cache.", client_context.m_client_id, req.page_id);

    resp_handle.Init();
    auto& reply = resp_handle.Get();
    reply.ret = true;
}

}  // namespace rpc_client
