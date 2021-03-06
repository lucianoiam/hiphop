/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "IpcChannel.hpp"

#include <errno.h>
#include <sys/select.h>

USE_NAMESPACE_DISTRHO

IpcChannel::IpcChannel(int fdr, int fdw)
{
    ipc_conf_t conf;
    conf.fd_r = fdr;
    conf.fd_w = fdw;
    fIpc = ipc_init(&conf); 
}

IpcChannel::~IpcChannel()
{
    if (fIpc != nullptr) {
        ipc_destroy(fIpc);
    }
}

int IpcChannel::getFdRead() const
{
    return ipc_get_config(fIpc)->fd_r;
}

int IpcChannel::getFdWrite() const
{
    return ipc_get_config(fIpc)->fd_w;
}

int IpcChannel::read(tlv_t* packet, int timeoutMs) const
{
    if (timeoutMs == -1) {
        return waitAndRead(packet);
    }

    fd_set rfds;
    FD_ZERO(&rfds);

    const int fd = getFdRead();
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000L * timeoutMs;

    const int rc = select(fd + 1, &rfds, 0, 0, &tv);

    if (rc == -1) {
        d_stderr("Failed select() on IPC channel - %s", strerror(errno));
        return -1;
    }

    if (rc == 0) {
        return -1; // no fd ready
    }

    if (waitAndRead(packet) == -1) {
        return -1;
    }

    return 0;
}

int IpcChannel::waitAndRead(tlv_t* packet) const
{
    if (ipc_read(fIpc, packet) == -1) {
        d_stderr("Could not read from IPC channel - %s", strerror(errno));
        return -1;
    }

    return 0;
}

int IpcChannel::write(msg_opcode_t opcode) const
{
    return write(opcode, 0, 0);
}

int IpcChannel::write(msg_opcode_t opcode, String& str) const
{
    const char *cStr = static_cast<const char *>(str);
    return write(opcode, cStr, strlen(cStr) + 1);
}

int IpcChannel::write(msg_opcode_t opcode, const void* payload, int payloadSize) const
{
    tlv_t packet;

    packet.t = static_cast<short>(opcode);
    packet.l = payloadSize;
    packet.v = payload;

    const int rc = ipc_write(fIpc, &packet);

    if (rc == -1) {
        d_stderr("Could not write to IPC channel - %s", strerror(errno));
    }

    return rc;
}
