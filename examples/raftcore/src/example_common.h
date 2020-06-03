//
// Created by Hython on 2020/3/16.
//

#ifndef NS3_EXAMPLE_COMMON_H
#define NS3_EXAMPLE_COMMON_H

#include "cornerstone.hxx"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "in_memory_log_store.h"
#include "ns3/simulator.h"
namespace cornerstone {

    class fs_logger : public logger {
    public:
        fs_logger(const std::string &filename) : fs_(filename) {}

    __nocopy__(fs_logger);

    public:
        virtual void debug(const std::string &log_line) {
            fs_ << log_line << std::endl;
            fs_.flush();
        }

        virtual void info(const std::string &log_line) {
            fs_ << log_line << std::endl;
            fs_.flush();
        }

        virtual void warn(const std::string &log_line) {
            fs_ << log_line << std::endl;
            fs_.flush();
        }

        virtual void err(const std::string &log_line) {
            fs_ << log_line << std::endl;
            fs_.flush();
        }

    private:
        std::ofstream fs_;
    };




    class echo_state_machine : public state_machine {
    public:
        echo_state_machine() : lock_() {}

    public:
        virtual void commit(const ulong, buffer &data, const uptr <log_entry_cookie> &) {
            auto_lock(lock_);
            std::string tmp = reinterpret_cast<const char *>(data.data());
            std::cout << ns3::Simulator::Now()<<" -data size:"<<data.size()<<", commit message:" << tmp.substr(0,data.size()) << std::endl;
        }

        virtual void pre_commit(const ulong, buffer &data, const uptr <log_entry_cookie> &) {
            auto_lock(lock_);
            std::string tmp = reinterpret_cast<const char *>(data.data());
            std::cout << ns3::Simulator::Now()<<" -data size:"<<data.size()<<", pre-commit message:" << tmp.substr(0,data.size()) << std::endl;

        }

        virtual void rollback(const ulong, buffer &data, const uptr <log_entry_cookie> &) {
            auto_lock(lock_);
            std::cout << "rollback: " << reinterpret_cast<const char *>(data.data()) << std::endl;
        }

        virtual void save_snapshot_data(snapshot &, const ulong, buffer &) {}

        virtual bool apply_snapshot(snapshot &) {
            return false;
        }

        virtual int read_snapshot_data(snapshot &, const ulong, buffer &) {
            return 0;
        }

        virtual ulong last_commit_index() {
            return 0;
        }

        virtual ptr <snapshot> last_snapshot() {
            return ptr<snapshot>();
        }

        virtual void create_snapshot(snapshot &, async_result<bool>::handler_type &) {}

    private:
        std::mutex lock_;
    };


    class simple_state_mgr : public state_mgr {
    public:
        simple_state_mgr(int32 srv_id, const std::vector<ptr < srv_config>>

        & cluster)
        :

        srv_id_ (srv_id), cluster_(cluster) {
            store_path_ = sstrfmt("my store%d").fmt(srv_id_);
        }

    public:
        virtual ptr <cluster_config> load_config() {
            ptr<cluster_config> conf = cs_new<cluster_config>();
            for (const auto &srv : cluster_) {
                conf->get_servers().push_back(srv);
            }

            return conf;
        }

        virtual void save_config(const cluster_config &) {}

        virtual void save_state(const srv_state &) {}

        virtual ptr <srv_state> read_state() {
            return cs_new<srv_state>();
        }

        virtual ptr <log_store> load_log_store() {
            mkdir(store_path_.c_str(), 0766);
            return cs_new<fs_log_store>(store_path_);
            //return cs_new<inmem_log_store>();
        }

        virtual int32 server_id() {
            return srv_id_;
        }

        virtual void system_exit(const int exit_code) {
            std::cout << "system exiting with code " << exit_code << std::endl;
        }

    private:
        int32 srv_id_;
        std::vector<ptr < srv_config>> cluster_;
        std::string store_path_;
    };

    class test_event_listener: public raft_event_listener {
    public:
        test_event_listener(int id) : raft_event_listener(), srv_id_(id){}

    public:
        virtual void on_event(raft_event event) override {
            switch (event)
            {
                case raft_event::become_follower:
                    std::cout << srv_id_ << " becomes a follower" << std::endl;
                    break;
                case raft_event::become_leader:
                    std::cout << srv_id_ << " becomes a leader" << std::endl;
                    break;
                case raft_event::logs_catch_up:
                    std::cout << srv_id_ << " catch up all logs" << std::endl;
                    break;
            }
        }

    private:
        int srv_id_;
    };

    class ns3_event_listener: public raft_event_listener {
    public:
        ns3_event_listener(int id) : raft_event_listener(), srv_id_(id){}

        virtual void on_event(raft_event event) override;

    private:
        int srv_id_;
    };

}

using namespace cornerstone;



#endif //NS3_EXAMPLE_COMMON_H
