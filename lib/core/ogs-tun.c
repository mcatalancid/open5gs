/*
 * Copyright (C) 2019 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
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

#include "ogs-tun.h"

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __ogs_sock_domain

#if !defined(WIN32)
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#if defined(__linux__)
#include <linux/if_tun.h>
#endif

#ifndef IFNAMSIZ
#define IFNAMSIZ 32
#endif

ogs_socket_t ogs_tun_open(char *ifname, int len, int is_tap)
{
    ogs_socket_t fd = INVALID_SOCKET;

#if !defined(__linux__)
    return fd;
#else
    const char *dev = "/dev/net/tun";
    int rc;
    struct ifreq ifr;
    int flags = IFF_NO_PI;

    ogs_assert(ifname);

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno,
                "open() failed : dev[%s]", dev);
        return INVALID_SOCKET;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = (is_tap ? (flags | IFF_TAP) : (flags | IFF_TUN));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);

    rc = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (rc < 0) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno,
                "ioctl() failed : dev[%s] flags[0x%x]", dev, flags);
        goto cleanup;
    }

    return fd;

cleanup:
    close(fd);
    return INVALID_SOCKET;
#endif
}

int ogs_tun_set_ip(char *ifname, ogs_ipsubnet_t *gw, ogs_ipsubnet_t *sub)
{
    return OGS_OK;
}

ogs_pkbuf_t *ogs_tun_read(ogs_socket_t fd, ogs_pkbuf_pool_t *packet_pool)
{
    ogs_pkbuf_t *recvbuf = NULL;
    int n;

    ogs_assert(fd != INVALID_SOCKET);
    ogs_assert(packet_pool);

    recvbuf = ogs_pkbuf_alloc(packet_pool, OGS_MAX_PKT_LEN);
    ogs_assert(recvbuf);
    ogs_pkbuf_reserve(recvbuf, OGS_TUN_MAX_HEADROOM);
    ogs_pkbuf_put(recvbuf, OGS_MAX_PKT_LEN-OGS_TUN_MAX_HEADROOM);

    n = ogs_read(fd, recvbuf->data, recvbuf->len);
    if (n <= 0) {
        ogs_log_message(OGS_LOG_WARN, ogs_socket_errno, "ogs_read() failed");
        ogs_pkbuf_free(recvbuf);
        return NULL;
    }

    ogs_pkbuf_trim(recvbuf, n);

    return recvbuf;
}

int ogs_tun_write(ogs_socket_t fd, ogs_pkbuf_t *pkbuf)
{
    ogs_assert(fd != INVALID_SOCKET);
    ogs_assert(pkbuf);

    if (ogs_write(fd, pkbuf->data, pkbuf->len) <= 0) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno, "ogs_write() failed");
        return OGS_ERROR;
    }

    return OGS_OK;
}
