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
 * Handles connection to the metaserver and receiving data from it.
 *
 * @author Alex Tokar
 */

#include <global.h>
#include <toolkit_string.h>
#include <curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/**
 * Macro to check for XML string equality without the annoying xmlChar
 * pointer casts.
 *
 * @param s1
 * First string to compare.
 * @param s2
 * Second string to compare.
 * @return
 * 1 if both strings are equal, 0 otherwise.
 */
#define XML_STR_EQUAL(s1, s2) \
    xmlStrEqual((const xmlChar *) s1, (const xmlChar *) s2)

#ifdef __MINGW32__
#   define xmlFree free
#endif

/** Are we connecting to the metaserver? */
static int metaserver_connecting = 1;
/** Mutex to protect ::metaserver_connecting. */
static SDL_mutex *metaserver_connecting_mutex;
/** The list of the servers. */
static server_struct *server_head;
/** Number of the servers. */
static size_t server_count;
/** Mutex to protect ::server_head and ::server_count. */
static SDL_mutex *server_head_mutex;
/** Is metaserver enabled? */
static uint8_t enabled = 1;

/**
 * Initialize the metaserver data.
 */
void metaserver_init(void)
{
    /* Initialize the data. */
    server_head = NULL;
    server_count = 0;

    /* Initialize mutexes. */
    metaserver_connecting_mutex = SDL_CreateMutex();
    server_head_mutex = SDL_CreateMutex();

    /* Initialize libxml2 */
    LIBXML_TEST_VERSION
}

/**
 * Disable the metaserver.
 */
void metaserver_disable(void)
{
    enabled = 0;
    metaserver_connecting = 0;
}

/**
 * Free the specified metaserver server node.
 *
 * @param server
 * Node to free.
 */
static void
metaserver_free (server_struct *server)
{
    HARD_ASSERT(server != NULL);

    if (server->hostname != NULL) {
        efree(server->hostname);
    }

    if (server->name != NULL) {
        efree(server->name);
    }

    if (server->version != NULL) {
        efree(server->version);
    }

    if (server->desc != NULL) {
        efree(server->desc);
    }

    if (server->cert_pubkey != NULL) {
        efree(server->cert_pubkey);
    }

    efree(server);
}

/**
 * Parse a single metaserver data node within a 'server' node.
 *
 * @param node
 * The data node.
 * @param server
 * Allocated server structure.
 * @return
 * True on success, false on failure.
 */
static bool
parse_metaserver_data_node (xmlNodePtr node, server_struct *server)
{
    HARD_ASSERT(node != NULL);
    HARD_ASSERT(server != NULL);

    xmlChar *content = xmlNodeGetContent(node);
    SOFT_ASSERT_LABEL(content != NULL && *content != '\0',
                      error,
                      "Parsing error");

    if (XML_STR_EQUAL(node->name, "hostname")) {
        SOFT_ASSERT_LABEL(server->hostname == NULL, error, "Parsing error");
        server->hostname = estrdup((const char *) content);
    } else if (XML_STR_EQUAL(node->name, "port")) {
        SOFT_ASSERT_LABEL(server->port == 0, error, "Parsing error");
        server->port = atoi((const char *) content);
    } else if (XML_STR_EQUAL(node->name, "port_crypto")) {
        SOFT_ASSERT_LABEL(server->port_crypto == -1, error, "Parsing error");
        server->port_crypto = atoi((const char *) content);
    } else if (XML_STR_EQUAL(node->name, "name")) {
        SOFT_ASSERT_LABEL(server->name == NULL, error, "Parsing error");
        server->name = estrdup((const char *) content);
    } else if (XML_STR_EQUAL(node->name, "num_players")) {
        SOFT_ASSERT_LABEL(server->player == 0, error, "Parsing error");
        server->player = atoi((const char *) content);
    } else if (XML_STR_EQUAL(node->name, "version")) {
        SOFT_ASSERT_LABEL(server->version == NULL, error, "Parsing error");
        server->version = estrdup((const char *) content);
    } else if (XML_STR_EQUAL(node->name, "text_comment")) {
        SOFT_ASSERT_LABEL(server->desc == NULL, error, "Parsing error");
        server->desc = estrdup((const char *) content);
    } else if (XML_STR_EQUAL(node->name, "cert_pubkey")) {
        SOFT_ASSERT_LABEL(server->cert_pubkey == NULL, error, "Parsing error");
        server->cert_pubkey = estrdup((const char *) content);
    } else {
        LOG(DEVEL, "Unrecognized node: %s", (const char *) node->name);
    }

    bool ret = true;
    goto out;

error:
    ret = false;

out:
    if (content != NULL) {
        xmlFree(content);
    }

    return ret;
}

/**
 * Parse metaserver 'server' node.
 *
 * @param node
 * Node to parse.
 */
static void
parse_metaserver_node (xmlNodePtr node)
{
    HARD_ASSERT(node != NULL);

    server_struct *server = ecalloc(1, sizeof(*server));
    server->port_crypto = -1;

    for (xmlNodePtr tmp = node->children; tmp != NULL; tmp = tmp->next) {
        if (!parse_metaserver_data_node(tmp, server)) {
            goto error;
        }
    }

    if (server->hostname == NULL ||
        server->port == 0 ||
        server->name == NULL ||
        server->version == NULL ||
        server->desc == NULL) {
        LOG(ERROR, "Incomplete data from metaserver");
        goto error;
    }

    SDL_LockMutex(server_head_mutex);
    DL_PREPEND(server_head, server);
    server_count++;
    SDL_UnlockMutex(server_head_mutex);
    return;

error:
    metaserver_free(server);
}

/**
 * Parse data returned from HTTP metaserver and add it to the list of servers.
 *
 * @param body
 * The data to parse.
 * @param body_size
 * Length of the body.
 */
static void
parse_metaserver_data (const char *body, size_t body_size)
{
    HARD_ASSERT(body != NULL);

    xmlDocPtr doc = xmlReadMemory(body, body_size, "noname.xml", NULL, 0);
    if (doc == NULL) {
        LOG(ERROR, "Failed to parse data from metaserver");
	goto out;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == NULL || !XML_STR_EQUAL(root->name, "servers")) {
        LOG(ERROR, "No servers element found in metaserver XML");
        goto out;
    }

    xmlNodePtr last = NULL;
    for (xmlNodePtr node = root->children; node != NULL; node = node->next) {
        last = node;
    }

    for (xmlNodePtr node = last; node != NULL; node = node->prev) {
        if (!XML_STR_EQUAL(node->name, "server")) {
            continue;
        }

        parse_metaserver_node(node);
    }

out:
    xmlFreeDoc(doc);
}

/**
 * Get server from the servers list by its ID.
 * @param num
 * ID of the server to find.
 * @return
 * The server if found, NULL otherwise.
 */
server_struct *server_get_id(size_t num)
{
    server_struct *node;
    size_t i;

    SDL_LockMutex(server_head_mutex);

    for (node = server_head, i = 0; node; node = node->next, i++) {
        if (i == num) {
            break;
        }
    }

    SDL_UnlockMutex(server_head_mutex);
    return node;
}

/**
 * Get number of the servers in the list.
 * @return
 * The number.
 */
size_t server_get_count(void)
{
    size_t count;

    SDL_LockMutex(server_head_mutex);
    count = server_count;
    SDL_UnlockMutex(server_head_mutex);
    return count;
}

/**
 * Check if we're connecting to the metaserver.
 * @param val
 * If not -1, set the metaserver connecting value to this.
 * @return
 * 1 if we're connecting to the metaserver, 0 otherwise.
 */
int ms_connecting(int val)
{
    int connecting;

    SDL_LockMutex(metaserver_connecting_mutex);
    connecting = metaserver_connecting;

    /* More useful to return the old value than the one we're setting. */
    if (val != -1) {
        metaserver_connecting = val;
    }

    SDL_UnlockMutex(metaserver_connecting_mutex);
    return connecting;
}

/**
 * Clear all data in the linked list of servers reported by metaserver.
 */
void metaserver_clear_data(void)
{
    server_struct *node, *tmp;

    SDL_LockMutex(server_head_mutex);

    DL_FOREACH_SAFE(server_head, node, tmp)
    {
        DL_DELETE(server_head, node);
        metaserver_free(node);
    }

    server_count = 0;
    SDL_UnlockMutex(server_head_mutex);
}

/**
 * Add a server entry to the linked list of available servers reported by
 * metaserver.
 *
 * @param hostname
 * Server's hostname.
 * @param port
 * Server port.
 * @param port_crypto
 * Secure port to use.
 * @param name
 * Server's name.
 * @param version
 * Server version.
 * @param desc
 * Description of the server.
 */
server_struct *
metaserver_add (const char *hostname,
                int         port,
                int         port_crypto,
                const char *name,
                const char *version,
                const char *desc)
{
    server_struct *node = ecalloc(1, sizeof(*node));
    node->player = -1;
    node->port = port;
    node->port_crypto = port_crypto;
    node->hostname = estrdup(hostname);
    node->name = estrdup(name);
    node->version = estrdup(version);
    node->desc = estrdup(desc);

    SDL_LockMutex(server_head_mutex);
    DL_PREPEND(server_head, node);
    server_count++;
    SDL_UnlockMutex(server_head_mutex);

    return node;
}

/**
 * Threaded function to connect to metaserver.
 *
 * Goes through the list of metaservers and calls metaserver_connect()
 * until it gets a return value of 1. If if goes through all the
 * metaservers and still fails, show an info to the user.
 * @param dummy
 * Unused.
 * @return
 * Always returns 0.
 */
int metaserver_thread(void *dummy)
{
    /* Go through all the metaservers in the list */
    for (size_t i = clioption_settings.metaservers_num; i > 0; i--) {
        /* Send a GET request to the metaserver */
        curl_request_t *request =
            curl_request_create(clioption_settings.metaservers[i - 1],
                                CURL_PKEY_TRUST_ULTIMATE);
        curl_request_do_get(request);

        /* If the request succeeded, parse the metaserver data and break out. */
        int http_code = curl_request_get_http_code(request);
        size_t body_size;
        char *body = curl_request_get_body(request, &body_size);
        if (http_code == 200 && body != NULL) {
            parse_metaserver_data(body, body_size);
            curl_request_free(request);
            break;
        }

        curl_request_free(request);
    }

    SDL_LockMutex(metaserver_connecting_mutex);
    /* We're not connecting anymore. */
    metaserver_connecting = 0;
    SDL_UnlockMutex(metaserver_connecting_mutex);
    return 0;
}

/**
 * Connect to metaserver and get the available servers.
 *
 * Works in a thread using SDL_CreateThread().
 */
void metaserver_get_servers(void)
{
    SDL_Thread *thread;

    if (!enabled) {
        return;
    }

    SDL_LockMutex(metaserver_connecting_mutex);
    metaserver_connecting = 1;
    SDL_UnlockMutex(metaserver_connecting_mutex);

    thread = SDL_CreateThread(metaserver_thread, NULL);

    if (!thread) {
        LOG(ERROR, "Thread creation failed.");
        exit(1);
    }
}
