#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>

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
#include <webrtc/api/peerconnectioninterface.h>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>


#include <functional>
#include <iostream>
#include <thread>