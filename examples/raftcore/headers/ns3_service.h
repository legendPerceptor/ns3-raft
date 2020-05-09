//
// Created by Hython on 2020/3/13.
//

#ifndef _NS3_SERVICE_H
#define _NS3_SERVICE_H

#include "cornerstone.hxx"
#include "ns3all.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace cornerstone::ns3impls {

    class ns3_service_impl;

    class ns3_service: public delayed_task_scheduler, public rpc_client_factory {

    public:
        ns3_service(ns3::Ptr<ns3::Socket> &socket, std::map<ns3::Ipv4Address, ns3::Ptr<ns3::Socket>>* peersSockets);
        ~ns3_service();
        __nocopy__(ns3_service);

    public:
        virtual void schedule(ptr<delayed_task>& task, int32 milliseconds) __override__;
        virtual ptr<rpc_client> create_client(const std::string& endpoint) __override__;

        ptr<logger> create_logger(ns3::LogLevel level, const std::string& log_file);

        ptr<rpc_listener> create_rpc_listener(ushort listening_port, ptr<logger>& l);

        void stop();

        virtual void cancel(ptr<delayed_task>& task) __override__;

    private:
        virtual void cancel_impl(ptr<delayed_task> & task)__override__;

    private:
        ns3::Ptr<ns3::Socket> socket_;
        std::map<ns3::Ipv4Address, ns3::Ptr<ns3::Socket>>* m_peersSockets;
        std::map<ptr<cornerstone::delayed_task>, ns3::EventId> m_tasks;
    };

}

#endif //_NS3_SERVICE_H
