/*
 * Copyright (c) 2010 Keith Cullen.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *  @file test_tls6client.c
 *
 *  @brief Source file for the FreeCoAP TLS/IPv6 client test application
 */

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "tls6sock.h"
#include "tls.h"
#include "sock.h"
#include "coap_log.h"

#define SERVER_COMMON_NAME  "dummy/server"
#define TRUST_FILE_NAME     "root_server_cert.pem"
#define CERT_FILE_NAME      "client_cert.pem"
#define KEY_FILE_NAME       "client_privkey.pem"
#define HOST                "::1"
#define PORT                "9999"
#define BUF_SIZE            (1 << 4)
#define TIMEOUT             30
#define NUM_ITER            2
#define DELAY               2

/* ignore broken pipe signal, i.e. don't terminate if server terminates */
static void set_signal()
{
    struct sigaction sa = {{0}};
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, NULL);
}

static int client()
{
    tls6sock_t s = {0};
    char addr_str[INET6_ADDRSTRLEN] = {0};
    char out_buf[BUF_SIZE] = {0};
    char in_buf[BUF_SIZE] = {0};
    int ret = 0;
    int i = 0;

    ret = tls6sock_open(&s, HOST, PORT, SERVER_COMMON_NAME, TIMEOUT);
    if (ret != SOCK_OK)
    {
        return ret;
    }

    tls6sock_get_addr_string(&s, addr_str, sizeof(addr_str));
    coap_log_notice("Connected to address %s and port %d", addr_str, tls6sock_get_port(&s));
    if (tls6sock_is_resumed(&s))
        coap_log_debug("Session resumed");
    else
        coap_log_debug("Session not resumed");

    for (i = 0; i < BUF_SIZE; i++)
    {
        out_buf[i] = i;
    }

    ret = tls6sock_write_full(&s, out_buf, BUF_SIZE);
    if (ret <= 0)
    {
        tls6sock_close(&s);
        return ret;
    }
    coap_log_debug("Sent %d bytes", ret);

    ret = tls6sock_read_full(&s, in_buf, BUF_SIZE);
    if (ret <= 0)
    {
        tls6sock_close(&s);
        return ret;
    }
    coap_log_debug("Received %d bytes", ret);

    tls6sock_close(&s);
    return SOCK_OK;
}

int main()
{
    time_t start = 0;
    time_t end = 0;
    int ret = 0;
    int i = 0;

    /* initialise signal handling */
    set_signal();

    coap_log_set_level(COAP_LOG_DEBUG);

    ret = tls_init();
    if (ret != SOCK_OK)
    {
        coap_log_error("%s", sock_strerror(ret));
        return EXIT_FAILURE;
    }

    ret = tls_client_init(TRUST_FILE_NAME, CERT_FILE_NAME, KEY_FILE_NAME);
    if (ret != SOCK_OK)
    {
        coap_log_error("%s", sock_strerror(ret));
        tls_deinit();
        return EXIT_FAILURE;
    }

    for (i = 0; i < NUM_ITER; i++)
    {
        start = time(NULL);
        ret = client();
        end = time(NULL);
        if (ret != SOCK_OK)
        {
            coap_log_error("%s", sock_strerror(ret));
            tls_client_deinit();
            tls_deinit();
            return EXIT_FAILURE;
        }
        coap_log_info("Result: %s", sock_strerror(ret));
        coap_log_debug("Time: %d sec", (int)(end - start));
        coap_log_debug("Sleeping for %d seconds...", DELAY);
        sleep(DELAY);
    }

    tls_client_deinit();
    tls_deinit();
    return EXIT_SUCCESS;
}
