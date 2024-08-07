/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * examples/tls/client/tls_client_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "mbedtls/config.h"

#if !defined(MBEDTLS_ENTROPY_C) || \
	!defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
	!defined(MBEDTLS_NET_C) || !defined(MBEDTLS_CTR_DRBG_C)
int tls_client_main(int argc, char **argv)
{
	printf("MBEDTLS_ENTROPY_C and/or\n");
	printf("MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or\n");
	printf("MBEDTLS_NET_C and/or MBEDTLS_CTR_DRBG_C and/or not defined.\n");
	return 0;
}
#else

#define mbedtls_printf     printf
#define mbedtls_fprintf    fprintf
#define mbedtls_snprintf   snprintf

#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/test/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

#ifdef CONFIG_EXAMPLES_TLS_ARTIK_KEY
#include "mbedtls/see_api.h"
#endif

/*
 * Definition for handling pthread
 */
#define TLS_CLIENT_PRIORITY     100
#define TLS_CLIENT_STACK_SIZE   51200
#define TLS_CLIENT_SCHED_POLICY SCHED_RR

struct pthread_arg {
	int argc;
	char **argv;
};

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DFL_SERVER_NAME         "localhost"
#define DFL_SERVER_ADDR         "127.0.0.1"
#define DFL_SERVER_PORT         "4433"
#define DFL_REQUEST_PAGE        "/"
#define DFL_REQUEST_SIZE        -1
#define DFL_DEBUG_LEVEL         5
#define DFL_NBIO                0
#define DFL_READ_TIMEOUT        0
#define DFL_MAX_RESEND          0
#define DFL_CA_FILE             ""
#define DFL_CA_PATH             ""
#define DFL_CRT_FILE            ""
#define DFL_KEY_FILE            ""
#define DFL_PSK                 ""
#define DFL_PSK_IDENTITY        "Client_identity"
#define DFL_ECJPAKE_PW          NULL
#define DFL_FORCE_CIPHER        0
#define DFL_RENEGOTIATION       MBEDTLS_SSL_RENEGOTIATION_DISABLED
#define DFL_ALLOW_LEGACY        -2
#define DFL_RENEGOTIATE         0
#define DFL_EXCHANGES           1
#define DFL_MIN_VERSION         -1
#define DFL_MAX_VERSION         -1
#define DFL_ARC4                -1
#define DFL_AUTH_MODE           -1
#define DFL_MFL_CODE            MBEDTLS_SSL_MAX_FRAG_LEN_NONE
#define DFL_TRUNC_HMAC          -1
#define DFL_RECSPLIT            -1
#define DFL_DHMLEN              -1
#define DFL_RECONNECT           0
#define DFL_RECO_DELAY          0
#define DFL_RECONNECT_HARD      0
#define DFL_TICKETS             MBEDTLS_SSL_SESSION_TICKETS_ENABLED
#define DFL_ALPN_STRING         NULL
#define DFL_TRANSPORT           MBEDTLS_SSL_TRANSPORT_STREAM
#define DFL_HS_TO_MIN           0
#define DFL_HS_TO_MAX           0
#define DFL_FALLBACK            -1
#define DFL_EXTENDED_MS         -1
#define DFL_ETM                 -1

#define GET_REQUEST "GET %s HTTP/1.0\r\nExtra-header: "
#define GET_REQUEST_END "\r\n\r\n"

#if defined(MBEDTLS_X509_CRT_PARSE_C)
#if defined(MBEDTLS_FS_IO)
#define USAGE_IO \
	"    ca_file=%%s          The single file containing the top-level CA(s) you fully trust\n" \
	"                        default: \"\" (pre-loaded)\n" \
	"    ca_path=%%s          The path containing the top-level CA(s) you fully trust\n" \
	"                        default: \"\" (pre-loaded) (overrides ca_file)\n" \
	"    crt_file=%%s         Your own cert and chain (in bottom to top order, top may be omitted)\n" \
	"                        default: \"\" (pre-loaded)\n" \
	"    key_file=%%s         default: \"\" (pre-loaded)\n"
#else
#define USAGE_IO \
	"    No file operations available (MBEDTLS_FS_IO not defined)\n"
#endif							/* MBEDTLS_FS_IO */
#else
#define USAGE_IO ""
#endif							/* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
#define USAGE_PSK                                                   \
	"    psk=%%s              default: \"\" (in hex, without 0x)\n" \
	"    psk_identity=%%s     default: \"Client_identity\"\n"
#else
#define USAGE_PSK ""
#endif							/* MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED */

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
#define USAGE_TICKETS                                       \
	"    tickets=%%d          default: 1 (enabled)\n"
#else
#define USAGE_TICKETS ""
#endif							/* MBEDTLS_SSL_SESSION_TICKETS */

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC)
#define USAGE_TRUNC_HMAC                                    \
	"    trunc_hmac=%%d       default: library default\n"
#else
#define USAGE_TRUNC_HMAC ""
#endif							/* MBEDTLS_SSL_TRUNCATED_HMAC */

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
#define USAGE_MAX_FRAG_LEN                                      \
	"    max_frag_len=%%d     default: 16384 (tls default)\n"   \
	"                        options: 512, 1024, 2048, 4096\n"
#else
#define USAGE_MAX_FRAG_LEN ""
#endif							/* MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

#if defined(MBEDTLS_SSL_CBC_RECORD_SPLITTING)
#define USAGE_RECSPLIT \
	"    recsplit=0/1        default: (library default: on)\n"
#else
#define USAGE_RECSPLIT
#endif

#if defined(MBEDTLS_DHM_C)
#define USAGE_DHMLEN \
	"    dhmlen=%%d           default: (library default: 1024 bits)\n"
#else
#define USAGE_DHMLEN
#endif

#if defined(MBEDTLS_SSL_ALPN)
#define USAGE_ALPN \
	"    alpn=%%s             default: \"\" (disabled)\n"   \
	"                        example: spdy/1,http/1.1\n"
#else
#define USAGE_ALPN ""
#endif							/* MBEDTLS_SSL_ALPN */

#if defined(MBEDTLS_SSL_PROTO_DTLS)
#define USAGE_DTLS \
	"    dtls=%%d             default: 0 (TLS)\n"                           \
	"    hs_timeout=%%d-%%d    default: (library default: 1000-60000)\n"    \
	"                        range of DTLS handshake timeouts in millisecs\n"
#else
#define USAGE_DTLS ""
#endif

#if defined(MBEDTLS_SSL_FALLBACK_SCSV)
#define USAGE_FALLBACK \
	"    fallback=0/1        default: (library default: off)\n"
#else
#define USAGE_FALLBACK ""
#endif

#if defined(MBEDTLS_SSL_EXTENDED_MASTER_SECRET)
#define USAGE_EMS \
	"    extended_ms=0/1     default: (library default: on)\n"
#else
#define USAGE_EMS ""
#endif

#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
#define USAGE_ETM \
	"    etm=0/1             default: (library default: on)\n"
#else
#define USAGE_ETM ""
#endif

#if defined(MBEDTLS_SSL_RENEGOTIATION)
#define USAGE_RENEGO \
	"    renegotiation=%%d    default: 0 (disabled)\n"      \
	"    renegotiate=%%d      default: 0 (disabled)\n"
#else
#define USAGE_RENEGO ""
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
#define USAGE_ECJPAKE \
	"    ecjpake_pw=%%s       default: none (disabled)\n"
#else
#define USAGE_ECJPAKE ""
#endif

#define USAGE_LARGEDATA \
	"    aging=%%s       default: 0 (disabled, receive a http request)\n"

#define USAGE \
	"\n usage: ssl_client2 param=<>...\n"                   \
	"\n acceptable parameters:\n"                           \
	"    server_name=%%s      default: localhost\n"         \
	"    server_addr=%%s      default: given by name\n"     \
	"    server_port=%%d      default: 4433\n"              \
	"    request_page=%%s     default: \".\"\n"             \
	"    request_size=%%d     default: about 34 (basic request)\n" \
	"                        (minimum: 0, max: 16384)\n" \
	"    debug_level=%%d      default: 0 (disabled)\n"      \
	"    nbio=%%d             default: 0 (blocking I/O)\n"  \
	"                        options: 1 (non-blocking), 2 (added delays)\n" \
	"    read_timeout=%%d     default: 0 ms (no timeout)\n"    \
	"    max_resend=%%d       default: 0 (no resend on timeout)\n" \
	"\n"                                                    \
	USAGE_DTLS                                              \
	"\n"                                                    \
	"    auth_mode=%%s        default: (library default: none)\n"      \
	"                        options: none, optional, required\n" \
	USAGE_IO                                                \
	"\n"                                                    \
	USAGE_PSK                                               \
	USAGE_ECJPAKE                                           \
	"\n"                                                    \
	"    allow_legacy=%%d     default: (library default: no)\n"      \
	USAGE_RENEGO                                            \
	"    exchanges=%%d        default: 1\n"                 \
	"    reconnect=%%d        default: 0 (disabled)\n"      \
	"    reco_delay=%%d       default: 0 seconds\n"         \
	"    reconnect_hard=%%d   default: 0 (disabled)\n"      \
	USAGE_TICKETS                                           \
	USAGE_MAX_FRAG_LEN                                      \
	USAGE_TRUNC_HMAC                                        \
	USAGE_ALPN                                              \
	USAGE_FALLBACK                                          \
	USAGE_EMS                                               \
	USAGE_ETM                                               \
	USAGE_RECSPLIT                                          \
	USAGE_DHMLEN                                            \
	USAGE_LARGEDATA                                         \
	"\n"                                                    \
	"    arc4=%%d             default: (library default: 0)\n" \
	"    min_version=%%s      default: (library default: tls1)\n"       \
	"    max_version=%%s      default: (library default: tls1_2)\n"     \
	"    force_version=%%s    default: \"\" (none)\n"       \
	"                        options: ssl3, tls1, tls1_1, tls1_2, dtls1, dtls1_2\n" \
	"\n"                                                    \
	"    force_ciphersuite=<name>    default: all enabled\n"\
	" acceptable ciphersuite names:\n"

#define MAX_DATA_SIZE 100 * 1024 * 1024 /* Maximum transfer size is 100MB */

struct options {
	const char *server_name;	/* hostname of the server (client only)     */
	const char *server_addr;	/* address of the server (client only)      */
	const char *server_port;	/* port on which the ssl service runs       */
	int debug_level;			/* level of debugging                       */
	int nbio;					/* should I/O be blocking?                  */
	uint32_t read_timeout;		/* timeout on mbedtls_ssl_read() in milliseconds    */
	int max_resend;				/* DTLS times to resend on read timeout     */
	const char *request_page;	/* page on server to request                */
	int request_size;			/* pad request with header to requested size */
	const char *ca_file;		/* the file with the CA certificate(s)      */
	const char *ca_path;		/* the path with the CA certificate(s) reside */
	const char *crt_file;		/* the file with the client certificate     */
	const char *key_file;		/* the file with the client key             */
	const char *psk;			/* the pre-shared key                       */
	const char *psk_identity;	/* the pre-shared key identity              */
	const char *ecjpake_pw;		/* the EC J-PAKE password                   */
	int force_ciphersuite[2];	/* protocol/ciphersuite to use, or all      */
	int renegotiation;			/* enable / disable renegotiation           */
	int allow_legacy;			/* allow legacy renegotiation               */
	int renegotiate;			/* attempt renegotiation?                   */
	int renego_delay;			/* delay before enforcing renegotiation     */
	int exchanges;				/* number of data exchanges                 */
	int min_version;			/* minimum protocol version accepted        */
	int max_version;			/* maximum protocol version accepted        */
	int arc4;					/* flag for arc4 suites support             */
	int auth_mode;				/* verify mode for connection               */
	unsigned char mfl_code;		/* code for maximum fragment length         */
	int trunc_hmac;				/* negotiate truncated hmac or not          */
	int recsplit;				/* enable record splitting?                 */
	int dhmlen;					/* minimum DHM params len in bits           */
	int reconnect;				/* attempt to resume session                */
	int reco_delay;				/* delay in seconds before resuming session */
	int reconnect_hard;			/* unexpectedly reconnect from the same port */
	int tickets;				/* enable / disable session tickets         */
	const char *alpn_string;	/* ALPN supported protocols                 */
	int transport;				/* TLS or DTLS?                             */
	uint32_t hs_to_min;			/* Initial value of DTLS handshake timer    */
	uint32_t hs_to_max;			/* Max value of DTLS handshake timer        */
	int fallback;				/* is this a fallback connection?           */
	int extended_ms;			/* negotiate extended master secret?        */
	int etm;					/* negotiate encrypt then mac?              */
	int aging;                  /* enable the aging test when value is larger than 0*/
};



static struct options opt;

static void my_debug(void *ctx, int level, const char *file, int line, const char *str)
{
	const char *p;
	const char *basename;

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++)
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}

	mbedtls_fprintf((FILE *)ctx, "%s:%04d: |%d| %s", basename, line, level, str);
	fflush((FILE *)ctx);
}

/*
 * Test recv/send functions that make sure each try returns
 * WANT_READ/WANT_WRITE at least once before sucesseding
 */
static int my_recv(void *ctx, unsigned char *buf, size_t len)
{
	static int first_try = 1;
	int ret;

	if (first_try) {
		first_try = 0;
		return (MBEDTLS_ERR_SSL_WANT_READ);
	}

	ret = mbedtls_net_recv(ctx, buf, len);
	if (ret != MBEDTLS_ERR_SSL_WANT_READ) {
		first_try = 1;			/* Next call will be a new operation */
	}
	return (ret);
}

static int my_send(void *ctx, const unsigned char *buf, size_t len)
{
	static int first_try = 1;
	int ret;

	if (first_try) {
		first_try = 0;
		return (MBEDTLS_ERR_SSL_WANT_WRITE);
	}

	ret = mbedtls_net_send(ctx, buf, len);
	if (ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
		first_try = 1;			/* Next call will be a new operation */
	}
	return (ret);
}

#if defined(MBEDTLS_X509_CRT_PARSE_C)
/*
 * Enabled if debug_level > 1 in code below
 */
static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
	char buf[1024];
	((void)data);

	mbedtls_printf("\nVerify requested for (Depth %d):\n", depth);
	mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
	mbedtls_printf("%s", buf);

	if ((*flags) == 0) {
		mbedtls_printf("  This certificate has no flags\n");
	} else {
		mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
		mbedtls_printf("%s\n", buf);
	}

	return (0);
}
#endif							/* MBEDTLS_X509_CRT_PARSE_C */

/****************************************************************************
 * tls_client_main
 ****************************************************************************/

int tls_client_cb(void *args)
{
	int argc;
	char **argv;

	int ret = 0;
	int len;
	int tail_len;
	int i;
	int written;
	int frags = 0;
	int retry_left;
	mbedtls_net_context server_fd;
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	unsigned char psk[MBEDTLS_PSK_MAX_LEN];
	size_t psk_len = 0;
#endif
#if defined(MBEDTLS_SSL_ALPN)
	const char *alpn_list[10];
#endif
	const char *pers = "ssl_client2";

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_ssl_session saved_session;
#if defined(MBEDTLS_TIMING_C)
	mbedtls_timing_delay_context timer;
#endif
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	uint32_t flags;
	mbedtls_x509_crt cacert;
	mbedtls_x509_crt clicert;
	mbedtls_pk_context pkey;
#endif

	char *p;
	char *q;
	const int *list;
	unsigned char buf[2000];

	argc = ((struct pthread_arg *)args)->argc;
	argv = ((struct pthread_arg *)args)->argv;

	/*
	 * Make sure memory references are valid.
	 */
	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_entropy_init(&entropy);
	mbedtls_ssl_config_init(&conf);
	memset(&saved_session, 0, sizeof(mbedtls_ssl_session));
	mbedtls_ctr_drbg_init(&ctr_drbg);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_init(&cacert);
	mbedtls_x509_crt_init(&clicert);
	mbedtls_pk_init(&pkey);
#endif
#if defined(MBEDTLS_SSL_ALPN)
	memset((void *)alpn_list, 0, sizeof(alpn_list));
#endif

	if (argc == 0) {
usage:
		if (ret == 0) {
			ret = 1;
		}

		mbedtls_printf(USAGE);

		list = mbedtls_ssl_list_ciphersuites();
		while (*list) {
			mbedtls_printf(" %-42s", mbedtls_ssl_get_ciphersuite_name(*list));
			list++;
			if (!*list) {
				break;
			}
			mbedtls_printf(" %s\n", mbedtls_ssl_get_ciphersuite_name(*list));
			list++;
		}
		mbedtls_printf("\n");
		goto exit;
	}

	opt.server_name = DFL_SERVER_NAME;
	opt.server_addr = DFL_SERVER_ADDR;
	opt.server_port = DFL_SERVER_PORT;
	opt.debug_level = DFL_DEBUG_LEVEL;
	opt.nbio = DFL_NBIO;
	opt.read_timeout = DFL_READ_TIMEOUT;
	opt.max_resend = DFL_MAX_RESEND;
	opt.request_page = DFL_REQUEST_PAGE;
	opt.request_size = DFL_REQUEST_SIZE;
	opt.ca_file = DFL_CA_FILE;
	opt.ca_path = DFL_CA_PATH;
	opt.crt_file = DFL_CRT_FILE;
	opt.key_file = DFL_KEY_FILE;
	opt.psk = DFL_PSK;
	opt.psk_identity = DFL_PSK_IDENTITY;
	opt.ecjpake_pw = DFL_ECJPAKE_PW;
	opt.force_ciphersuite[0] = DFL_FORCE_CIPHER;
	opt.renegotiation = DFL_RENEGOTIATION;
	opt.allow_legacy = DFL_ALLOW_LEGACY;
	opt.renegotiate = DFL_RENEGOTIATE;
	opt.exchanges = DFL_EXCHANGES;
	opt.min_version = DFL_MIN_VERSION;
	opt.max_version = DFL_MAX_VERSION;
	opt.arc4 = DFL_ARC4;
	opt.auth_mode = DFL_AUTH_MODE;
	opt.mfl_code = DFL_MFL_CODE;
	opt.trunc_hmac = DFL_TRUNC_HMAC;
	opt.recsplit = DFL_RECSPLIT;
	opt.dhmlen = DFL_DHMLEN;
	opt.reconnect = DFL_RECONNECT;
	opt.reco_delay = DFL_RECO_DELAY;
	opt.reconnect_hard = DFL_RECONNECT_HARD;
	opt.tickets = DFL_TICKETS;
	opt.alpn_string = DFL_ALPN_STRING;
	opt.transport = DFL_TRANSPORT;
	opt.hs_to_min = DFL_HS_TO_MIN;
	opt.hs_to_max = DFL_HS_TO_MAX;
	opt.fallback = DFL_FALLBACK;
	opt.extended_ms = DFL_EXTENDED_MS;
	opt.etm = DFL_ETM;
	opt.aging = -1;

	for (i = 1; i < argc; i++) {
		p = argv[i];
		if ((q = strchr(p, '=')) == NULL) {
			goto usage;
		}
		*q++ = '\0';

		if (strncmp(p, "server_name", strlen("server_name") + 1) == 0) {
			opt.server_name = q;
		} else if (strncmp(p, "server_addr", strlen("server_addr") + 1) == 0) {
			opt.server_addr = q;
		} else if (strncmp(p, "server_port", strlen("server_port") + 1) == 0) {
			opt.server_port = q;
		} else if (strncmp(p, "dtls", strlen("dtls") + 1) == 0) {
			int t = atoi(q);
			if (t == 0) {
				opt.transport = MBEDTLS_SSL_TRANSPORT_STREAM;
			} else if (t == 1) {
				opt.transport = MBEDTLS_SSL_TRANSPORT_DATAGRAM;
			} else {
				goto usage;
			}
		} else if (strncmp(p, "debug_level", strlen("debug_level") + 1) == 0) {
			opt.debug_level = atoi(q);
			if (opt.debug_level < 0 || opt.debug_level > 65535) {
				goto usage;
			}
		} else if (strncmp(p, "nbio", strlen("nbio") + 1) == 0) {
			opt.nbio = atoi(q);
			if (opt.nbio < 0 || opt.nbio > 2) {
				goto usage;
			}
		} else if (strncmp(p, "read_timeout", strlen("read_timeout") + 1) == 0) {
			opt.read_timeout = atoi(q);
		} else if (strncmp(p, "max_resend", strlen("max_resend") + 1) == 0) {
			opt.max_resend = atoi(q);
			if (opt.max_resend < 0) {
				goto usage;
			}
		} else if (strncmp(p, "request_page", strlen("request_page") + 1) == 0) {
			opt.request_page = q;
		} else if (strncmp(p, "request_size", strlen("request_size") + 1) == 0) {
			opt.request_size = atoi(q);
			if (opt.request_size < 0 || opt.request_size > MBEDTLS_SSL_OUT_CONTENT_LEN) {
				goto usage;
			}
		} else if (strncmp(p, "ca_file", strlen("ca_file") + 1) == 0) {
			opt.ca_file = q;
		} else if (strncmp(p, "ca_path", strlen("ca_path") + 1) == 0) {
			opt.ca_path = q;
		} else if (strncmp(p, "crt_file", strlen("crt_file") + 1) == 0) {
			opt.crt_file = q;
		} else if (strncmp(p, "key_file", strlen("key_file") + 1) == 0) {
			opt.key_file = q;
		} else if (strncmp(p, "psk", strlen("psk") + 1) == 0) {
			opt.psk = q;
		} else if (strncmp(p, "psk_identity", strlen("psk_identity") + 1) == 0) {
			opt.psk_identity = q;
		} else if (strncmp(p, "ecjpake_pw", strlen("ecjpake_pw") + 1) == 0) {
			opt.ecjpake_pw = q;
		} else if (strncmp(p, "force_ciphersuite", strlen("force_ciphersuite") + 1) == 0) {
			opt.force_ciphersuite[0] = mbedtls_ssl_get_ciphersuite_id(q);

			if (opt.force_ciphersuite[0] == 0) {
				ret = 2;
				goto usage;
			}
			opt.force_ciphersuite[1] = 0;
		} else if (strncmp(p, "renegotiation", strlen("renegotiation") + 1) == 0) {
			opt.renegotiation = (atoi(q)) ? MBEDTLS_SSL_RENEGOTIATION_ENABLED : MBEDTLS_SSL_RENEGOTIATION_DISABLED;
		} else if (strncmp(p, "allow_legacy", strlen("allow_legacy") + 1) == 0) {
			switch (atoi(q)) {
			case -1:
				opt.allow_legacy = MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE;
				break;
			case 0:
				opt.allow_legacy = MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION;
				break;
			case 1:
				opt.allow_legacy = MBEDTLS_SSL_LEGACY_ALLOW_RENEGOTIATION;
				break;
			default:
				goto usage;
			}
		} else if (strncmp(p, "renegotiate", strlen("renegotiate") + 1) == 0) {
			opt.renegotiate = atoi(q);
			if (opt.renegotiate < 0 || opt.renegotiate > 1) {
				goto usage;
			}
		} else if (strncmp(p, "exchanges", strlen("exchanges") + 1) == 0) {
			opt.exchanges = atoi(q);
			if (opt.exchanges < 1) {
				goto usage;
			}
		} else if (strncmp(p, "reconnect", strlen("reconnect") + 1) == 0) {
			opt.reconnect = atoi(q);
			if (opt.reconnect < 0 || opt.reconnect > 2) {
				goto usage;
			}
		} else if (strncmp(p, "reco_delay", strlen("reco_delay") + 1) == 0) {
			opt.reco_delay = atoi(q);
			if (opt.reco_delay < 0) {
				goto usage;
			}
		} else if (strncmp(p, "reconnect_hard", strlen("reconnect_hard") + 1) == 0) {
			opt.reconnect_hard = atoi(q);
			if (opt.reconnect_hard < 0 || opt.reconnect_hard > 1) {
				goto usage;
			}
		} else if (strncmp(p, "tickets", strlen("tickets") + 1) == 0) {
			opt.tickets = atoi(q);
			if (opt.tickets < 0 || opt.tickets > 2) {
				goto usage;
			}
		} else if (strncmp(p, "alpn", strlen("alpn") + 1) == 0) {
			opt.alpn_string = q;
		} else if (strncmp(p, "extended_ms", strlen("extended_ms") + 1) == 0) {
			switch (atoi(q)) {
			case 0:
				opt.extended_ms = MBEDTLS_SSL_EXTENDED_MS_DISABLED;
				break;
			case 1:
				opt.extended_ms = MBEDTLS_SSL_EXTENDED_MS_ENABLED;
				break;
			default:
				goto usage;
			}
		} else if (strncmp(p, "etm", strlen("etm") + 1) == 0) {
			switch (atoi(q)) {
			case 0:
				opt.etm = MBEDTLS_SSL_ETM_DISABLED;
				break;
			case 1:
				opt.etm = MBEDTLS_SSL_ETM_ENABLED;
				break;
			default:
				goto usage;
			}
		} else if (strncmp(p, "min_version", strlen("min_version") + 1) == 0) {
			if (strncmp(q, "tls12", strlen("tls12") + 1) == 0 || strncmp(q, "dtls12", strlen("dtls12")) == 0) {
				opt.min_version = MBEDTLS_SSL_VERSION_TLS1_2;
			}
#if defined(MBEDTLS_SSL_PROTO_TLS1_3)
			else if (strncmp(q, "tls13", strlen("tls13") + 1) == 0) {
				opt.min_version = MBEDTLS_SSL_VERSION_TLS1_3;
			}
#endif /* MBEDTLS_SSL_PROTO_TLS1_3 */
			else {
				goto usage;
			}
		} else if (strncmp(p, "max_version", strlen("max_version") + 1) == 0) {
			if (strncmp(q, "tls12", strlen("tls12") + 1) == 0 || strncmp(q, "dtls12", strlen("dtls12") + 1) == 0) {
					opt.max_version = MBEDTLS_SSL_VERSION_TLS1_2;
				}
	#if defined(MBEDTLS_SSL_PROTO_TLS1_3)
			else if (strncmp(q, "tls13", strlen("tls13") + 1) == 0) {
				opt.max_version = MBEDTLS_SSL_VERSION_TLS1_3;
			}
	#endif /* MBEDTLS_SSL_PROTO_TLS1_3 */
			else {
				goto usage;
			}
		} else if (strncmp(p, "force_version", strlen("force_version") + 1) == 0) {
			if (strncmp(q, "tls12", strlen("tls12") + 1) == 0) {
				opt.min_version = MBEDTLS_SSL_VERSION_TLS1_2;
				opt.max_version = MBEDTLS_SSL_VERSION_TLS1_2;
			} else if (strncmp(q, "dtls12", strlen("dtls12") + 1) == 0) {
				opt.min_version = MBEDTLS_SSL_VERSION_TLS1_2;
				opt.max_version = MBEDTLS_SSL_VERSION_TLS1_2;
				opt.transport = MBEDTLS_SSL_TRANSPORT_DATAGRAM;
			}
#if defined(MBEDTLS_SSL_PROTO_TLS1_3)
			else if (strncmp(q, "tls13", strlen("tls13") + 1) == 0) {
				opt.min_version = MBEDTLS_SSL_VERSION_TLS1_3;
				opt.max_version = MBEDTLS_SSL_VERSION_TLS1_3;
			}
#endif /* MBEDTLS_SSL_PROTO_TLS1_3 */
			else {
				goto usage;
			}
		} else if (strncmp(p, "auth_mode", strlen("auth_mode") + 1) == 0) {
			if (strncmp(q, "none", strlen("none") + 1) == 0) {
				opt.auth_mode = MBEDTLS_SSL_VERIFY_NONE;
			} else if (strncmp(q, "optional", strlen("optional") + 1) == 0) {
				opt.auth_mode = MBEDTLS_SSL_VERIFY_OPTIONAL;
			} else if (strncmp(q, "required", strlen("required") + 1) == 0) {
				opt.auth_mode = MBEDTLS_SSL_VERIFY_REQUIRED;
			} else {
				goto usage;
			}
		} else if (strncmp(p, "max_frag_len", strlen("max_frag_len") + 1) == 0) {
			if (strncmp(q, "512", strlen("512") + 1) == 0) {
				opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_512;
			} else if (strncmp(q, "1024", strlen("1024") + 1) == 0) {
				opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_1024;
			} else if (strncmp(q, "2048", strlen("2048") + 1) == 0) {
				opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_2048;
			} else if (strncmp(q, "4096", strlen("4096") + 1) == 0) {
				opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_4096;
			} else {
				goto usage;
			}
		} else if (strncmp(p, "trunc_hmac", strlen("trunc_hmac") + 1) == 0) {
			switch (atoi(q)) {
			case 0:
				opt.trunc_hmac = MBEDTLS_SSL_TRUNC_HMAC_DISABLED;
				break;
			case 1:
				opt.trunc_hmac = MBEDTLS_SSL_TRUNC_HMAC_ENABLED;
				break;
			default:
				goto usage;
			}
		} else if (strncmp(p, "hs_timeout", strlen("hs_timeout") + 1) == 0) {
			if ((p = strchr(q, '-')) == NULL) {
				goto usage;
			}
			*p++ = '\0';
			opt.hs_to_min = atoi(q);
			opt.hs_to_max = atoi(p);
			if (opt.hs_to_min == 0 || opt.hs_to_max < opt.hs_to_min) {
				goto usage;
			}
		} else if (strncmp(p, "recsplit", strlen("recsplit") + 1) == 0) {
			opt.recsplit = atoi(q);
			if (opt.recsplit < 0 || opt.recsplit > 1) {
				goto usage;
			}
		} else if (strncmp(p, "dhmlen", strlen("dhmlen") + 1) == 0) {
			opt.dhmlen = atoi(q);
			if (opt.dhmlen < 0) {
				goto usage;
			}
		} else if (strncmp(p, "aging", strlen("aging") + 1) == 0) {
			opt.aging = atoi(q);
			if (opt.aging <= 0 && opt.aging > MAX_DATA_SIZE) {
				goto usage;
			}
		} else {
			goto usage;
		}
	}

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(opt.debug_level);
#endif

	if (opt.force_ciphersuite[0] > 0) {
		const mbedtls_ssl_ciphersuite_t *ciphersuite_info;
		ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(opt.force_ciphersuite[0]);

		if (opt.max_version != -1 && ciphersuite_info->min_tls_version > opt.max_version) {
			mbedtls_printf("forced ciphersuite not allowed with this protocol version\n");
			ret = 2;
			goto usage;
		}
		if (opt.min_version != -1 && ciphersuite_info->max_tls_version < opt.min_version) {
			mbedtls_printf("forced ciphersuite not allowed with this protocol version\n");
			ret = 2;
			goto usage;
		}

		/* If the server selects a version that's not supported by
		 * this suite, then there will be no common ciphersuite... */
		if (opt.max_version == -1 || opt.max_version > ciphersuite_info->max_tls_version) {
			opt.max_version = ciphersuite_info->max_tls_version;
		}
		if (opt.min_version < ciphersuite_info->min_tls_version) {
			opt.min_version = ciphersuite_info->min_tls_version;
			/* DTLS starts with TLS 1.1 */
			if (opt.transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM && opt.min_version < MBEDTLS_SSL_VERSION_TLS1_2) {
				opt.min_version = MBEDTLS_SSL_VERSION_TLS1_2;
			}
		}

#if defined(MBEDTLS_USE_PSA_CRYPTO)
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
		if (opt.psk_opaque != 0) {
			/* Determine KDF algorithm the opaque PSK will be used in. */
#if defined(MBEDTLS_HAS_ALG_SHA_384_VIA_MD_OR_PSA_BASED_ON_USE_PSA)
			if (ciphersuite_info->mac == MBEDTLS_MD_SHA384) {
				alg = PSA_ALG_TLS12_PSK_TO_MS(PSA_ALG_SHA_384);
			} else
#endif /* MBEDTLS_HAS_ALG_SHA_384_VIA_MD_OR_PSA_BASED_ON_USE_PSA */
			alg = PSA_ALG_TLS12_PSK_TO_MS(PSA_ALG_SHA_256);
		}
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED */
#endif /* MBEDTLS_USE_PSA_CRYPTO */
	}

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	/*
	 * Unhexify the pre-shared key if any is given
	 */
	if (strlen(opt.psk)) {
		unsigned char c;
		size_t j;

		if (strlen(opt.psk) % 2 != 0) {
			mbedtls_printf("pre-shared key not valid hex\n");
			goto exit;
		}

		psk_len = strlen(opt.psk) / 2;

		for (j = 0; j < strlen(opt.psk); j += 2) {
			c = opt.psk[j];
			if (c >= '0' && c <= '9') {
				c -= '0';
			} else if (c >= 'a' && c <= 'f') {
				c -= 'a' - 10;
			} else if (c >= 'A' && c <= 'F') {
				c -= 'A' - 10;
			} else {
				mbedtls_printf("pre-shared key not valid hex\n");
				goto exit;
			}
			psk[j / 2] = c << 4;

			c = opt.psk[j + 1];
			if (c >= '0' && c <= '9') {
				c -= '0';
			} else if (c >= 'a' && c <= 'f') {
				c -= 'a' - 10;
			} else if (c >= 'A' && c <= 'F') {
				c -= 'A' - 10;
			} else {
				mbedtls_printf("pre-shared key not valid hex\n");
				goto exit;
			}
			psk[j / 2] |= c;
		}
	}
#endif							/* MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED */

#if defined(MBEDTLS_SSL_ALPN)
	if (opt.alpn_string != NULL) {
		p = (char *)opt.alpn_string;
		i = 0;

		/* Leave room for a final NULL in alpn_list */
		while ((i < sizeof(alpn_list) / sizeof(alpn_list[0]) - 1)
			   && (*p != '\0')) {
			alpn_list[i++] = p;

			/* Terminate the current string and move on to next one */
			while (*p != ',' && *p != '\0') {
				p++;
			}
			if (*p == ',') {
				*p++ = '\0';
			}
		}
	}
#endif							/* MBEDTLS_SSL_ALPN */

	/*
	 * 0. Initialize the RNG and the session data
	 */
	mbedtls_printf("\n  . Seeding the random number generator...");
	fflush(stdout);

	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n", -ret);
		goto exit;
	}

	mbedtls_printf("ok\n");

#ifdef CONFIG_EXAMPLES_TLS_ARTIK_KEY
	unsigned int cert_buflen = SEE_MAX_BUF_SIZE;
	char *cert_buf = NULL;

	cert_buf = (char *)malloc(SEE_MAX_BUF_SIZE);

	/*
	 * 1. Load own certificate from secure storage.
	 *    ARTIK certificates chain stored in secure storage. (CA - SubCA - Device)
	 */

	if ((ret = see_get_certificate((unsigned char *)cert_buf, &cert_buflen, FACTORYKEY_ARTIK_CERT, 0)) != 0) {
		mbedtls_printf(" failed\n  ! see_get_certificate -0x%x\n", -ret);
		free(cert_buf);
		goto exit;
	}

	char *cert_offset[3] = {NULL, NULL, NULL};
	int cert_length[3] = {0, 0, 0};

	cert_offset[0] = cert_buf;
	cert_length[0] = (cert_offset[0][2] << 8) + cert_offset[0][3] + 4;
	cert_offset[1] = cert_offset[0] + cert_length[0];
	cert_length[1] = (cert_offset[1][2] << 8) + cert_offset[1][3] + 4;
	cert_offset[2] = cert_offset[1] + cert_length[1];
	cert_length[2] = (cert_offset[2][2] << 8) + cert_offset[2][3] + 4;

	/* Parse CA Cert */
	if ((ret = mbedtls_x509_crt_parse_der(&cacert, (const unsigned char *)cert_offset[0], cert_length[0])) < 0) {
		mbedtls_printf(" failed\n  ! mbedtls_x509_crt_parse CA cert -0x%x\n", -ret);
		free(cert_buf);
		goto exit;
	}

	/* Parse Device Cert */
	if ((ret = mbedtls_x509_crt_parse_der(&clicert, (const unsigned char *)cert_offset[2], cert_length[2])) < 0) {
		mbedtls_printf(" failed\n  ! mbedtls_x509_crt_parse Device cert -0x%x\n", -ret);
		free(cert_buf);
		goto exit;
	}
	free(cert_buf);

	/*
	 * 2. Set the key index
	 */
	const mbedtls_pk_info_t *pk_info;

	if ((pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == NULL) {
		mbedtls_printf(" failed\n  ! mbedtls_pk_info_from_type -0x%x\n", -ret);
		goto exit;
	}

	if ((ret = mbedtls_pk_setup(&pkey, pk_info)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_pk_setup -0x%x\n", -ret);
		goto exit;
	}

	((mbedtls_ecdsa_context *)(pkey.pk_ctx))->grp.id = MBEDTLS_ECP_DP_SECP256R1;
	((mbedtls_ecdsa_context *)(pkey.pk_ctx))->key_index = FACTORYKEY_ARTIK_DEVICE;

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	if ((ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
		goto exit;
	}
#endif
	mbedtls_printf(" ok (%d skipped)\n", ret);
#else
	/*
	 * 1. Load the trusted CA
	 */
	mbedtls_printf("  . Loading the CA root certificate ...");
	fflush(stdout);

	for (i = 0; mbedtls_test_cas[i] != NULL; i++) {
		ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *)mbedtls_test_cas[i], mbedtls_test_cas_len[i]);

		if (ret != 0) {
			break;
		}
	}

	mbedtls_printf(" ok (%d skipped)\n", ret);

	/*
	 * 1.2. Load own certificate and key
	 */
	mbedtls_printf("  . Loading the own cert...");
	fflush(stdout);

	if ((ret = mbedtls_x509_crt_parse(&clicert, (const unsigned char *)mbedtls_test_cli_crt, mbedtls_test_cli_crt_len)) != 0) {
		mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

	mbedtls_printf("  . Loading the Private Key...");
	fflush(stdout);

	if ((ret = mbedtls_pk_parse_key(&pkey, (const unsigned char *)mbedtls_test_cli_key, mbedtls_test_cli_key_len, NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
		mbedtls_printf(" failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret);
		goto exit;
	}
	/*
	 * 2. Start the connection
	 */
	if (opt.server_addr == NULL) {
		opt.server_addr = opt.server_name;
	}
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	if ((ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
		goto exit;
	}
#endif
#endif							/* EXAMPLES_ARTIK_KEY */

	mbedtls_printf("  . Connecting to %s/%s/%s...", opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM ? "tcp" : "udp", opt.server_addr, opt.server_port);
	fflush(stdout);

	if ((ret = mbedtls_net_connect(&server_fd, opt.server_addr, opt.server_port, opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM ? MBEDTLS_NET_PROTO_TCP : MBEDTLS_NET_PROTO_UDP)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_net_connect returned -0x%x\n\n", -ret);
		goto exit;
	}

	if (opt.nbio > 0) {
		ret = mbedtls_net_set_nonblock(&server_fd);
	} else {
		ret = mbedtls_net_set_block(&server_fd);
	}
	if (ret != 0) {
		mbedtls_printf(" failed\n  ! net_set_(non)block() returned -0x%x\n\n", -ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

	/*
	 * 3. Setup stuff
	 */
	mbedtls_printf("  . Setting up the SSL/TLS structure...");
	fflush(stdout);

	if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, opt.transport, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret);
		goto exit;
	}
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (opt.debug_level > 0) {
		mbedtls_ssl_conf_verify(&conf, my_verify, NULL);
	}
#endif

	if (opt.auth_mode != DFL_AUTH_MODE) {
		mbedtls_ssl_conf_authmode(&conf, opt.auth_mode);
	}
#if defined(MBEDTLS_SSL_PROTO_DTLS)
	if (opt.hs_to_min != DFL_HS_TO_MIN || opt.hs_to_max != DFL_HS_TO_MAX) {
		mbedtls_ssl_conf_handshake_timeout(&conf, opt.hs_to_min, opt.hs_to_max);
	}
#endif							/* MBEDTLS_SSL_PROTO_DTLS */

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
	if ((ret = mbedtls_ssl_conf_max_frag_len(&conf, opt.mfl_code)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_max_frag_len returned %d\n\n", ret);
		goto exit;
	}
#endif

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC)
	if (opt.trunc_hmac != DFL_TRUNC_HMAC) {
		mbedtls_ssl_conf_truncated_hmac(&conf, opt.trunc_hmac);
	}
#endif

#if defined(MBEDTLS_SSL_EXTENDED_MASTER_SECRET)
	if (opt.extended_ms != DFL_EXTENDED_MS) {
		mbedtls_ssl_conf_extended_master_secret(&conf, opt.extended_ms);
	}
#endif

#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
	if (opt.etm != DFL_ETM) {
		mbedtls_ssl_conf_encrypt_then_mac(&conf, opt.etm);
	}
#endif

#if defined(MBEDTLS_SSL_CBC_RECORD_SPLITTING)
	if (opt.recsplit != DFL_RECSPLIT) {
		mbedtls_ssl_conf_cbc_record_splitting(&conf, opt.recsplit ? MBEDTLS_SSL_CBC_RECORD_SPLITTING_ENABLED : MBEDTLS_SSL_CBC_RECORD_SPLITTING_DISABLED);
	}
#endif

#if defined(MBEDTLS_DHM_C)
	if (opt.dhmlen != DFL_DHMLEN) {
		mbedtls_ssl_conf_dhm_min_bitlen(&conf, opt.dhmlen);
	}
#endif

#if defined(MBEDTLS_SSL_ALPN)
	if (opt.alpn_string != NULL)
		if ((ret = mbedtls_ssl_conf_alpn_protocols(&conf, alpn_list)) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_alpn_protocols returned %d\n\n", ret);
			goto exit;
		}
#endif

	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

	mbedtls_ssl_conf_read_timeout(&conf, opt.read_timeout);

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
	mbedtls_ssl_conf_session_tickets(&conf, opt.tickets);
#endif

	if (opt.force_ciphersuite[0] != DFL_FORCE_CIPHER) {
		mbedtls_ssl_conf_ciphersuites(&conf, opt.force_ciphersuite);
	}
#if defined(MBEDTLS_ARC4_C)
	if (opt.arc4 != DFL_ARC4) {
		mbedtls_ssl_conf_arc4_support(&conf, opt.arc4);
	}
#endif

	if (opt.allow_legacy != DFL_ALLOW_LEGACY) {
		mbedtls_ssl_conf_legacy_renegotiation(&conf, opt.allow_legacy);
	}
#if defined(MBEDTLS_SSL_RENEGOTIATION)
	mbedtls_ssl_conf_renegotiation(&conf, opt.renegotiation);
#endif

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	if (psk_len) {
		if ((ret = mbedtls_ssl_conf_psk(&conf, psk, psk_len, (const unsigned char *)opt.psk_identity, strlen(opt.psk_identity))) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_psk returned %d\n\n", ret);
			goto exit;
		}
	}
#endif

	if (opt.min_version != DFL_MIN_VERSION) {
		mbedtls_ssl_conf_min_version(&conf, MBEDTLS_SSL_MAJOR_VERSION_3, opt.min_version);
	}

	if (opt.max_version != DFL_MAX_VERSION) {
		mbedtls_ssl_conf_max_version(&conf, MBEDTLS_SSL_MAJOR_VERSION_3, opt.max_version);
	}
#if defined(MBEDTLS_SSL_FALLBACK_SCSV)
	if (opt.fallback != DFL_FALLBACK) {
		mbedtls_ssl_conf_fallback(&conf, opt.fallback);
	}
#endif

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned -0x%x\n\n", -ret);
		goto exit;
	}
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if ((ret = mbedtls_ssl_set_hostname(&ssl, opt.server_name)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
		goto exit;
	}
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
	if (opt.ecjpake_pw != DFL_ECJPAKE_PW) {
		if ((ret = mbedtls_ssl_set_hs_ecjpake_password(&ssl, (const unsigned char *)opt.ecjpake_pw, strlen(opt.ecjpake_pw))) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hs_ecjpake_password returned %d\n\n", ret);
			goto exit;
		}
	}
#endif

	if (opt.nbio == 2) {
		mbedtls_ssl_set_bio(&ssl, &server_fd, my_send, my_recv, NULL);
	} else {
		mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, opt.nbio == 0 ? mbedtls_net_recv_timeout : NULL);
	}

#if defined(MBEDTLS_TIMING_C)
	mbedtls_ssl_set_timer_cb(&ssl, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);
#endif
	mbedtls_printf(" ok\n");

	/*
	 * 4. Handshake
	 */
	mbedtls_printf("  . Performing the SSL/TLS handshake...");
	fflush(stdout);

	while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n", -ret);
			if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
				mbedtls_printf("    Unable to verify the server's certificate. " "Either it is invalid,\n" "    or you didn't set ca_file or ca_path " "to an appropriate value.\n" "    Alternatively, you may want to use " "auth_mode=optional for testing purposes.\n");
			}
			mbedtls_printf("\n");
			goto exit;
		}
	}

	mbedtls_printf(" ok\n    [ Protocol is %s ]\n    [ Ciphersuite is %s ]\n", mbedtls_ssl_get_version(&ssl), mbedtls_ssl_get_ciphersuite(&ssl));

	if ((ret = mbedtls_ssl_get_record_expansion(&ssl)) >= 0) {
		mbedtls_printf("    [ Record expansion is %d ]\n", ret);
	} else {
		mbedtls_printf("    [ Record expansion is unknown (compression) ]\n");
	}

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
	mbedtls_printf("    [ Maximum incoming record payload length is %u ]\n", (unsigned int) mbedtls_ssl_get_max_in_record_payload(&ssl));
	mbedtls_printf("    [ Maximum outgoing record payload length is %u ]\n", (unsigned int) mbedtls_ssl_get_max_out_record_payload(&ssl));
#endif

#if defined(MBEDTLS_SSL_ALPN)
	if (opt.alpn_string != NULL) {
		const char *alp = mbedtls_ssl_get_alpn_protocol(&ssl);
		mbedtls_printf("    [ Application Layer Protocol is %s ]\n", alp ? alp : "(none)");
	}
#endif

	if (opt.reconnect != 0) {
		mbedtls_printf("  . Saving session for reuse...");
		fflush(stdout);

		if ((ret = mbedtls_ssl_get_session(&ssl, &saved_session)) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_get_session returned -0x%x\n\n", -ret);
			goto exit;
		}

		mbedtls_printf(" ok\n");
	}

	/*
	 * 5. Verify the server certificate
	 */
	mbedtls_printf("  . Verifying peer X.509 certificate...");

	if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
		char vrfy_buf[512];

		mbedtls_printf(" failed\n");

		mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

		mbedtls_printf("%s\n", vrfy_buf);
	} else {
		mbedtls_printf(" ok\n");
	}

	if (mbedtls_ssl_get_peer_cert(&ssl) != NULL) {
		mbedtls_printf("  . Peer certificate information    ...\n");
		mbedtls_x509_crt_info((char *)buf, sizeof(buf) - 1, "      ", mbedtls_ssl_get_peer_cert(&ssl));
		mbedtls_printf("%s\n", buf);
	}
#if defined(MBEDTLS_SSL_RENEGOTIATION)
	if (opt.renegotiate) {
		/*
		 * Perform renegotiation (this must be done when the server is waiting
		 * for input from our side).
		 */
		mbedtls_printf("  . Performing renegotiation...");
		fflush(stdout);
		while ((ret = mbedtls_ssl_renegotiate(&ssl)) != 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				mbedtls_printf(" failed\n  ! mbedtls_ssl_renegotiate returned %d\n\n", ret);
				goto exit;
			}
		}
		mbedtls_printf(" ok\n");
	}
#endif							/* MBEDTLS_SSL_RENEGOTIATION */

	/*
	 * 6. Write the GET request
	 */
	retry_left = opt.max_resend;
send_request:
	if (opt.aging > 0) {
		mbedtls_printf("   start the aging test (%d)B sendbuf size(%d)B\n", opt.aging, sizeof(buf));
		int send_left = opt.aging;

		// send buf size
		unsigned int aging_size = htonl(opt.aging);
		ret = mbedtls_ssl_write(&ssl, (void *)&aging_size, sizeof(unsigned int));
		if (ret <= 0 && ret != MBEDTLS_ERR_SSL_WANT_READ &&
			ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_printf(" failed to send transfer size to the tls server(%x)\n", -ret);
			goto exit;
		}
		
		while (send_left > 0) {
			int datasize = send_left < MBEDTLS_SSL_OUT_CONTENT_LEN ? send_left : MBEDTLS_SSL_OUT_CONTENT_LEN;
			for (written = 0, frags = 0; written < datasize; written += ret, frags++) {
				int bufsize = datasize - written;
				while ((ret = mbedtls_ssl_write(&ssl, buf + written, bufsize)) <= 0) {
					if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
						ret != MBEDTLS_ERR_SSL_WANT_WRITE) {

						mbedtls_printf(" failed\n  ! mbedtls_ssl_write returned -0x%x\n\n", -ret);
						goto exit;
					}
				}
				mbedtls_printf(" sent (%d/%d)\n", ret, send_left);
			}
			send_left -= MBEDTLS_SSL_OUT_CONTENT_LEN;
		}
		mbedtls_printf(" transfer is done frags(%d)\n", frags);
		ret = 0;
		goto exit;
	}
		
	mbedtls_printf("  > Write to server:");
	fflush(stdout);

	len = mbedtls_snprintf((char *)buf, sizeof(buf) - 1, GET_REQUEST, opt.request_page);
	tail_len = (int)strlen(GET_REQUEST_END);

	/* Add padding to GET request to reach opt.request_size in length */
	if (opt.request_size != DFL_REQUEST_SIZE && len + tail_len < opt.request_size) {
		if (sizeof(buf) - len < opt.request_size - len - tail_len) {
			goto exit;
		}
		memset(buf + len, 'A', opt.request_size - len - tail_len);
		len += opt.request_size - len - tail_len;
	}

	if (len > sizeof(buf) - 1) {
		goto exit;
	}
	strncpy((char *)buf + len, GET_REQUEST_END, sizeof(buf) - len - 1);
	len += tail_len;

	/* Truncate if request size is smaller than the "natural" size */
	if (opt.request_size != DFL_REQUEST_SIZE && len > opt.request_size) {
		len = opt.request_size;

		if (len > sizeof(buf) - 1) {
			goto exit;
		}

		/* Still end with \r\n unless that's really not possible */
		if (len >= 2) {
			buf[len - 2] = '\r';
		}
		if (len >= 1) {
			buf[len - 1] = '\n';
		}
	}

	if (len > sizeof(buf) - 1) {
		goto exit;
	}

	if (opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM) {
		for (written = 0, frags = 0; written < len; written += ret, frags++) {
			while ((ret = mbedtls_ssl_write(&ssl, buf + written, len - written))
				   <= 0) {
				if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
					mbedtls_printf(" failed\n  ! mbedtls_ssl_write returned -0x%x\n\n", -ret);
					goto exit;
				}
			}
		}
	} else {					/* Not stream, so datagram */
		do {
			ret = mbedtls_ssl_write(&ssl, buf, len);
		} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

		if (ret < 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
			goto exit;
		}

		frags = 1;
		written = ret;
	}

	buf[written] = '\0';
	mbedtls_printf(" %d bytes written in %d fragments\n\n", written, frags);

	/*
	 * 7. Read the HTTP response
	 */
	mbedtls_printf("  < Read from server:");
	fflush(stdout);

	/*
	 * TLS and DTLS need different reading styles (stream vs datagram)
	 */
	if (opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM) {
		do {
			len = sizeof(buf) - 1;
			memset(buf, 0, sizeof(buf));
			ret = mbedtls_ssl_read(&ssl, buf, len);

			if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
				continue;
			}

			if (ret <= 0) {
				switch (ret) {
				case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
					mbedtls_printf(" connection was closed gracefully\n");
					ret = 0;
					goto close_notify;

				case 0:
				case MBEDTLS_ERR_NET_CONN_RESET:
					mbedtls_printf(" connection was reset by peer\n");
					ret = 0;
					goto reconnect;

				default:
					mbedtls_printf(" mbedtls_ssl_read returned -0x%x\n", -ret);
					goto exit;
				}
			}

			len = ret;
			buf[len] = '\0';
			mbedtls_printf(" %d bytes read\n\n%s", len, (char *)buf);

			/* End of message should be detected according to the syntax of the
			 * application protocol (eg HTTP), just use a dummy test here. */
			if (ret > 0 && buf[len - 1] == '\n') {
				break;
			}
		} while (1);
	} else {					/* Not stream, so datagram */
		len = sizeof(buf) - 1;
		memset(buf, 0, sizeof(buf));

		do {
			ret = mbedtls_ssl_read(&ssl, buf, len);
		} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

		if (ret <= 0) {
			switch (ret) {
			case MBEDTLS_ERR_SSL_TIMEOUT:
				mbedtls_printf(" timeout\n");
				if (retry_left-- > 0) {
					goto send_request;
				}
				goto exit;

			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				mbedtls_printf(" connection was closed gracefully\n");
				ret = 0;
				goto close_notify;

			default:
				mbedtls_printf(" mbedtls_ssl_read returned -0x%x\n", -ret);
				goto exit;
			}
		}

		len = ret;
		buf[len] = '\0';
		mbedtls_printf(" %d bytes read\n\n%s", len, (char *)buf);
		ret = 0;
	}

	/*
	 * 7b. Simulate hard reset and reconnect from same port?
	 */
	if (opt.reconnect_hard != 0) {
		opt.reconnect_hard = 0;

		mbedtls_printf("  . Restarting connection from same port...");
		fflush(stdout);

		if ((ret = mbedtls_ssl_session_reset(&ssl)) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_session_reset returned -0x%x\n\n", -ret);
			goto exit;
		}

		while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
				goto exit;
			}
		}

		mbedtls_printf(" ok\n");

		goto send_request;
	}

	/*
	 * 7c. Continue doing data exchanges?
	 */
	if (--opt.exchanges > 0) {
		goto send_request;
	}

	/*
	 * 8. Done, cleanly close the connection
	 */
close_notify:
	mbedtls_printf("  . Closing the connection...");
	fflush(stdout);

	/* No error checking, the connection might be closed already */
	do {
		ret = mbedtls_ssl_close_notify(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
	ret = 0;

	mbedtls_printf(" done\n");

	/*
	 * 9. Reconnect?
	 */
reconnect:
	if (opt.reconnect != 0) {
		--opt.reconnect;

		mbedtls_net_free(&server_fd);

#if defined(MBEDTLS_TIMING_C)
		if (opt.reco_delay > 0) {
			mbedtls_net_usleep(1000000 * opt.reco_delay);
		}
#endif

		mbedtls_printf("  . Reconnecting with saved session...");

		if ((ret = mbedtls_ssl_session_reset(&ssl)) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_session_reset returned -0x%x\n\n", -ret);
			goto exit;
		}

		if ((ret = mbedtls_ssl_set_session(&ssl, &saved_session)) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_session returned %d\n\n", ret);
			goto exit;
		}

		if ((ret = mbedtls_net_connect(&server_fd, opt.server_addr, opt.server_port, opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM ? MBEDTLS_NET_PROTO_TCP : MBEDTLS_NET_PROTO_UDP)) != 0) {
			mbedtls_printf(" failed\n  ! mbedtls_net_connect returned -0x%x\n\n", -ret);
			goto exit;
		}

		if (server_fd.fd < 0) {
			goto exit;
		}

		if (opt.nbio > 0) {
			ret = mbedtls_net_set_nonblock(&server_fd);
		} else {
			ret = mbedtls_net_set_block(&server_fd);
		}
		if (ret != 0) {
			mbedtls_printf(" failed\n  ! net_set_(non)block() returned -0x%x\n\n", -ret);
			goto exit;
		}

		while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
				goto exit;
			}
		}

		mbedtls_printf(" ok\n");

		goto send_request;
	}

	/*
	 * Cleanup and exit
	 */
exit:
#ifdef MBEDTLS_ERROR_C
	if (ret != 0) {
		char error_buf[100];
		mbedtls_strerror(ret, error_buf, 100);
		mbedtls_printf("Last error was: -0x%X - %s\n\n", -ret, error_buf);
	}
#endif
	mbedtls_net_free(&server_fd);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_free(&clicert);
	mbedtls_x509_crt_free(&cacert);
	mbedtls_pk_free(&pkey);
#endif
	mbedtls_ssl_session_free(&saved_session);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

#if defined(_WIN32)
	mbedtls_printf("  + Press Enter to exit this program.\n");
	fflush(stdout);
	getchar();
#endif

	return ret;
}

int tls_client_main(int argc, char **argv)
{
	pthread_t tid;
	pthread_attr_t attr;
	struct sched_param sparam;
	int r;

	struct pthread_arg args;
	args.argc = argc;
	args.argv = argv;

	/* Initialize the attribute variable */
	if ((r = pthread_attr_init(&attr)) != 0) {
		printf("%s: pthread_attr_init failed, status=%d\n", __func__, r);
	}

	/* 1. set a priority */
	sparam.sched_priority = TLS_CLIENT_PRIORITY;
	if ((r = pthread_attr_setschedparam(&attr, &sparam)) != 0) {
		printf("%s: pthread_attr_setschedparam failed, status=%d\n", __func__, r);
	}

	if ((r = pthread_attr_setschedpolicy(&attr, TLS_CLIENT_SCHED_POLICY)) != 0) {
		printf("%s: pthread_attr_setschedpolicy failed, status=%d\n", __func__, r);
	}

	/* 2. set a stacksize */
	if ((r = pthread_attr_setstacksize(&attr, TLS_CLIENT_STACK_SIZE)) != 0) {
		printf("%s: pthread_attr_setstacksize failed, status=%d\n", __func__, r);
	}

	/* 3. create pthread with entry function */
	if ((r = pthread_create(&tid, &attr, (pthread_startroutine_t)tls_client_cb, (void *)&args)) != 0) {
		printf("%s: pthread_create failed, status=%d\n", __func__, r);
	}

	/* Wait for the threads to stop */
	pthread_join(tid, NULL);

	return 0;
}
#endif
