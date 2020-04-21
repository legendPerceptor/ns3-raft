//
// Created by Hython on 2020/4/21.
//

#ifndef NS3_IN_MEMORY_LOG_STORE_H
#define NS3_IN_MEMORY_LOG_STORE_H

#include "cornerstone.hxx"
#include <map>

namespace cornerstone {

    class inmem_log_store : public log_store {
    public:
        inmem_log_store();

        ~inmem_log_store();

    __nocopy__(inmem_log_store);

        ulong next_slot() const;

        ulong start_index() const;

        ptr<log_entry> last_entry() const;

        ulong append(ptr<log_entry> &entry);

        void write_at(ulong index, ptr<log_entry> &entry);

        ptr<std::vector<ptr<log_entry>>> log_entries(ulong start, ulong end);

        ptr<std::vector<ptr<log_entry>>> log_entries_ext(
                ulong start, ulong end, ulong batch_size_hint_in_bytes = 0);

        ptr<log_entry> entry_at(ulong index);

        ulong term_at(ulong index);

        bufptr pack(ulong index, int32 cnt);

        void apply_pack(ulong index, buffer &pack);

        bool compact(ulong last_log_index);

        bool flush() { return true; }

        void close();

    private:
        static ptr<log_entry> make_clone(const ptr<log_entry> &entry);

        std::map<ulong, ptr<log_entry>> logs_;
        mutable std::mutex logs_lock_;
        std::atomic<ulong> start_idx_;
    };

}

#endif //NS3_IN_MEMORY_LOG_STORE_H
