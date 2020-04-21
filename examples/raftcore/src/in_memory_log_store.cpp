//
// Created by Hython on 2020/4/21.
//

#include "in_memory_log_store.h"


namespace cornerstone {

    inmem_log_store::inmem_log_store()
            : start_idx_(1)
    {
        // Dummy entry for index 0.
        bufptr buf = buffer::alloc(sz_ulong);
        logs_[0] = cs_new<log_entry>(0, std::move(buf));
    }

    inmem_log_store::~inmem_log_store() {}

    ptr<log_entry> inmem_log_store::make_clone(const ptr<log_entry>& entry) {
        ptr<log_entry> clone = cs_new<log_entry>
                ( entry->get_term(),
                  buffer::copy( entry->get_buf() ),
                  entry->get_val_type() );
        return clone;
    }

    ulong inmem_log_store::next_slot() const {
        std::lock_guard<std::mutex> l(logs_lock_);
        // Exclude the dummy entry.
        return start_idx_ + logs_.size() - 1;
    }

    ulong inmem_log_store::start_index() const {
        return start_idx_;
    }

    ptr<log_entry> inmem_log_store::last_entry() const {
        ulong next_idx = next_slot();
        std::lock_guard<std::mutex> l(logs_lock_);
        auto entry = logs_.find( next_idx - 1 );
        if (entry == logs_.end()) {
            entry = logs_.find(0);
        }

        return make_clone(entry->second);
    }

    ulong inmem_log_store::append(ptr<log_entry>& entry) {
        ptr<log_entry> clone = make_clone(entry);

        std::lock_guard<std::mutex> l(logs_lock_);
        size_t idx = start_idx_ + logs_.size() - 1;
        logs_[idx] = clone;
        return idx;
    }

    void inmem_log_store::write_at(ulong index, ptr<log_entry>& entry) {
        ptr<log_entry> clone = make_clone(entry);

        // Discard all logs equal to or greater than `index.
        std::lock_guard<std::mutex> l(logs_lock_);
        auto itr = logs_.lower_bound(index);
        while (itr != logs_.end()) {
            itr = logs_.erase(itr);
        }
        logs_[index] = clone;
    }

    ptr< std::vector< ptr<log_entry> > >
    inmem_log_store::log_entries(ulong start, ulong end)
    {
        ptr< std::vector< ptr<log_entry> > > ret =
                cs_new< std::vector< ptr<log_entry> > >();

        ret->resize(end - start);
        ulong cc=0;
        for (ulong ii = start ; ii < end ; ++ii) {
            ptr<log_entry> src = nullptr;
            {   std::lock_guard<std::mutex> l(logs_lock_);
                auto entry = logs_.find(ii);
                if (entry == logs_.end()) {
                    entry = logs_.find(0);
                    assert(0);
                }
                src = entry->second;
            }
            (*ret)[cc++] = make_clone(src);
        }
        return ret;
    }

    ptr<std::vector<ptr<log_entry>>>
    inmem_log_store::log_entries_ext(ulong start,
                                     ulong end,
                                     ulong batch_size_hint_in_bytes)
    {
        ptr< std::vector< ptr<log_entry> > > ret =
                cs_new< std::vector< ptr<log_entry> > >();

        size_t accum_size = 0;
        for (ulong ii = start ; ii < end ; ++ii) {
            ptr<log_entry> src = nullptr;
            {   std::lock_guard<std::mutex> l(logs_lock_);
                auto entry = logs_.find(ii);
                if (entry == logs_.end()) {
                    entry = logs_.find(0);
                    assert(0);
                }
                src = entry->second;
            }
            ret->push_back(make_clone(src));
            accum_size += src->get_buf().size();
            if (batch_size_hint_in_bytes &&
                accum_size >= batch_size_hint_in_bytes) break;
        }
        return ret;
    }

    ptr<log_entry> inmem_log_store::entry_at(ulong index) {
        ptr<log_entry> src = nullptr;
        {   std::lock_guard<std::mutex> l(logs_lock_);
            auto entry = logs_.find(index);
            if (entry == logs_.end()) {
                entry = logs_.find(0);
            }
            src = entry->second;
        }
        return make_clone(src);
    }

    ulong inmem_log_store::term_at(ulong index) {
        ulong term = 0;
        {   std::lock_guard<std::mutex> l(logs_lock_);
            auto entry = logs_.find(index);
            if (entry == logs_.end()) {
                entry = logs_.find(0);
            }
            term = entry->second->get_term();
        }
        return term;
    }

    bufptr inmem_log_store::pack(ulong index, int32 cnt) {
        std::vector< bufptr > logs;

        size_t size_total = 0;
        for (ulong ii=index; ii<index+cnt; ++ii) {
            ptr<log_entry> le = nullptr;
            {   std::lock_guard<std::mutex> l(logs_lock_);
                le = logs_[ii];
            }
            assert(le.get());
            bufptr buf = le->serialize();
            size_total += buf->size();
            logs.push_back( std::move(buf) );
        }

        bufptr buf_out = buffer::alloc
                ( sizeof(int32) +
                  cnt * sizeof(int32) +
                  size_total );
        buf_out->pos(0);
        buf_out->put((int32)cnt);

        for (auto& entry: logs) {
            bufptr& bb = entry;
            buf_out->put((int32)bb->size());
            buf_out->put(*bb);
        }
        return buf_out;
    }

    void inmem_log_store::apply_pack(ulong index, buffer& pack) {
        pack.pos(0);
        int32 num_logs = pack.get_int();

        for (int32 ii=0; ii<num_logs; ++ii) {
            ulong cur_idx = index + ii;
            int32 buf_size = pack.get_int();

            bufptr buf_local = buffer::alloc(buf_size);
            pack.get(buf_local);

            ptr<log_entry> le = log_entry::deserialize(*buf_local);
            {   std::lock_guard<std::mutex> l(logs_lock_);
                logs_[cur_idx] = le;
            }
        }

        {   std::lock_guard<std::mutex> l(logs_lock_);
            auto entry = logs_.upper_bound(0);
            if (entry != logs_.end()) {
                start_idx_ = entry->first;
            } else {
                start_idx_ = 1;
            }
        }
    }

    bool inmem_log_store::compact(ulong last_log_index) {
        std::lock_guard<std::mutex> l(logs_lock_);
        for (ulong ii = start_idx_; ii <= last_log_index; ++ii) {
            auto entry = logs_.find(ii);
            if (entry != logs_.end()) {
                logs_.erase(entry);
            }
        }

        // WARNING:
        //   Even though nothing has been erased,
        //   we should set `start_idx_` to new index.
        start_idx_ = last_log_index + 1;
        return true;
    }

    void inmem_log_store::close() {}

}
    
