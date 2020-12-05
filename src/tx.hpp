// -*- C++ -*-
//
// Copyright (C) 2017, 2018 Vasily Evseenko <svpcom@p2ptech.org>

/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 3.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>
#include <string>
#include <vector>
#include <string.h>
#include "wifibroadcast.hpp"
#include <stdexcept>
#include <iostream>

#include "Encryption.hpp"
#include "FEC.hpp"
#include "Helper.hpp"

class Transmitter : public FECEncoder {
public:
    Transmitter(RadiotapHeader radiotapHeader, int k, int m, const std::string &keypair);

    ~Transmitter() = default;

    void send_packet(const uint8_t *buf, size_t size);

    void send_session_key();

    virtual void select_output(int idx) = 0;

protected:
    // What inject_packet does is left to the implementation (e.g. PcapTransmitter)
    virtual void inject_packet(const uint8_t *buf, size_t size) = 0;

private:
    void sendFecBlock(const XBlock &xBlock);

    void make_session_key();

    Encryptor mEncryptor;
protected:
    Ieee80211Header mIeee80211Header;
public:
    // const since params like bandwidth never change !
    const RadiotapHeader mRadiotapHeader;
};

// Pcap Transmitter injects packets into the wifi adapter using pcap
class PcapTransmitter : public Transmitter {
public:
    PcapTransmitter(RadiotapHeader radiotapHeader, int k, int m, const std::string &keypair, uint8_t radio_port,
                    const std::vector<std::string> &wlans);

    virtual ~PcapTransmitter();

    void select_output(int idx) override { current_output = idx; }

private:
    void inject_packet(const uint8_t *buf, size_t size) override;

    // the radio port is what is used as an index to multiplex multiple streams (telemetry,video,...)
    // into the one wfb stream
    const uint8_t radio_port;
    // TODO what the heck is this one ?
    // I think it is supposed to be the wifi interface data is sent on
    int current_output=0;
    uint16_t ieee80211_seq=0;
    std::vector<pcap_t *> ppcap;
};

// UdpTransmitter can be used to emulate a wifi bridge without using a wifi adapter
// Usefully for Testing and Debugging.
// Use the Aggregator functionality as rx when using UdpTransmitter
class UdpTransmitter : public Transmitter {
public:
    UdpTransmitter(int k, int m, const std::string &keypair, const std::string &client_addr, int client_port)
            : Transmitter({}, k, m, keypair) {
        sockfd = SocketHelper::open_udp_socket(client_addr, client_port);
    }

    virtual ~UdpTransmitter() {
        close(sockfd);
    }

    void select_output(int /*idx*/) override {}

private:
    void inject_packet(const uint8_t *buf, size_t size) override;
    int sockfd;
};
