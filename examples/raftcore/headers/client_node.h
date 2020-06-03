//
// Created by Hython on 2020/4/16.
//

#ifndef NS3_CLIENT_NODE_H
#define NS3_CLIENT_NODE_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include <string>
#include <map>
#include "ns3_service.h"
using namespace ns3;

namespace cornerstone {

    class ClientNode: public Application, public std::enable_shared_from_this<ClientNode> {
    protected:
        // Inherited from Object base class.
        virtual void DoDispose ();

        // Inherited from Application base class.
        virtual void StartApplication ();
        virtual void StopApplication ();
        void OpenConnection();
    public:
        static TypeId GetTypeId ();
        ClientNode();

        enum State_t {
            NOT_STARTED,
            CONNECTING,
            READING,
            STOPPED
        };

        std::string GetStateString();
        std::string GetStateString(State_t state);
        void ConnectionSucceededCallback (Ptr<Socket> socket);
        void ConnectionFailedCallback(Ptr<Socket> socket);
        void ReceivedDataCallback(Ptr<Socket> socket);
        void RequestMainObject(bool resend);
        void SwitchToState(State_t state);
        void HandleRead(Ptr<Socket> socket);

        void SetPeersAddresses(Ipv4Address ipv4Address);

        void SetIpv4Address(Ipv4Address ipv4Address);


    private:
        Address m_remoteServerAddress;
        uint16_t m_remoteServerPort;
        Ptr<Socket>  m_socket;
        State_t m_state;

        EventId m_eventRequestMainObject;

        std::map<Ipv4Address, Ptr<Socket>> m_peersSockets;

        ptr<cornerstone::ns3impls::ns3_service> ns3Service;
        std::vector<Ipv4Address> m_peersAddresses;
        Ipv4Address m_ipv4Address;

        std::map<Address, std::string>                  m_bufferedData;

        std::map<ptr<rpc_client>, rpc_handler> clients_rpc_;

    };
}


#endif //NS3_CLIENT_NODE_H
