Video Sink Example
==================

A simple WebRTC Video Source to Sink sample using web socket transport:
- Uses node.js and WebSocket.
- Supports Chrome and Firefox.
- No need for a Web Server.
- Supports HTTPS and secure websocket wss.
- Supports H264 and VP8, see useH264 flag in code in index.html.

####  Prerequesites

- Node.js and WebSocket 

####  Server Steps 

- Clone this repo to your machine, does not need to be to a traditional web server. Can be same machine as browser or another one.
- Generate keys unless you have real ones, run these commands in the same folder as app.js (find equivalant commands for Windows or Linux these are for Mac).
  -  openssl genrsa -out webrtcwwsocket-key.pem 1024
  -  openssl req -new -key webrtcwwsocket-key.pem -out webrtcwwsocket-csr.pem
  -  openssl x509 -req -in webrtcwwsocket-csr.pem -signkey webrtcwwsocket-key.pem -out webrtcwwsocket-cert.pem
- run  'sudo node relay-server.js'
- You may get errors, if you do then install WebSocket in that folder, e.g. 'sudo npm install websocket'


####  Client Steps

- Open video-source.html in one browser. This will be the Source endpoint;
- From another browser tab/window point to https://\<your ip address\> . Accept self signed certs. This will be the Sink endpoint;
- Connect from the Source endpoint.

#### References

Mostly derived from [1]

[1] https://github.com/emannion/webrtc-audio-video
