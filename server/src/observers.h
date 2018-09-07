// No-op implementations of most webrtc::*Observer methods
//
// Author: brian@brkho.com
//         ceztko@gmail.com
#pragma once

#include <webrtc/api/peerconnectioninterface.h>

class PeerConnectionObserver : public webrtc::PeerConnectionObserver
{
  public:
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState stream) override
    {
        (void)stream;
    }

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override
    {
        (void)stream;
    }

    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override
    {
        (void)stream;
    }

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override
    {
        (void)channel;
    }

    void OnRenegotiationNeeded() override { }

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override
    {
        (void)new_state;
    }

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override
    {
        (void)new_state;
    }

    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) { }
};

class DataChannelObserver : public webrtc::DataChannelObserver
{
  public:
    void OnStateChange() override { }
    
    void OnMessage(const webrtc::DataBuffer& buffer) override
    {
        (void)buffer;
    }

    void OnBufferedAmountChange(uint64_t previous_amount) override
    {
        (void)previous_amount;
    }
};

class CreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver
{
  public:
    void OnSuccess(webrtc::SessionDescriptionInterface* desc)
    {
        (void)desc;
    }

    void OnFailure(const std::string& error)
    {
        (void)error;
    }

    int AddRef() const { return 0; }

    int Release() const { return 0; }
};

class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver
{
  public:
    SetSessionDescriptionObserver() { }

    void OnSuccess() {}

    void OnFailure(const std::string& error)
    {
        (void)error;
    }

    int AddRef() const { return 0; }

    int Release() const { return 0; }
};
