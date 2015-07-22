/*************************************************************************
 *           Atrinik, a Multiplayer Online Role Playing Game             *
 *                                                                       *
 *   Copyright (C) 2009-2014 Alex Tokar and Atrinik Development Team     *
 *                                                                       *
 * Fork from Crossfire (Multiplayer game for X-windows).                 *
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program; if not, write to the Free Software           *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.             *
 *                                                                       *
 * The author can be reached at admin@atrinik.org                        *
 ************************************************************************/

/**
 * @file
 * Socket related functions.
 *
 * @author Alex Tokar
 */

#ifndef __CPROTO__

#include <global.h>
#include <toolkit_string.h>
#include <sys/types.h>

/**
 * The socket structure.
 */
struct sock_struct {
    /**
     * Actual socket handle, as returned by socket() call.
     */
    int handle;

    /**
     * The socket address.
     */
    struct sockaddr_storage addr;

    /**
     * Hostname that the socket connection will use.
     */
    char *host;

    /**
     * Port that the socket connection will use.
     */
    uint16_t port;

    /**
     * SSL socket handle.
     */
    SSL *ssl_handle;
};

/**
 * The OpenSSL context.
 */
static SSL_CTX *ssl_context;

/**
 * Whitelist of candidate ciphers.
 */
static const char *const ciphers_candidate[] = {
    "AES128-GCM-SHA256", "AES128-SHA256", "AES256-SHA256", /* strong ciphers */
    "AES128-SHA", "AES256-SHA", /* strong ciphers, also in older versions */
    "RC4-SHA", "RC4-MD5", /* backwards compatibility, supposed to be weak */
    "DES-CBC3-SHA", "DES-CBC3-MD5", /* more backwards compatibility */
    NULL
};

static SSL_CTX *socket_ssl_ctx_create(void);
static void socket_ssl_ctx_destroy(SSL_CTX *ctx);

TOOLKIT_API();

TOOLKIT_INIT_FUNC(socket)
{
    OPENSSL_config(NULL);
    SSL_load_error_strings();
    SSL_library_init();

    ssl_context = socket_ssl_ctx_create();
}
TOOLKIT_INIT_FUNC_FINISH

TOOLKIT_DEINIT_FUNC(socket)
{
    socket_ssl_ctx_destroy(ssl_context);
}
TOOLKIT_DEINIT_FUNC_FINISH

/**
 * Creates a new socket structure, complete with a socket for the specified
 * host/port.
 * @param host The host(name). Can be an IP address (either IPv4 or IPv6) or
 * a hostname. Can be NULL.
 * @param port Port to connec to.
 * @return Newly allocated socket, NULL in case of failure.
 */
socket_t *socket_create(const char *host, uint16_t port)
{
    socket_t *sc = ecalloc(1, sizeof(*sc));

#ifdef HAVE_GETADDRINFO
    char port_str[6];
    snprintf(VS(port_str), "%" PRIu16, port);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    if (host == NULL) {
        hints.ai_flags |= AI_PASSIVE;
    }

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        LOG(ERROR, "Cannot getaddrinfo(), host %s, port %" PRIu16 ": %s (%d)",
                host != NULL ? host : "<none>", port, strerror(errno), errno);
        goto error;
    }

    for (struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
#ifdef HAVE_IPV6
        if (host == NULL && ai->ai_family != AF_INET6) {
            continue;
        }
#endif

        sc->handle = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sc->handle == -1) {
            continue;
        }

#ifdef HAVE_IPV6
        if (ai->ai_family == AF_INET6) {
            int flag = 0;
            if (setsockopt(sc->handle, IPPROTO_IPV6, IPV6_V6ONLY,
                    (const char *) &flag, sizeof(flag)) != 0) {
                LOG(ERROR, "Cannot setsockopt(IPV6_V6ONLY): %s (%d)",
                        strerror(errno), errno);
                socket_close(sc);
                goto error;
            }
        }
#endif

        memcpy(&sc->addr, ai->ai_addr, sizeof(sc->addr));
        break;
    }

    freeaddrinfo(res);
#else
    if (host != NULL) {
        struct hostent *host_entry = gethostbyname(host);
        if (host_entry == NULL) {
            LOG(ERROR, "Cannot gethostbyname(), host %s, port %" PRIu16 ": %s "
                    "(%d)", host != NULL ? host : "<none>", port,
                    strerror(errno), errno);
            goto error;
        }

        sc->addr.sin_addr = *((struct in_addr *) host_entry->h_addr);
    } else {
        sc->addr.sin_addr = INADDR_ANY;
    }

    sc->addr.sin_family = AF_INET;
    sc->addr.sin_port = htons(port);
    memset(&sc->addr.sin_zero, 0, sizeof(sc->addr.sin_zero));

    struct protoent *protox = getprotobyname("tcp");
    if (protox == NULL) {
        LOG(ERROR, "Cannot getprotobyname(), host %s, port %" PRIu16 ": %s "
                "(%d)", host != NULL ? host : "<none>", port, strerror(errno),
                errno);
        goto error;
    }

    sc->handle = socket(PF_INET, SOCK_STREAM, protox->p_proto);
#endif
    if (sc->handle == -1) {
        LOG(ERROR, "Cannot socket(), host %s, port %" PRIu16 ": %s (%d)",
                host != NULL ? host : "<none>", port, strerror(errno), errno);
        goto error;
    }

    sc->host = host != NULL ? estrdup(host) : NULL;
    sc->port = port;
    return sc;

error:
    efree(sc);
    return NULL;
}

/**
 * Acquire the socket's address as a string representation.
 * @param sc Socket.
 * @return Pointer to a static buffer containing the socket's address. Will be
 * overwritten with the next call.
 */
char *socket_get_addr(socket_t *sc)
{
    static char buf[MAX_BUF];
    if (socket_addr2host(&sc->addr, VS(buf)) == NULL) {
        snprintf(VS(buf), "<no address>");
    }
    return buf;
}

/**
 * Acquire a string representation of the socket (its address and port).
 * @param sc Socket.
 * @return Pointer to a static buffer containing the socket's string
 * representation. Will be overwritten with the next call.
 */
char *socket_get_str(socket_t *sc)
{
    static char buf[MAX_BUF];
    snprintf(VS(buf), "%s %" PRIu16, socket_get_addr(sc), sc->port);
    return buf;
}

/** Helper structure used in socket_cmp_addr(). */
union sockaddr_union {
    struct sockaddr sa; ///< sockaddr.
    struct sockaddr_in in4; ///< IPv4.
#ifdef HAVE_IPV6
    struct sockaddr_in6 in6; ///< IPv6.
    struct sockaddr_storage storage; ///< Storage.
#endif
};

#ifdef HAVE_IPV6
struct in6_addr_helper {
    union {
        uint8_t __u6_addr8[16];
        uint16_t __u6_addr16[8];
        uint32_t __u6_addr32[4];
    } __in6_u;
#   define s6_addr32__ __in6_u.__u6_addr32
};
#endif

/**
 * Compare the socket's address to another address/subnet.
 * @param sc Socket to compare.
 * @param addr Address. IPv4 or IPv6.
 * @param plen Prefix length; the subnet.
 * @return 0 if the socket's address matches the supplied address/subnet,
 * anything else otherwise.
 */
int socket_cmp_addr(socket_t *sc, const struct sockaddr_storage *addr,
        unsigned short plen)
{
    HARD_ASSERT(sc != NULL);
    HARD_ASSERT(addr != NULL);

    const struct sockaddr *sc_addr = (const struct sockaddr *) &sc->addr;
    const struct sockaddr *saddr = (const struct sockaddr *) addr;
    if (sc_addr->sa_family != saddr->sa_family) {
        return -1;
    }

    const union sockaddr_union *addr1 = (const union sockaddr_union *) sc_addr;
    const union sockaddr_union *addr2 = (const union sockaddr_union *) addr;

    switch (sc_addr->sa_family) {
    case AF_INET:
    {
        HARD_ASSERT(plen <= 32);
        unsigned long mask = htonl(0xffffffff << (32 - plen));

        if ((addr1->in4.sin_addr.s_addr & mask) ==
            (addr2->in4.sin_addr.s_addr & mask)) {
            return 0;
        }

        break;
    }

#ifdef HAVE_IPV6
    case AF_INET6:
    {
        HARD_ASSERT(plen <= 128);
        struct in6_addr_helper mask;
        mask.s6_addr32__[0] = htonl(plen <= 32 ? 0xffffffff << (32 - plen) :
            0xffffffff);
        mask.s6_addr32__[1] = htonl(plen <= 32 ? 0 :
            (plen > 64 ? 0xffffffff : 0xffffffff << (32 - (plen - 32))));
        mask.s6_addr32__[2] = htonl(plen <= 64 ? 0 :
            (plen > 96 ? 0xffffffff : 0xffffffff << (32 - (plen - 64))));
        mask.s6_addr32__[3] = htonl(plen <= 96 ? 0 :
            0xffffffff << (32 - (plen - 96)));

        struct in6_addr_helper *sin6_addr1 =
            (struct in6_addr_helper *) &addr1->in6.sin6_addr;
        struct in6_addr_helper *sin6_addr2 =
            (struct in6_addr_helper *) &addr2->in6.sin6_addr;

        return (!!(((sin6_addr1->s6_addr32__[0] ^
                sin6_addr2->s6_addr32__[0]) & mask.s6_addr32__[0]) |
                ((sin6_addr1->s6_addr32__[1] ^
                sin6_addr2->s6_addr32__[1]) & mask.s6_addr32__[1]) |
                ((sin6_addr1->s6_addr32__[2] ^
                sin6_addr2->s6_addr32__[2]) & mask.s6_addr32__[2]) |
                ((sin6_addr1->s6_addr32__[3] ^
                sin6_addr2->s6_addr32__[3]) & mask.s6_addr32__[3])));
    }
#endif

    default:
        LOG(ERROR, "Don't know how to compare socket family: %u",
                sc_addr->sa_family);
        break;
    }

    return -1;
}

/**
 * Acquire the socket's file descriptor.
 * @param sc Socket.
 * @return The file descriptor.
 */
int socket_fd(socket_t *sc)
{
    HARD_ASSERT(sc != NULL);
    HARD_ASSERT(sc->handle != -1);

    return sc->handle;
}

/**
 * Connect the specified socket to the host/port specified during the socket
 * structure's creation (using socket_create()).
 * @param sc The socket.
 * @return True on success, false on failure.
 */
bool socket_connect(socket_t *sc)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, false, "Invalid socket file handle");

    if (connect(sc->handle, (struct sockaddr *) &sc->addr,
            sizeof(struct sockaddr)) == -1) {
        LOG(ERROR, "Cannot connect(): %s (%d)", strerror(errno), errno);
        return false;
    }

    return true;
}

/**
 * Begin listening on the specified socket.
 * @param sc Socket.
 * @return True on success, false on failure.
 */
bool socket_bind(socket_t *sc)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, false, "Invalid socket file handle");

    if (bind(sc->handle, (struct sockaddr *) &sc->addr,
            sizeof(sc->addr)) == -1) {
        LOG(ERROR, "Cannot bind(): %s (%d)", strerror(errno), errno);
        return false;
    }

    if (listen(sc->handle, 5) == -1) {
        LOG(ERROR, "Cannot listen(): %s (%d)", strerror(errno), errno);
        return false;
    }

    return true;
}

/**
 * Accept an incoming connection on the specified socket.
 * @param sc Socket.
 * @return Newly allocated socket structure containing a valid handle, NULL
 * on failure.
 */
socket_t *socket_accept(socket_t *sc)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, NULL, "Invalid socket file handle");

    socket_t *tmp = ecalloc(1, sizeof(*tmp));
    socklen_t addrlen = sizeof(tmp->addr);
    tmp->handle = accept(sc->handle, (struct sockaddr *) &tmp->addr, &addrlen);
    if (tmp->handle == -1) {
        efree(tmp);
        return NULL;
    }

    tmp->port = ((struct sockaddr_in *) &tmp->addr)->sin_port;
    return tmp;
}

/**
 * Checks if socket's file descriptor is valid.
 * @param sc Socket to check.
 * @return True if the socket's file descriptor is valid, false otherwise.
 */
bool socket_is_fd_valid(socket_t *sc)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, false, "Invalid socket file handle");

#ifndef WIN32
    return fcntl(sc->handle, F_GETFL) != -1 || errno != EBADF;
#else
    return true;
#endif
}

/**
 * Changes the socket's linger option.
 * @param sc Socket.
 * @param enable Whether to enable linger.
 * @param linger Linger value.
 * @return True on success, false on failure.
 */
bool socket_opt_linger(socket_t *sc, bool enable, unsigned short linger)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, false, "Invalid socket file handle");

    struct linger linger_opt;
    linger_opt.l_onoff = !!enable;
    linger_opt.l_linger = linger;

    if (setsockopt(sc->handle, SOL_SOCKET, SO_LINGER,
            (const char *) &linger_opt, sizeof(struct linger)) == -1) {
        LOG(ERROR, "Cannot setsockopt(SO_LINGER): %s (%d)", strerror(errno),
                errno);
        return false;
    }

    return true;
}

/**
 * Changes the socket's reuse address option.
 * @param sc Socket.
 * @param enable Whether to enable address reuse.
 * @return True on success, false on failure.
 */
bool socket_opt_reuse_addr(socket_t *sc, bool enable)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, false, "Invalid socket file handle");

    int flag = !!enable;
    if (setsockopt(sc->handle, SOL_SOCKET, SO_REUSEADDR, (const char *) &flag,
            sizeof(flag)) == -1) {
        LOG(ERROR, "Cannot setsockopt(SO_REUSEADDR): %s (%d)", strerror(errno),
                errno);
        return false;
    }

    return true;
}

/**
 * Makes the socket blocking/non-blocking.
 * @param sc Socket.
 * @param enable If true, socket will be changed to non-blocking, otherwise it
 * will be changed to blocking.
 * @return True on success, false on failure.
 */
bool socket_opt_non_blocking(socket_t *sc, bool enable)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, false, "Invalid socket file handle");

#ifdef WIN32
    u_long flag = !!enable;
    if (ioctlsocket(sc->handle, FIONBIO, &flag) == -1) {
        LOG(ERROR, "Cannot ioctlsocket(): %s (%d)", strerror(errno), errno);
#else
    int flags = fcntl(sc->handle, F_GETFL);
    if (flags == -1) {
        LOG(ERROR, "Cannot fcntl(F_GETFL): %s (%d)", strerror(errno), errno);
        return false;
    }

    if (enable) {
        flags |= O_NDELAY | O_NONBLOCK;
    } else {
        flags &= ~(O_NDELAY | O_NONBLOCK);
    }

    if (fcntl(sc->handle, F_SETFL, flags) == -1) {
        LOG(ERROR, "Cannot fcntl(F_SETFL), flags %d: %s (%d)", flags,
                strerror(errno), errno);
#endif
        return false;
    }

    return true;
}

/**
 * Changes the socket's send buffer size option.
 * @param sc Socket.
 * @param bufsize New send buffer size. If smaller than existing buffer size,
 * nothing is done.
 * @return True on success, false on failure.
 */
bool socket_opt_send_buffer(socket_t *sc, int bufsize)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT_RC(sc->handle != -1, false, "Invalid socket file handle");

    int oldbufsize;
    socklen_t buflen = sizeof(int);

    if (getsockopt(sc->handle, SOL_SOCKET, SO_SNDBUF, (char *) &oldbufsize,
            &buflen) == -1) {
        oldbufsize = 0;
    }

    if (oldbufsize < bufsize) {
        if (setsockopt(sc->handle, SOL_SOCKET, SO_SNDBUF,
                (const char *) &bufsize, sizeof(bufsize))) {
            LOG(ERROR, "Cannot setsockopt(), bufsize %d: %s (%d)", bufsize,
                    strerror(errno), errno);
            return false;
        }
    }

    return true;
}

/**
 * Destroys the specified socket, freeing all data associated with it and
 * closing any file descriptors.
 * @param sc Socket.
 */
void socket_destroy(socket_t *sc)
{
    HARD_ASSERT(sc != NULL);

    if (sc->host != NULL) {
        efree(sc->host);
    }

    socket_close(sc);
    efree(sc);
}

/**
 * Closes file descriptor associated with the socket.
 * @param sc Socket.
 */
void socket_close(socket_t *sc)
{
    HARD_ASSERT(sc != NULL);

    SOFT_ASSERT(sc->handle != -1, "Invalid socket handle");

#ifndef WIN32
    close(sc->handle);
#else
    shutdown(sc->handle, SD_BOTH);
    closesocket(sc->handle);
#endif
}

/**
 * Convert an IPv4/IPv6 address or a hostname to a binary representation.
 * @param host IP/hostname.
 * @param[out] addr Where to store the binary representation.
 * @return True on success, false on failure.
 */
bool socket_host2addr(const char *host, struct sockaddr_storage *addr)
{
    HARD_ASSERT(host != NULL);
    HARD_ASSERT(addr != NULL);

#ifdef HAVE_GETADDRINFO
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        LOG(ERROR, "Cannot getaddrinfo(), host %s: %s (%d)", host,
                strerror(errno), errno);
        return false;
    }

    bool retval = false;
    for (struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
        int handle = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (handle == -1) {
            continue;
        }

        retval = true;
        memcpy(addr, ai->ai_addr, sizeof(*addr));
        close(handle);
        break;
    }

    freeaddrinfo(res);
    return retval;
#else
    struct hostent *host_entry = gethostbyname(host);
    if (host_entry == NULL) {
        LOG(ERROR, "Cannot gethostbyname(), host %s: %s (%d)", host,
                strerror(errno), errno);
        return false;
    }

    addr->sin_addr = *((struct in_addr *) host_entry->h_addr);
    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    memset(&addr->sin_zero, 0, sizeof(addr->sin_zero));
    return true;
#endif
}

#if defined(WIN32) && defined(HAVE_IPV6)
/** @cond */
static const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
    struct sockaddr_storage ss;
    ZeroMemory(&ss, sizeof(ss));
    ss.ss_family = af;

    switch (af) {
        case AF_INET:
            ((struct sockaddr_in *) &ss)->sin_addr = *(struct in_addr *) src;
            break;

        case AF_INET6:
            ((struct sockaddr_in6 *) &ss)->sin6_addr = *(struct in6_addr *) src;
            break;

        default:
            LOG(ERROR, "Unknown family: %d", af);
            return NULL;
    }

    unsigned long s = size;
    if (WSAAddressToString((struct sockaddr *) &ss, sizeof(ss), NULL, dst,
            &s) == 0) {
        return dst;
    }

    LOG(ERROR, "Cannot WSAAddressToString(): %s (%d)", strerror(s_errno),
            errno);
    return NULL;
}
/** @endcond */
#endif

/**
 * Convert a binary representation of an IP (either IPv4 or IPv6) address into
 * text form.
 * @param addr Binary representation of the IP.
 * @param buf Where to store the text representation.
 * @param bufsize Size of 'buf'.
 * @return 'buf' on success, NULL on failure.
 */
const char *socket_addr2host(struct sockaddr_storage *addr, char *buf,
        size_t bufsize)
{
    HARD_ASSERT(addr != NULL);
    HARD_ASSERT(buf != NULL);

    struct sockaddr_in *saddr = (struct sockaddr_in *) addr;
#ifdef HAVE_IPV6
    if (saddr->sin_family == AF_INET6) {
        struct sockaddr_in6 *saddr6 = (struct sockaddr_in6 *) addr;
        return inet_ntop(saddr6->sin6_family, &saddr6->sin6_addr, buf,
                bufsize);
    }
    return inet_ntop(saddr->sin_family, &saddr->sin_addr, buf, bufsize);
#else
    return inet_ntoa_r(saddr->sin_addr, buf, bufsize);
#endif
}

/**
 * Selects the best cipher from the list of available ciphers, which is
 * obtained by creating a dummy SSL session.
 * @param ctx Context to select the best cipher for.
 * @return 1 on success, 0 on failure.
 */
static int socket_ssl_ctx_select_cipher(SSL_CTX *ctx)
{
    SSL *ssl;
    STACK_OF(SSL_CIPHER) *active_ciphers;
    char ciphers[300];
    const char *const *c;
    int i;

    ssl = SSL_new(ctx);

    if (ssl == NULL) {
        return 0;
    }

    active_ciphers = SSL_get_ciphers(ssl);

    if (active_ciphers == NULL) {
        return 0;
    }

    ciphers[0] = '\0';

    for (c = ciphers_candidate; *c; c++) {
        for (i = 0; i < sk_SSL_CIPHER_num(active_ciphers); i++) {
            if (strcmp(SSL_CIPHER_get_name(
                    sk_SSL_CIPHER_value(active_ciphers, i)), *c) == 0) {
                if (*ciphers != '\0') {
                    snprintfcat(VS(ciphers), ":");
                }

                snprintfcat(VS(ciphers), "%s", *c);
                break;
            }
        }
    }

    SSL_free(ssl);

    /* Apply final cipher list. */
    if (SSL_CTX_set_cipher_list(ctx, ciphers) != 1) {
        return 0;
    }

    return 1;
}

static SSL_CTX *socket_ssl_ctx_create(void)
{
    const SSL_METHOD *req_method;
    SSL_CTX *ctx;

    req_method = TLSv1_client_method();
    ctx = SSL_CTX_new(req_method);

    if (ctx == NULL) {
        return NULL;
    }

    /* Configure a client connection context. */
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);

    /* Adjust the ciphers list based on a whitelist. First enable all
     * ciphers of at least medium strength, to get the list which is
     * compiled into OpenSSL. */
    if (SSL_CTX_set_cipher_list(ctx, "HIGH:MEDIUM") != 1) {
        return NULL;
    }

    /* Select best cipher. */
    if (socket_ssl_ctx_select_cipher(ctx) != 1) {
        return NULL;
    }

    /* Load the set of trusted root certificates. */
    if (!SSL_CTX_set_default_verify_paths(ctx)) {
        return NULL;
    }

    return ctx;
}

static void socket_ssl_ctx_destroy(SSL_CTX *ctx)
{
    SSL_CTX_free(ctx);
}

SSL *socket_ssl_create(socket_t *sc, SSL_CTX *ctx)
{
    SSL *ssl;
    int ret;
    X509 *peercert;
    char peer_CN[256];

    ssl = SSL_new(ctx);

    if (ssl == NULL) {
        return NULL;
    }

    SSL_set_fd(ssl, sc->handle);

    /* Enable the ServerNameIndication extension */
    if (!SSL_set_tlsext_host_name(ssl, sc->host)) {
        return NULL;
    }

    /* Perform the TLS handshake with the server. */
    ret = SSL_connect(ssl);

    /* Error status can be 0 or negative. */
    if (ret != 1) {
        return NULL;
    }

    /* Obtain the server certificate. */
    peercert = SSL_get_peer_certificate(ssl);

    if (peercert == NULL) {
        LOG(SYSTEM, "Server's peer certificate is missing.");
        return NULL;
    }

    ret = SSL_get_verify_result(ssl);

    /* Check the certificate verification result. */
    if (ret != X509_V_OK) {
        LOG(SYSTEM, "Verify result: %s",
                X509_verify_cert_error_string(ret));
        X509_free(peercert);
        return NULL;
    }

    /* Check if the server certificate matches the host name used to
     * establish the connection. */
    X509_NAME_get_text_by_NID(X509_get_subject_name(peercert), NID_commonName,
            peer_CN, sizeof(peer_CN));

    if (strcasecmp(peer_CN, sc->host) != 0) {
        LOG(SYSTEM, "Peer name %s doesn't match host name %s\n", peer_CN,
                sc->host);
        X509_free(peercert);
        return NULL;
    }

    X509_free(peercert);

    return ssl;
}

void socket_ssl_destroy(SSL *ssl)
{
    int ret;

    /* Send the close_notify alert. */
    ret = SSL_shutdown(ssl);

    switch (ret) {
        /* A close_notify alert has already been received. */
    case 1:
        break;

        /* Wait for the close_notify alert from the peer. */
    case 0:
        ret = SSL_shutdown(ssl);

        switch (ret) {
        case 0:
            LOG(SYSTEM,  "second SSL_shutdown returned zero");
            break;

        case 1:
            break;

        default:
            LOG(SYSTEM,  "SSL_shutdown 2 %d", ret);
        }
        break;

    default:
        LOG(SYSTEM,  "SSL_shutdown 1 %d", ret);
    }

    SSL_free(ssl);
}

#endif
