//
// Created by Hython on 2020/3/13.
//

#include "ns3_service.h"
#include "base64.h"
#include <queue>
#include <regex>

// request header, ulong term (8), msg_type type (1), int32 src (4), int32 dst (4), ulong last_log_term (8), ulong last_log_idx (8), ulong commit_idx (8) + one int32 (4) for log data size
#define RPC_REQ_HEADER_SIZE 3 * 4 + 8 * 4 + 1
/*

 log_data: {
    term: ulong,
    msg_type: byte,
    src: int32,
    dst: int32,
    last_log_term: ulong,
    last_log_idx: ulong,
    commit_idx: ulong,
    //datasize: int32,
    log_data: [{
        term:,
        val_type,
        val_size,
        data: string
    }]
 }

 */

// response header ulong term (8), msg_type type (1), int32 src (4), int32 dst (4), ulong next_idx (8), bool accepted (1)
#define RPC_RESP_HEADER_SIZE 4 * 2 + 8 * 2 + 2
using namespace ns3;

namespace cornerstone::ns3impls {

    NS_LOG_COMPONENT_DEFINE("RPC_SESSION");

        class fs_based_logger: public logger {
        public:
            fs_based_logger(const std::string& log_file, ns3::LogLevel level)
            :level_(level),fs_(log_file),buffer_(),lock_(){}

            virtual ~fs_based_logger();

            __nocopy__(fs_based_logger);

            virtual void debug(const std::string& log_line) {
                if (level_ <= ns3::LogLevel::LOG_DEBUG) {
                    write_log("dbug", log_line);
                }
            }

            virtual void info(const std::string& log_line) {
                if (level_ <= ns3::LogLevel::LOG_INFO) {
                    write_log("info", log_line);
                }
            }

            virtual void warn(const std::string& log_line) {
                if (level_ <= ns3::LogLevel::LOG_WARN) {
                    write_log("warn", log_line);
                }
            }

            virtual void err(const std::string& log_line) {
                if (level_ <= ns3::LogLevel::LOG_ERROR) {
                    write_log("errr", log_line);
                }
            }

        public:
            void write_log(const std::string& level, const std::string& log_line);
        private:
            ns3::LogLevel level_;
            std::ofstream fs_;
            std::queue<std::string> buffer_;
            std::mutex lock_;
        };




//    class ns3_service_impl: public std::enable_shared_from_this<ns3_service_impl> {
//    public:
//        ns3_service_impl();
//        ~ns3_service_impl();
//    public:
//        void run();
//
//    private:
//        void stop();
//        void worker_entry();
//        void flush_all_loggers(std::error_code err);
//
//    private:
//        //io_service
//
//        //timer
//
//
//        std::list<ptr<ns3impls::fs_based_logger>> loggers_;
//        bool stopping_;
//        std::atomic_int continue_;
//        std::mutex logger_list_lock_;
//        std::mutex stoppin_lock_;
//        std::condition_variable stopping_cv_;
//
//        friend ns3_service;
//        friend ns3impls::fs_based_logger;
//    };

    class rpc_session;
    typedef std::function<void(const ptr<rpc_session>&)> session_closed_callback;

    class rpc_session: public std::enable_shared_from_this<rpc_session> {
    private:
        ptr<msg_handler> handler_;
        ns3::Ptr<ns3::Socket> socket_;
        //session_closed_callback callback_;
        std::map<Address, std::string>                  m_bufferedData;
        session_closed_callback callback_;
    public:
        template<typename SessionCloseCallback>
        rpc_session(ns3::Ptr<ns3::Socket> & io, ptr<msg_handler>& handler,SessionCloseCallback&& callback)
                : handler_(handler), socket_(io), callback_(callback)
                {

                }

    __nocopy__(rpc_session)

    public:
        ~rpc_session() {
        }

    public:

        void HandleAccept(Ptr<Socket> socket, const Address &from){
            socket->SetRecvCallback (MakeCallback(&rpc_session::HandleRead, this));
        }

        void HandleRead(ns3::Ptr<ns3::Socket> socket) {
            NS_LOG_INFO(socket);
            Ptr<Packet> packet;
            Address from;

            while(packet = socket->RecvFrom(from)) {
                if(packet->GetSize() == 0){
                    break;
                }
                if(InetSocketAddress::IsMatchingType(from)) {
                    std::string delimiter = "#";
                    std::string parsedPacket;
                    // [C++ 11] Use smart pointer to prevent memory leaks
                    std::shared_ptr<char> packetInfo(new char[packet->GetSize() + 1],
                                                     [](char *p){delete []p;});
                    std::ostringstream totalStream;
                    packet->CopyData(reinterpret_cast<uint8_t *>(packetInfo.get()), packet->GetSize());
                    packetInfo.get()[packet->GetSize()] = '\0';
                    NS_LOG_DEBUG("READ mbufdatasize:"<<m_bufferedData.size());
                    totalStream << m_bufferedData[from] << packetInfo.get();
                    std::string totalReceivedData(totalStream.str());
                    NS_LOG_INFO( " Total Received Data: "<< totalReceivedData);

                    size_t pos = 0;
                    while((pos = totalReceivedData.find(delimiter))!=std::string::npos) {
                        parsedPacket = totalReceivedData.substr(0, pos);
                        NS_LOG_INFO("Parsed Packet: " << parsedPacket);

                        rapidjson::Document d;
                        d.Parse(parsedPacket.c_str());

                        if(!d.IsObject()){
                            NS_LOG_WARN("The parsed packet is corrupted");
                            totalReceivedData.erase(0, pos + delimiter.length());
                            continue;
                        }
                        read_complete(d);

                        totalReceivedData.erase(0,pos+delimiter.length());
                    }
                    m_bufferedData[from] = totalReceivedData;

                }
            }
        }

        Ptr<Socket> socket(){return socket_;}

        void NormalCloseCallback(Ptr<Socket> socket) {
            NS_LOG_FUNCTION (this << socket);
            auto it=m_bufferedData.begin();
            for(;it!=m_bufferedData.end();it++) {
                if(!it->second.empty()){
                    NS_LOG_WARN("Buffer from " << it->first <<" is still not empty");
                }
            }
        }

        void start() {
            //ptr<rpc_session> self = this->shared_from_this();
            socket_->SetRecvCallback(MakeCallback(&rpc_session::HandleRead, this));
            //CloseCallback
//            socket_->SetCloseCallbacks(MakeCallback(&rpc_session::NormalCloseCallback),
//                    MakeCallback())
            //SendCallback

            //Connection Established Trace
        }

        void stop() {
            //socket_->Close();
            if(callback_){
                callback_(this->shared_from_this());
            }
        }

    private:

        void read_complete(rapidjson::Document &d) {
            msg_type t = (msg_type)d["msg_type"].GetInt();
            int32 src = d["src"].GetInt();
            int32 dst = d["dst"].GetInt();
            ulong term = d["term"].GetUint64();
            ulong last_term = d["last_log_term"].GetUint64();
            ulong last_idx = d["last_log_idx"].GetUint64();
            ulong commit_idx = d["commit_idx"].GetUint64();
            ptr<req_msg> req(cs_new<req_msg>(term, t, src, dst, last_term, last_idx, commit_idx));
            if(d["log_data"].IsArray()) {
                for(size_t j=0; j<d["log_data"].Size();j++) {
                    ulong term = d["log_data"][j]["term"].GetUint64();
                    log_val_type val_type = (log_val_type)d["log_data"][j]["val_type"].GetInt();
                    int32 val_size = d["log_data"][j]["val_size"].GetInt();
                    std::string s = d["log_data"][j]["data"].GetString();
                    std::string decoded = base64_decode(s);
                    NS_LOG_DEBUG("BASE64 original:"<<s);
                    NS_LOG_DEBUG("decoded:"<<decoded);
                    NS_LOG_DEBUG("decoded size:"<<decoded.size());
                    bufptr buf(buffer::alloc((size_t)val_size));
                    for(int i=0;i<decoded.size();i++){
                        buf->put((byte)decoded[i]);
                    }
                    NS_LOG_DEBUG("bufsize:"<<buf->size());
                    buf->pos(0);
                    //buf->put(decoded);
                    ptr<log_entry> entry(cs_new<log_entry>(term,std::move(buf),val_type));
                    req->log_entries().push_back(entry);
                }
            }

            ptr<resp_msg> resp = handler_ ->process_req(*req);
            if (!resp) {
                NS_LOG_ERROR("no response is returned from raft message handler, potential system bug");
                this->stop();
            }else {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                rapidjson::Document dres;
                rapidjson::Value value;
                dres.SetObject();
                dres.AddMember("msg_type",value, d.GetAllocator());
                dres.AddMember("src",value, d.GetAllocator());
                dres.AddMember("dst",value, d.GetAllocator());
                dres.AddMember("term",value, d.GetAllocator());
                dres.AddMember("next_idx",value, d.GetAllocator());
                dres.AddMember("accepted", value, d.GetAllocator());
                dres["msg_type"].SetInt(resp->get_type());
                dres["src"].SetInt(resp->get_src());
                dres["dst"].SetInt(resp->get_dst());
                dres["term"].SetUint64(resp->get_term());
                dres["next_idx"].SetUint64(resp->get_next_idx());
                dres["accepted"].SetBool(resp->get_accepted());
                dres.Accept(writer);
                const uint8_t delimiter[] = "#";
                socket_->Send(reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
                socket_->Send(delimiter, 1, 0);


            }
        }

    };

class ns3_rpc_listener: public rpc_listener, public std::enable_shared_from_this<ns3_rpc_listener> {
    public:
        ns3_rpc_listener(Ptr<Socket> &io, ushort port):socket(io){}
        __nocopy__(ns3_rpc_listener);

    public:
        virtual void stop() override {

        }

        virtual void listen(ptr<msg_handler>& handler) override {
            this->handler = handler;
            start();
        }
        void AcceptCallBackNS3(Ptr<Socket> socket, const Address &from){
            NS_LOG_FUNCTION(this);
//            if(this->socket==socket){
//                NS_LOG_DEBUG("Accept Socket EQUAL");
//            }else{
//                NS_LOG_DEBUG("Accept Socket NOT EQUAL");
//            }

            ptr<ns3_rpc_listener> self(this->shared_from_this());
            ptr<rpc_session> session(cs_new<rpc_session>(socket, handler,
                                                         [self](const ptr<rpc_session>& s){
                                                             self->remove_session(s);
                                                         }));
            session->start();
            active_sessions_.push_back(session);
            //this->start();
        }
    private:


        void start() {
            NS_LOG_FUNCTION(this);
            socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                    MakeCallback(&ns3_rpc_listener::AcceptCallBackNS3,this));

        }



        void remove_session(const ptr<rpc_session> & session) {
            for (auto it = active_sessions_.begin(); it != active_sessions_.end(); ++it) {
                if (*it == session) {
                    active_sessions_.erase(it);
                    break;
                }
            }
        }

    private:
        Ptr<ns3::Socket> socket;
        ptr<msg_handler> handler;
        std::vector<ptr<rpc_session>> active_sessions_;
    };

class ns3_rpc_client: public rpc_client {
public:
    std::map<Address, std::string>                  m_bufferedData;

    ns3_rpc_client(Ptr<Socket> &io, std::string& hostname, std::string& port)
    :socket_(io),host_(hostname),port_(port),local(InetSocketAddress (Ipv4Address(hostname.c_str()), std::stoi(port))){
//        NS_LOG_INFO("IN RPC_CLIENT CONSTRUCTION:"<<m_bufferedData.size());
        //socket_->SetRecvCallback(MakeCallback(&ns3_rpc_client::ResponseRead,this));

    }

    virtual ~ns3_rpc_client(){

    }
public:
    void ResponseRead(rapidjson::Document &d){

        NS_LOG_INFO("Inside Response Read~");

        msg_type msgType = (msg_type)d["msg_type"].GetInt();
        int32 src = d["src"].GetInt();
        int32 dst = d["dst"].GetInt();
        ulong term = d["term"].GetUint64();
        ulong next_idx = d["next_idx"].GetUint64();
        bool accepted = d["accepted"].GetBool();

        NS_LOG_INFO("Response Read decode no wrong");
        ptr<resp_msg> rsp(cs_new<resp_msg>(term, msgType, src, dst, next_idx, accepted));
        ptr<rpc_exception> except;
        if(handler!= nullptr){

            NS_LOG_DEBUG("CALL handler");
            handler(rsp,except);
        }
    }

    void HandleRead(ns3::Ptr<ns3::Socket> socket) {
        NS_LOG_INFO("rpc_client:"<<m_bufferedData.size());
        Ptr<Packet> packet;
        Address from;

        while (packet = socket->RecvFrom(from)) {
            if (packet->GetSize() == 0) {
                break;
            }

            if (InetSocketAddress::IsMatchingType(from)) {
                std::string delimiter = "#";
                std::string parsedPacket;
                // [C++ 11] Use smart pointer to prevent memory leaks
                std::shared_ptr<char> packetInfo(new char[packet->GetSize() + 1],
                                                 [](char *p) { delete[]p; });
                std::ostringstream totalStream;
                packet->CopyData(reinterpret_cast<uint8_t *>(packetInfo.get()), packet->GetSize());
                packetInfo.get()[packet->GetSize()] = '\0';

                NS_LOG_DEBUG("bufferedData in "<<from<<":"<<m_bufferedData[from]);
                totalStream << m_bufferedData[from] << packetInfo.get();

                std::string totalReceivedData(totalStream.str());
                NS_LOG_INFO(" CLIENT Total Received Data: " << totalReceivedData);

                size_t pos = 0;
                while ((pos = totalReceivedData.find(delimiter)) != std::string::npos) {
                    parsedPacket = totalReceivedData.substr(0, pos);
                    NS_LOG_INFO("RPC CLIENT Parsed Packet: " << parsedPacket);

                    rapidjson::Document d;
                    d.Parse(parsedPacket.c_str());

                    if (!d.IsObject()) {
                        NS_LOG_WARN("The parsed packet is corrupted");
                        totalReceivedData.erase(0, pos + delimiter.length());
                        continue;
                    }
                    //read_complete(d);
                    ResponseRead(d);

                    totalReceivedData.erase(0, pos + delimiter.length());
                }
                m_bufferedData[from] = totalReceivedData;

            }
        }
    }


    virtual void send(ptr<req_msg>& req, rpc_handler when_done) __override__ {
        if(!socket_) {

            NS_LOG_ERROR("The socket is not initialized properly");
        }else {
            //多次调用send时有问题
            NS_LOG_INFO("SEND update to "<<host_<<":"<<port_);
            handler = when_done;
            if(when_done) {
                socket_->SetRecvCallback(MakeCallback(&ns3_rpc_client::HandleRead, this));
            }
            rapidjson::Value array(rapidjson::kArrayType);

            rapidjson::Document d;
            d.SetObject();
            rapidjson::Value value;
            rapidjson::Document::AllocatorType& allocator = d.GetAllocator();



            for(auto it=req->log_entries().begin();it!=req->log_entries().end();it++) {

                rapidjson::Value log;
                log.SetObject();
                log.AddMember("term", value, allocator);
                log.AddMember("val_type", value, allocator);
                log.AddMember("val_size",value,allocator);
                log.AddMember("data",value,allocator);
                log["term"].SetUint64((*it)->get_term());
                log["val_type"].SetInt((*it)->get_val_type());
                log["val_size"].SetInt((*it)->get_buf().size());
                (*it)->get_buf().pos(0);
                std::string encoded = base64_encode((*it)->get_buf().data(),(*it)->get_buf().size());
                //NS_LOG_DEBUG("BEFORE BASE64:"<<(*it)->get_buf().data());
                NS_LOG_DEBUG("BASE64 size:"<<(*it)->get_buf().size()<<", ENCODED:"<<encoded.c_str());
                log["data"].SetString(encoded.c_str(),encoded.size(),d.GetAllocator());
                array.PushBack(log, allocator);

            }

            d.AddMember("msg_type",value,allocator);
            d.AddMember("src",value,allocator);
            d.AddMember("dst",value,allocator);
            d.AddMember("term",value,allocator);
            d.AddMember("last_log_term",value,allocator);
            d.AddMember("last_log_idx",value,allocator);
            d.AddMember("commit_idx",value,allocator);

            d["msg_type"].SetInt(req->get_type());
            d["src"].SetInt(req->get_src());
            d["dst"].SetInt(req->get_dst());
            d["term"].SetUint64(req->get_term());
            d["last_log_term"].SetUint64(req->get_last_log_term());
            d["last_log_idx"].SetUint64(req->get_last_log_idx());
            d["commit_idx"].SetUint64(req->get_commit_idx());
            d.AddMember("log_data", array, d.GetAllocator());
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            d.Accept(writer);
            const uint8_t delimiter[] = "#";





            socket_->Send(reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
            socket_->Send(delimiter, 1, 0);





        }
    }

private:

    Ptr<Socket> socket_;
    rpc_handler handler;
    std::string host_;
    std::string port_;
    InetSocketAddress local;

};


    ns3_service::ns3_service(Ptr<Socket> &socket, std::map<ns3::Ipv4Address, ns3::Ptr<ns3::Socket>>* peersSockets):socket_(socket),m_peersSockets(peersSockets) {

    }

    ns3_service::~ns3_service() = default;

    void ExecuteTask(ptr<delayed_task> &task) {
        task->execute();
    }

    void ns3_service::schedule(ptr<delayed_task> &task, int32 milliseconds) {
        task->reset();
        EventId eventID = Simulator::Schedule(ns3::MilliSeconds(milliseconds), &ExecuteTask, task);
        m_tasks.insert({task, eventID});
    }

    void ns3_service::cancel(ptr<delayed_task> &task) {
        task->cancel();
        if(m_tasks.find(task)==m_tasks.end()) {
            NS_LOG_ERROR("the task to be canceled does not exist!");
            return;
        }
        EventId id = m_tasks[task];
        Simulator::Cancel(id);
        m_tasks.erase(task);

    }

    ptr<rpc_client> ns3_service::create_client(const std::string& endpoint) {
        static std::regex reg("^tcp://(([a-zA-Z0-9\\-]+\\.)*([a-zA-Z0-9]+)):([0-9]+)$");
        std::smatch mresults;
        if (!std::regex_match(endpoint, mresults, reg) || mresults.size() != 5) {
            return ptr<rpc_client>();
        }

        std::string hostname = mresults[1].str();
        std::string port = mresults[4].str();
        Ipv4Address ipv4Address = Ipv4Address(hostname.c_str());
        //NS_LOG_DEBUG("hostname:"<<hostname);
        NS_LOG_DEBUG("create_client IP_addr:"<<ipv4Address);
        if(m_peersSockets->find(ipv4Address)==m_peersSockets->end()){
            std::cerr<<"Cannot find the related socket to send messages!"<<std::endl;
            return ptr<rpc_client>();
        }
        Ptr<Socket> t=(*m_peersSockets)[ipv4Address];
        return cornerstone::ptr<ns3_rpc_client>(new ns3_rpc_client(t,hostname, port));
    }

    ptr<logger> ns3_service::create_logger(ns3::LogLevel level, const std::string &log_file) {
        return cornerstone::ptr<logger>();
    }

    ptr<rpc_listener> ns3_service::create_rpc_listener(ushort listening_port, ptr<logger> &l) {
        return cornerstone::ptr<ns3_rpc_listener>(new ns3_rpc_listener(socket_, listening_port));
    }

    void ns3_service::stop() {

    }

    void ns3_service::cancel_impl(ptr<delayed_task> &task) {

    }
}