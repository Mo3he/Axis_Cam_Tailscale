// Copyright (C) 2024  Mo3he
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * ACAP parameter bridge for Tailscale VPN.
 *
 * Responsibilities:
 *  1. Read Tailscale parameters from the ACAP parameter store (axparameter).
 *  2. Write them to CONFIG_FILE so the shell script can source them.
 *  3. Launch the shell script (Tailscale_VPN_run) as a child process.
 *  4. On any parameter change: rewrite CONFIG_FILE and do a full stop+restart
 *     of the child so the new config is picked up.
 *     Rapid changes within 300 ms are coalesced into a single restart.
 *  5. Watchdog: if the child exits unexpectedly, restart it.
 *
 * Shared across the userspace-networking variants (unprivileged 'sdk' ACAP
 * user) and the ROOT / kernel-networking variant. Build with -DHAS_PROXY_PORTS
 * for the userspace variants, which exposes the HTTP/SOCKS5 proxy port
 * parameters; the ROOT variant omits them since it has no local proxy.
 */

#include <axsdk/axparameter.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define APP_NAME      "Tailscale_VPN"
#define CONFIG_FILE   "/usr/local/packages/Tailscale_VPN/localdata/params.conf"
#define RUN_SCRIPT    "/usr/local/packages/Tailscale_VPN/Tailscale_VPN_run"
#define SENTINEL_FILE "/usr/local/packages/Tailscale_VPN/localdata/authkey_clear"

#ifdef HAS_PROXY_PORTS
#define RUN_SCRIPT_VARIANT "standard"
#else
#define RUN_SCRIPT_VARIANT "root"
#endif

static AXParameter *g_ax_handle  = NULL;
static pid_t child_pid = -1;
static guint reload_timer_id = 0;

static char *cfg_custom_server   = NULL;
static char *cfg_auth_key        = NULL;
#ifdef HAS_PROXY_PORTS
static char *cfg_http_proxy_port = NULL;
static char *cfg_socks5_port     = NULL;
#endif
static char *cfg_accept_dns      = NULL;
static char *cfg_accept_routes   = NULL;
static char *cfg_advertise_routes = NULL;

static void cache_set(char **field, const char *value) {
    if (!value) return;
    free(*field);
    *field = strdup(value);
}

static const char *cache_get(char **field, const char *fallback) {
    return (*field && **field) ? *field : fallback;
}

/* Ensure a parameter exists in the device parameter database. On in-place ACAP
 * upgrades a newly introduced manifest parameter is not always auto-registered,
 * which makes param.cgi return a 404 when the web UI tries to set it. Creating
 * it here is idempotent: if it already exists, ax_parameter_add fails harmlessly. */
static void ensure_param(AXParameter *handle, const char *name, const char *def) {
    GError *err = NULL;
    if (!ax_parameter_add(handle, name, def, "string", &err)) {
        if (err) g_error_free(err);
    }
}

/* ── child process management ──────────────────────────────────────────── */

static void stop_child(void) {
    if (child_pid <= 0)
        return;
    kill(child_pid, SIGTERM);
    for (int i = 0; i < 30; i++) {
        int status;
        if (waitpid(child_pid, &status, WNOHANG) == child_pid) {
            child_pid = -1;
            return;
        }
        usleep(100000);
    }
    syslog(LOG_WARNING, "child did not exit in 3 s, sending SIGKILL");
    kill(child_pid, SIGKILL);
    waitpid(child_pid, NULL, 0);
    child_pid = -1;
}

static void start_child(void) {
    stop_child();
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork failed: %s", strerror(errno));
        return;
    }
    if (pid == 0) {
        execl(RUN_SCRIPT, RUN_SCRIPT, RUN_SCRIPT_VARIANT, NULL);
        syslog(LOG_ERR, "execl %s failed: %s", RUN_SCRIPT, strerror(errno));
        _exit(1);
    }
    child_pid = pid;
    syslog(LOG_INFO, "started %s (pid %d)", RUN_SCRIPT, child_pid);
}

/* ── watchdog ────────────────────────────────────────────────────────────── */

static gboolean watchdog_cb(gpointer G_GNUC_UNUSED data) {
    if (child_pid > 0) {
        int status;
        pid_t ret = waitpid(child_pid, &status, WNOHANG);
        if (ret == child_pid) {
            int exit_code = WEXITSTATUS(status);
            syslog(LOG_WARNING, "child exited (status %d), restarting", exit_code);
            child_pid = -1;
            /* If child exited 0, auth succeeded — clear AuthKey via axparameter */
            if (exit_code == 0 && g_ax_handle && cfg_auth_key && *cfg_auth_key) {
                GError *err = NULL;
                if (ax_parameter_set(g_ax_handle, "AuthKey", "", TRUE, &err)) {
                    free(cfg_auth_key); cfg_auth_key = strdup("");
                    syslog(LOG_INFO, "AuthKey cleared after successful auth");
                } else {
                    syslog(LOG_WARNING, "failed to clear AuthKey: %s",
                           err ? err->message : "unknown");
                    if (err) g_error_free(err);
                }
            }
            start_child();
        }
    }
    return G_SOURCE_CONTINUE;
}

/* ── auth-key sentinel ───────────────────────────────────────────────────── */

/* The run script drops SENTINEL_FILE after a successful `tailscale up` that
 * used a one-time auth key. Clear the stored AuthKey so it is not reused and
 * disappears from the settings UI. This replaces the old exit-code-0 path,
 * which never fired because tailscaled keeps the child alive indefinitely. */
static gboolean authkey_sentinel_cb(gpointer G_GNUC_UNUSED data) {
    if (access(SENTINEL_FILE, F_OK) != 0)
        return G_SOURCE_CONTINUE;

    if (g_ax_handle && cfg_auth_key && *cfg_auth_key) {
        GError *err = NULL;
        if (ax_parameter_set(g_ax_handle, "AuthKey", "", TRUE, &err)) {
            free(cfg_auth_key); cfg_auth_key = strdup("");
            syslog(LOG_INFO, "AuthKey cleared after successful auth (sentinel)");
        } else {
            syslog(LOG_WARNING, "failed to clear AuthKey: %s",
                   err ? err->message : "unknown");
            if (err) g_error_free(err);
        }
    }
    unlink(SENTINEL_FILE);
    return G_SOURCE_CONTINUE;
}

/* ── config file ─────────────────────────────────────────────────────────── */

static void load_config_cache(AXParameter *handle) {
    GError *error = NULL;
    gchar *val = NULL;

#define LOAD(name, field) \
    val = NULL; error = NULL; \
    if (ax_parameter_get(handle, name, &val, &error)) { \
        free(field); field = val ? strdup(val) : strdup(""); \
        g_free(val); val = NULL; \
    } else { \
        syslog(LOG_WARNING, "ax_parameter_get %s failed: %s", name, \
               error ? error->message : "unknown"); \
        if (error) { g_error_free(error); error = NULL; } \
    }

    LOAD("CustomServer",   cfg_custom_server)
    LOAD("AuthKey",        cfg_auth_key)
#ifdef HAS_PROXY_PORTS
    LOAD("HttpProxyPort",  cfg_http_proxy_port)
    LOAD("Socks5Port",     cfg_socks5_port)
#endif
    LOAD("AcceptDNS",      cfg_accept_dns)
    LOAD("AcceptRoutes",   cfg_accept_routes)
    LOAD("AdvertiseRoutes", cfg_advertise_routes)
#undef LOAD
}

static void write_config_file(void) {
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) {
        syslog(LOG_ERR, "cannot open config file %s: %s",
               CONFIG_FILE, strerror(errno));
        return;
    }
    fprintf(f, "CUSTOM_SERVER=%s\n", cache_get(&cfg_custom_server,   ""));
    fprintf(f, "AUTH_KEY=%s\n",      cache_get(&cfg_auth_key,        ""));
#ifdef HAS_PROXY_PORTS
    fprintf(f, "CONF_HTTP=%s\n",     cache_get(&cfg_http_proxy_port, "8080"));
    fprintf(f, "CONF_SOCKS=%s\n",    cache_get(&cfg_socks5_port,     "1080"));
#endif
    fprintf(f, "ACCEPT_DNS=%s\n",    cache_get(&cfg_accept_dns,      "false"));
    fprintf(f, "ACCEPT_ROUTES=%s\n", cache_get(&cfg_accept_routes,   "false"));
    fprintf(f, "ADVERTISE_ROUTES=%s\n", cache_get(&cfg_advertise_routes, ""));
    fclose(f);
    chmod(CONFIG_FILE, 0600);
#ifdef HAS_PROXY_PORTS
    syslog(LOG_INFO, "config updated: http=%s socks=%s server=%s",
           cache_get(&cfg_http_proxy_port, "8080"),
           cache_get(&cfg_socks5_port,     "1080"),
           cache_get(&cfg_custom_server,   "(default)"));
#else
    syslog(LOG_INFO, "config updated: server=%s",
           cache_get(&cfg_custom_server, "(default)"));
#endif
}

/* ── ACAP parameter callback ─────────────────────────────────────────────── */

static gboolean debounced_restart(gpointer G_GNUC_UNUSED data) {
    reload_timer_id = 0;
    if (g_ax_handle)
        load_config_cache(g_ax_handle);
    write_config_file();
    syslog(LOG_INFO, "restarting with new config");
    stop_child();
    start_child();
    return G_SOURCE_REMOVE;
}

static void parameter_changed(const gchar *name, const gchar *value,
                               gpointer G_GNUC_UNUSED handle_void_ptr) {
    const char *dot = strrchr(name, '.');
    const char *short_name = dot ? dot + 1 : name;

    syslog(LOG_INFO, "parameter changed: %s", short_name);

    if      (strcmp(short_name, "CustomServer")   == 0) cache_set(&cfg_custom_server,   value);
    else if (strcmp(short_name, "AuthKey")        == 0) cache_set(&cfg_auth_key,        value);
#ifdef HAS_PROXY_PORTS
    else if (strcmp(short_name, "HttpProxyPort")  == 0) cache_set(&cfg_http_proxy_port, value);
    else if (strcmp(short_name, "Socks5Port")     == 0) cache_set(&cfg_socks5_port,     value);
#endif
    else if (strcmp(short_name, "AcceptDNS")      == 0) cache_set(&cfg_accept_dns,      value);
    else if (strcmp(short_name, "AcceptRoutes")   == 0) cache_set(&cfg_accept_routes,   value);
    else if (strcmp(short_name, "AdvertiseRoutes") == 0) cache_set(&cfg_advertise_routes, value);

    if (reload_timer_id)
        g_source_remove(reload_timer_id);
    reload_timer_id = g_timeout_add(300, debounced_restart, NULL);
}

/* ── embedded settings HTTP server (reverse-proxy fallback) ──────────────────
 * Some AXIS device classes (e.g. recorders/NVRs) do not expose the legacy
 * /axis-cgi/param.cgi VAPIX endpoint, so the web UI cannot load or save
 * settings through it. This tiny HTTP server, reached through the manifest
 * reverseProxy mapping at /local/Tailscale_VPN/api/settings, lets the web UI
 * fall back to reading and writing the parameters directly. */

#define HTTP_PORT 2201

static const char *http_param_names[] = {
    "CustomServer", "AuthKey",
#ifdef HAS_PROXY_PORTS
    "HttpProxyPort", "Socks5Port",
#endif
    "AcceptDNS", "AcceptRoutes", "AdvertiseRoutes"
};

static void cache_set_by_name(const char *name, const char *value) {
    if      (strcmp(name, "CustomServer")    == 0) cache_set(&cfg_custom_server,    value);
    else if (strcmp(name, "AuthKey")         == 0) cache_set(&cfg_auth_key,         value);
#ifdef HAS_PROXY_PORTS
    else if (strcmp(name, "HttpProxyPort")   == 0) cache_set(&cfg_http_proxy_port,  value);
    else if (strcmp(name, "Socks5Port")      == 0) cache_set(&cfg_socks5_port,      value);
#endif
    else if (strcmp(name, "AcceptDNS")       == 0) cache_set(&cfg_accept_dns,       value);
    else if (strcmp(name, "AcceptRoutes")    == 0) cache_set(&cfg_accept_routes,    value);
    else if (strcmp(name, "AdvertiseRoutes") == 0) cache_set(&cfg_advertise_routes, value);
}

static int http_is_known_param(const char *name) {
    for (size_t i = 0; i < G_N_ELEMENTS(http_param_names); i++)
        if (strcmp(name, http_param_names[i]) == 0) return 1;
    return 0;
}

static void http_json_append_escaped(GString *out, const char *s) {
    for (const char *p = s; *p; p++) {
        switch (*p) {
            case '"':  g_string_append(out, "\\\""); break;
            case '\\': g_string_append(out, "\\\\"); break;
            case '\n': g_string_append(out, "\\n");  break;
            case '\r': g_string_append(out, "\\r");  break;
            case '\t': g_string_append(out, "\\t");  break;
            default:
                if ((unsigned char)*p < 0x20)
                    g_string_append_printf(out, "\\u%04x", (unsigned char)*p);
                else
                    g_string_append_c(out, *p);
        }
    }
}

static gchar *http_build_settings_json(AXParameter *handle) {
    GString *out = g_string_new("{");
    for (size_t i = 0; i < G_N_ELEMENTS(http_param_names); i++) {
        gchar *val = NULL;
        GError *err = NULL;
        if (!ax_parameter_get(handle, http_param_names[i], &val, &err)) {
            if (err) g_error_free(err);
            val = g_strdup("");
        }
        if (i) g_string_append_c(out, ',');
        g_string_append_printf(out, "\"%s\":\"", http_param_names[i]);
        http_json_append_escaped(out, val ? val : "");
        g_string_append_c(out, '"');
        g_free(val);
    }
    g_string_append_c(out, '}');
    /* g_string_free(out, FALSE) is inlined by glib >= 2.76 headers into a call
     * to g_string_free_and_steal(), which doesn't exist in older glib runtimes
     * (e.g. AXIS OS 11.x). Copy out and fully free instead to stay portable. */
    gchar *json_result = g_strdup(out->str);
    g_string_free(out, TRUE);
    return json_result;
}

static gchar *http_url_decode(const char *s, size_t len) {
    GString *out = g_string_new(NULL);
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '+') {
            g_string_append_c(out, ' ');
        } else if (c == '%' && i + 2 < len &&
                   g_ascii_isxdigit(s[i + 1]) && g_ascii_isxdigit(s[i + 2])) {
            int hi = g_ascii_xdigit_value(s[i + 1]);
            int lo = g_ascii_xdigit_value(s[i + 2]);
            g_string_append_c(out, (char)((hi << 4) | lo));
            i += 2;
        } else {
            g_string_append_c(out, c);
        }
    }
    gchar *decoded_result = g_strdup(out->str);
    g_string_free(out, TRUE);
    return decoded_result;
}

/* Apply an application/x-www-form-urlencoded body of shortName=value pairs to
 * the parameter store. Returns the number of parameters successfully set. */
static int http_apply_settings(AXParameter *handle, const char *body, size_t len) {
    int applied = 0;
    size_t start = 0;
    for (size_t i = 0; i <= len; i++) {
        if (i == len || body[i] == '&') {
            size_t seg_len = i - start;
            if (seg_len > 0) {
                const char *seg = body + start;
                const char *eq = memchr(seg, '=', seg_len);
                if (eq) {
                    size_t nlen = (size_t)(eq - seg);
                    gchar *name = g_strndup(seg, nlen);
                    gchar *value = http_url_decode(eq + 1, seg_len - nlen - 1);
                    if (http_is_known_param(name)) {
                        GError *err = NULL;
                        if (ax_parameter_set(handle, name, value, TRUE, &err)) {
                            cache_set_by_name(name, value);
                            applied++;
                        } else {
                            syslog(LOG_WARNING, "http set %s failed: %s",
                                   name, err ? err->message : "unknown");
                            if (err) g_error_free(err);
                        }
                    }
                    g_free(name);
                    g_free(value);
                }
            }
            start = i + 1;
        }
    }
    return applied;
}

static size_t http_parse_content_length(const char *hdr, size_t hlen) {
    const char *key = "content-length:";
    size_t klen = strlen(key);
    for (size_t i = 0; i + klen <= hlen; i++) {
        if (g_ascii_strncasecmp(hdr + i, key, klen) == 0) {
            i += klen;
            while (i < hlen && (hdr[i] == ' ' || hdr[i] == '\t')) i++;
            return (size_t)strtoul(hdr + i, NULL, 10);
        }
    }
    return 0;
}

static void http_send(GOutputStream *out, const char *status,
                      const char *ctype, const char *body) {
    gchar *resp = g_strdup_printf(
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, ctype, strlen(body), body);
    g_output_stream_write_all(out, resp, strlen(resp), NULL, NULL, NULL);
    g_free(resp);
}

static gboolean http_on_incoming(GSocketService    *service G_GNUC_UNUSED,
                                 GSocketConnection *connection,
                                 GObject           *source  G_GNUC_UNUSED,
                                 gpointer           user_data) {
    AXParameter   *handle = (AXParameter *)user_data;
    GInputStream  *in  = g_io_stream_get_input_stream(G_IO_STREAM(connection));
    GOutputStream *out = g_io_stream_get_output_stream(G_IO_STREAM(connection));

    GString *req = g_string_new(NULL);
    char buf[2048];
    int have_headers = 0;
    size_t header_end = 0;
    size_t content_length = 0;

    while (1) {
        gssize n = g_input_stream_read(in, buf, sizeof(buf), NULL, NULL);
        if (n <= 0) break;
        g_string_append_len(req, buf, n);
        if (!have_headers) {
            char *p = g_strstr_len(req->str, req->len, "\r\n\r\n");
            if (p) {
                have_headers = 1;
                header_end = (size_t)(p - req->str) + 4;
                content_length = http_parse_content_length(req->str, header_end);
            }
        }
        if (have_headers && req->len - header_end >= content_length) break;
        if (req->len > 262144) break; /* safety cap */
    }

    int is_get = 0, is_post = 0, is_settings = 0;
    if (have_headers) {
        if (g_str_has_prefix(req->str, "GET "))  is_get = 1;
        if (g_str_has_prefix(req->str, "POST ")) is_post = 1;
        const char *sp1 = strchr(req->str, ' ');
        if (sp1) {
            const char *path = sp1 + 1;
            const char *sp2 = strchr(path, ' ');
            size_t plen = sp2 ? (size_t)(sp2 - path) : strlen(path);
            const char *q = memchr(path, '?', plen);
            size_t match_len = q ? (size_t)(q - path) : plen;
            if (match_len >= 8 &&
                g_ascii_strncasecmp(path + match_len - 8, "settings", 8) == 0)
                is_settings = 1;
        }
    }

    if (is_settings && is_get) {
        gchar *json = http_build_settings_json(handle);
        http_send(out, "200 OK", "application/json", json);
        g_free(json);
    } else if (is_settings && is_post) {
        const char *body = req->str + header_end;
        size_t body_len = req->len - header_end;
        if (body_len > content_length) body_len = content_length;
        int applied = http_apply_settings(handle, body, body_len);
        syslog(LOG_INFO, "settings http: applied %d parameter(s)", applied);
        if (reload_timer_id) g_source_remove(reload_timer_id);
        reload_timer_id = g_timeout_add(300, debounced_restart, NULL);
        http_send(out, "200 OK", "text/plain", "OK");
    } else {
        http_send(out, "404 Not Found", "text/plain", "Not found");
    }

    g_string_free(req, TRUE);
    g_io_stream_close(G_IO_STREAM(connection), NULL, NULL);
    return TRUE;
}

static void http_server_start(AXParameter *handle) {
    GError *err = NULL;
    GSocketService *service = g_socket_service_new();
    GInetAddress   *addr    = g_inet_address_new_from_string("127.0.0.1");
    GSocketAddress *saddr   = g_inet_socket_address_new(addr, HTTP_PORT);

    if (!g_socket_listener_add_address(G_SOCKET_LISTENER(service), saddr,
                                       G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP,
                                       NULL, NULL, &err)) {
        syslog(LOG_WARNING, "settings http: bind 127.0.0.1:%d failed: %s",
               HTTP_PORT, err ? err->message : "unknown");
        if (err) g_error_free(err);
        g_object_unref(service);
    } else {
        g_signal_connect(service, "incoming", G_CALLBACK(http_on_incoming), handle);
        g_socket_service_start(service);
        syslog(LOG_INFO, "settings http server listening on 127.0.0.1:%d", HTTP_PORT);
    }
    g_object_unref(addr);
    g_object_unref(saddr);
}

/* ── signal handler ──────────────────────────────────────────────────────── */

static gboolean signal_handler(gpointer loop) {
    syslog(LOG_INFO, "stopping");
    stop_child();
    g_main_loop_quit((GMainLoop *)loop);
    return G_SOURCE_REMOVE;
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void) {
    GError *error = NULL;

    openlog(APP_NAME, LOG_PID, LOG_USER);
    syslog(LOG_INFO, "starting");

    /* Ensure localdata dir exists */
    mkdir("/usr/local/packages/Tailscale_VPN/localdata", 0755);

    /* Drop any stale auth-key sentinel from a previous run so we don't clear a
     * freshly configured key before it has been used. */
    unlink(SENTINEL_FILE);

    AXParameter *handle = ax_parameter_new(APP_NAME, &error);
    if (!handle) {
        syslog(LOG_ERR, "ax_parameter_new: %s",
               error ? error->message : "unknown");
        if (error) g_error_free(error);
        return 1;
    }
    g_ax_handle = handle;

    ensure_param(handle, "AdvertiseRoutes", "");

    load_config_cache(handle);
    write_config_file();
    start_child();

    const char *params[] = {
        "CustomServer", "AuthKey",
#ifdef HAS_PROXY_PORTS
        "HttpProxyPort", "Socks5Port",
#endif
        "AcceptDNS", "AcceptRoutes", "AdvertiseRoutes"
    };
    for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
        if (!ax_parameter_register_callback(handle, params[i],
                                            parameter_changed, handle, &error)) {
            syslog(LOG_WARNING, "register callback %s: %s",
                   params[i], error ? error->message : "unknown");
            if (error) { g_error_free(error); error = NULL; }
        }
    }

    http_server_start(handle);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, signal_handler, loop);
    g_unix_signal_add(SIGINT,  signal_handler, loop);
    g_timeout_add_seconds(60, watchdog_cb, NULL);
    g_timeout_add_seconds(5, authkey_sentinel_cb, NULL);

    syslog(LOG_INFO, "running — watching for parameter changes");
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    ax_parameter_free(handle);
    return 0;
}
