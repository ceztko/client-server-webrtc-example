/**
 * This a minimal fully functional example for setting up a client written in JavaScript that
 * communicates with a server via WebRTC data channels. This uses WebSockets to perform the WebRTC
 * handshake (offer/accept SDP) with the server. We only use WebSockets for the initial handshake
 * because TCP often presents too much latency in the context of real-time action games. WebRTC
 * data channels, on the other hand, allow for unreliable and unordered message sending via SCTP
 *
 * Brian Ho
 * brian@brkho.com
 */

// The WebSocket object used to manage a connection.
let webSocketConnection = null;
// The RTCPeerConnection through which we engage in the SDP handshake.
let rtcPeerConnection = null;
// The data channel used to communicate.
let dataChannel = null;

// Callback for when we receive a message on the data channel.
function onDataChannelMessage(event) {
    console.log('DataChannelMessage');
    console.log(event.data)
}

function onDataChannelOpen(event) {
    console.log('DataChannelOpen');
    dataChannel.send("OK");
}

function onDataChannelError(error) {
    console.log('DataChannel Error : ' + error);
}

// Callback for when the STUN server responds with the ICE candidates.
function onIceCandidate(event) {
    if (event && event.candidate) {
      webSocketConnection.send(JSON.stringify({type: 'candidate', payload: event.candidate}));
      console.log(event.candidate)
    }
}

// Callback for when the SDP offer was successfully created.
function onOfferFulfilled(description) {
    rtcPeerConnection.setLocalDescription(description);
    webSocketConnection.send(JSON.stringify({type: 'offer', payload: description}));
}

function onOfferRejected(reason) {
    console.log(reason)
}

// Callback for when the WebSocket is successfully opened.
function onWebSocketOpen() {
    // NOTE: STUN servers may not work in some circustances. If ICE connection can't be established,
    // try to change them
    const config = { iceServers: [{ urls: ["stun:iphone-stun.strato-iphone.de:3478", "stun:stun01.sipphone.com"]  }] };
    rtcPeerConnection = new RTCPeerConnection(config);
    //For reference, check https://hpbn.co/webrtc/
    const dataChannelConfig = { ordered: false, maxRetransmits: 0 };
    dataChannel = rtcPeerConnection.createDataChannel('dc', dataChannelConfig);
    dataChannel.onmessage = onDataChannelMessage;
    dataChannel.onopen = onDataChannelOpen;
    dataChannel.onerror = onDataChannelError;
    rtcPeerConnection.ondatachannel = onDataChannelOpen
    rtcPeerConnection.onicecandidate = onIceCandidate;
    rtcPeerConnection.oniceconnectionstatechange = e => console.log(rtcPeerConnection.iceConnectionState);
    rtcPeerConnection.createOffer().then(onOfferFulfilled, onOfferRejected);
}

// Callback for when we receive a message from the server via the WebSocket.
function onWebSocketMessage(event) {
    const messageObject = JSON.parse(event.data);
    if (messageObject.type === 'answer') {
        rtcPeerConnection.setRemoteDescription(new RTCSessionDescription(messageObject.payload));
    } else if (messageObject.type === 'candidate') {
        rtcPeerConnection.addIceCandidate(new RTCIceCandidate(messageObject.payload));
        console.log(messageObject.payload)
    }
}

// Connects by creating a new WebSocket connection and associating some callbacks.
function connectHost(webSocketUrl) {
    webSocketConnection = new WebSocket(webSocketUrl);
    webSocketConnection.onopen = onWebSocketOpen;
    webSocketConnection.onmessage = onWebSocketMessage;
}
