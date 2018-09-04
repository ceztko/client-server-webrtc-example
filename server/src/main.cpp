// This a minimal fully functional example for setting up a server written in C++ that communicates
// with clients via WebRTC data channels. This uses WebSockets to perform the WebRTC handshake
// (offer/accept SDP) with the client. We only use WebSockets for the initial handshake because TCP
// often presents too much latency in the context of real-time action games. WebRTC data channels,
// on the other hand, allow for unreliable and unordered message sending via SCTP.
//
// Author: brian@brkho.com

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "observers.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/pc/peerconnectionfactory.h>
#include <webrtc/base/physicalsocketserver.h>
#include <webrtc/base/fakenetwork.h>
#include <webrtc/base/ssladapter.h>
#include <webrtc/base/thread.h>
#include <webrtc/p2p/client/basicportallocator.h>
#include "webrtc/p2p/base/basicpacketsocketfactory.h"
#include "webrtc/api/audio_codecs/builtin_audio_decoder_factory.h"
#include "webrtc/api/audio_codecs/builtin_audio_encoder_factory.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <thread>

// WebSocket++ types are gnarly.
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace std;

// Some forward declarations.
void OnDataChannelCreated(webrtc::DataChannelInterface* channel);
void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
void OnDataChannelMessage(const webrtc::DataBuffer& buffer);
void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc);

typedef websocketpp::server<websocketpp::config::asio> WebSocketServer;
typedef WebSocketServer::message_ptr message_ptr;

// The WebSocket server being used to handshake with the clients.
WebSocketServer ws_server;
// The peer conncetion factory that sets up signaling and worker threads. It is also used to create
// the PeerConnection.
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
// The separate thread where all of the WebRTC code runs since we use the main thread for the
// WebSocket listening loop.
std::thread webrtc_thread;
// The WebSocket connection handler that uniquely identifies one of the connections that the
// WebSocket has open. If you want to have multiple connections, you will need to store more than
// one of these.
websocketpp::connection_hdl websocket_connection_handler;
// The peer connection through which we engage in the SDP handshake.
rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
// The data channel used to communicate.
rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel;
// The observer that responds to peer connection events.
PeerConnectionObserver peer_connection_observer(OnDataChannelCreated, OnIceCandidate);
// The observer that responds to data channel events.
DataChannelObserver data_channel_observer(OnDataChannelMessage);
// The observer that responds to session description creation events.
CreateSessionDescriptionObserver create_session_description_observer(OnAnswerCreated);
// The observer that responds to session description set events. We don't really use this one here.
SetSessionDescriptionObserver set_session_description_observer;
std::unique_ptr<rtc::BasicPacketSocketFactory> socket_factory;
std::unique_ptr<rtc::Thread> network_thread;
std::unique_ptr<rtc::Thread> worker_thread;
std::unique_ptr<cricket::BasicPortAllocator> port_allocator;

class CustomNetworkManager : public rtc::NetworkManagerBase
{
public:
    void AddAddress(const string &addrstr)
    {
        rtc::SocketAddress sockaddr;
        if (!sockaddr.FromString(addrstr))
        {

        }

        auto ipaddr = sockaddr.ipaddr();
        int prefixLength;
        if (ipaddr.family() == AF_INET6)
            prefixLength = 64;
        else
            prefixLength = 32;

        std::vector<rtc::Network*> networks;
        auto net = make_unique<rtc::Network>("host", "host", ipaddr, prefixLength);
        net->AddIP(ipaddr);
        networks.push_back(net.release());

        bool changed;
        MergeNetworkList(networks, &changed);
    }

    void StartUpdating() override
    {
        SignalNetworksChanged();
    }
    void StopUpdating() override { }
};

CustomNetworkManager network_manager;


// Callback for when the data channel is successfully created. We need to re-register the updated
// data channel here.
void OnDataChannelCreated(webrtc::DataChannelInterface* channel)
{
    data_channel = channel;
    data_channel->RegisterObserver(&data_channel_observer);
}

// Callback for when the STUN server responds with the ICE candidates.
void OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    auto cand = candidate->candidate();
    auto addr = cand.address();
    addr.SetIP("34.213.210.209"); // public IP
    cand.set_address(addr);
    std::string candidate_str = cand.ToString();
    rapidjson::Document message_object;
    message_object.SetObject();
    message_object.AddMember("type", "candidate", message_object.GetAllocator());
    rapidjson::Value candidate_value;
    candidate_value.SetString(rapidjson::StringRef(candidate_str.c_str()));
    rapidjson::Value sdp_mid_value;
    //sdp_mid_value.SetString(rapidjson::StringRef(candidate->sdp_mid().c_str()));
    sdp_mid_value.SetString(rapidjson::StringRef("data")); // FIXME hardcoded
    rapidjson::Value message_payload;
    message_payload.SetObject();
    message_payload.AddMember("candidate", candidate_value, message_object.GetAllocator());
    message_payload.AddMember("sdpMid", sdp_mid_value, message_object.GetAllocator());
    message_payload.AddMember("sdpMLineIndex", candidate->sdp_mline_index(),
                              message_object.GetAllocator());
    message_object.AddMember("payload", message_payload, message_object.GetAllocator());
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    message_object.Accept(writer);
    std::string payload = strbuf.GetString();
    ws_server.send(websocket_connection_handler, payload, websocketpp::frame::opcode::value::text);
    (void)candidate;
}

// Callback for when the server receives a message on the data channel.
void OnDataChannelMessage(const webrtc::DataBuffer& buffer)
{
    (void)buffer;
    data_channel->Send(buffer);
}

// Callback for when the answer is created. This sends the answer back to the client.
void OnAnswerCreated(webrtc::SessionDescriptionInterface* desc)
{
    peer_connection->SetLocalDescription(&set_session_description_observer, desc);
    // Apologies for the poor code ergonomics here; I think rapidjson is just verbose.
    std::string offer_string;
    desc->ToString(&offer_string);
    rapidjson::Document message_object;
    message_object.SetObject();
    message_object.AddMember("type", "answer", message_object.GetAllocator());
    rapidjson::Value sdp_value;
    sdp_value.SetString(rapidjson::StringRef(offer_string.c_str()));
    rapidjson::Value message_payload;
    message_payload.SetObject();
    message_payload.AddMember("type", "answer", message_object.GetAllocator());
    message_payload.AddMember("sdp", sdp_value, message_object.GetAllocator());
    message_object.AddMember("payload", message_payload, message_object.GetAllocator());
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    message_object.Accept(writer);
    std::string payload = strbuf.GetString();
    ws_server.send(websocket_connection_handler, payload, websocketpp::frame::opcode::value::text);
}

// Callback for when the WebSocket server receives a message from the client.
void OnWebSocketMessage(WebSocketServer* s, websocketpp::connection_hdl hdl, message_ptr msg)
{
    websocket_connection_handler = hdl;
    rapidjson::Document message_object;
    message_object.Parse(msg->get_payload().c_str());
    // Probably should do some error checking on the JSON object.
    std::string type = message_object["type"].GetString();
    if (type == "offer") {
        auto conn = s->get_con_from_hdl(hdl);
        auto localAddres = conn->get_socket().local_endpoint().address();
        //network_manager.AddAddress(localAddres.to_string());
        network_manager.AddAddress("172.31.46.189");

        std::string sdp = message_object["payload"]["sdp"].GetString();
        webrtc::PeerConnectionInterface::RTCConfiguration configuration;

        auto allocator = std::make_unique<cricket::BasicPortAllocator>(&network_manager, socket_factory.get());
        peer_connection = peer_connection_factory->CreatePeerConnection(configuration, std::move(allocator), nullptr,
                          &peer_connection_observer);
        webrtc::DataChannelInit data_channel_config;
        data_channel = peer_connection->CreateDataChannel("dc", &data_channel_config);
        data_channel->RegisterObserver(&data_channel_observer);

        webrtc::SdpParseError error;
        webrtc::SessionDescriptionInterface* session_description(
            webrtc::CreateSessionDescription("offer", sdp, &error));
        peer_connection->SetRemoteDescription(&set_session_description_observer, session_description);
        peer_connection->CreateAnswer(&create_session_description_observer, nullptr);
    }
}

// The thread entry point for the WebRTC thread. This sets the WebRTC thread as the signaling thread
// and creates a worker thread in the background.
void SignalThreadEntry()
{
    // Create the PeerConnectionFactory.
    rtc::InitializeSSL();

    network_thread.reset(rtc::Thread::CreateWithSocketServer().release());
    worker_thread.reset(rtc::Thread::Create().release());
    rtc::Thread *signaling_thread = rtc::Thread::Current();
    network_thread->Start();
    worker_thread->Start();
    peer_connection_factory = webrtc::CreatePeerConnectionFactory(network_thread.get(), worker_thread.get(), signaling_thread,
        nullptr,
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        nullptr, nullptr);

    socket_factory.reset(new rtc::BasicPacketSocketFactory(network_thread.get()));
    signaling_thread->Run();
}

// Main entry point of the code.
int main()
{
    rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_ERROR);
    webrtc_thread = std::thread(SignalThreadEntry);
    // In a real game server, you would run the WebSocket server as a separate thread so your main
    // process can handle the game loop.
    ws_server.set_message_handler(bind(OnWebSocketMessage, &ws_server, ::_1, ::_2));
    ws_server.init_asio();
    ws_server.clear_access_channels(websocketpp::log::alevel::all);
    ws_server.set_reuse_addr(true);
    ws_server.listen(8080);
    ws_server.start_accept();
    // I don't do it here, but you should gracefully handle closing the connection.
    ws_server.run();
    rtc::CleanupSSL();
}
