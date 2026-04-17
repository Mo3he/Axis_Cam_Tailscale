// Copyright (C) 2024  Mo3he
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * ACAP parameter bridge for Tailscale VPN (ROOT / kernel networking variant).
 * Same structure as regular param_bridge.c but without proxy port params.
 */

#include <axsdk/axparameter.h>
#include <glib-unix.h>
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

#define APP_NAME    "Tailscale_VPN"
#define CONFIG_FILE "/usr/local/packages/Tailscale_VPN/localdata/params.conf"
#define RUN_SCRIPT  "/usr/local/packages/Tailscale_VPN/Tailscale_VPN_run"

static AXParameter *g_ax_handle  = NULL;
static pid_t child_pid = -1;
static guint reload_timer_id = 0;

static char *cfg_custom_server = NULL;
static char *cfg_auth_key      = NULL;

static void cache_set(char **field, const char *value) {
    if (!value) return;
    free(*field);
    *field = strdup(value);
}

static const char *cache_get(char **field, const char *fallback) {
    return (*field && **field) ? *field : fallback;
}

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
        execl(RUN_SCRIPT, RUN_SCRIPT, NULL);
        syslog(LOG_ERR, "execl %s failed: %s", RUN_SCRIPT, strerror(errno));
        _exit(1);
    }
    child_pid = pid;
    syslog(LOG_INFO, "started %s (pid %d)", RUN_SCRIPT, child_pid);
}

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

    LOAD("CustomServer", cfg_custom_server)
    LOAD("AuthKey",      cfg_auth_key)
#undef LOAD
}

static void write_config_file(void) {
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) {
        syslog(LOG_ERR, "cannot open config file %s: %s",
               CONFIG_FILE, strerror(errno));
        return;
    }
    fprintf(f, "CUSTOM_SERVER=%s\n", cache_get(&cfg_custom_server, ""));
    fprintf(f, "AUTH_KEY=%s\n",      cache_get(&cfg_auth_key,      ""));
    fclose(f);
    chmod(CONFIG_FILE, 0600);
    syslog(LOG_INFO, "config updated: server=%s",
           cache_get(&cfg_custom_server, "(default)"));
}

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

    if      (strcmp(short_name, "CustomServer") == 0) cache_set(&cfg_custom_server, value);
    else if (strcmp(short_name, "AuthKey")      == 0) cache_set(&cfg_auth_key,      value);

    if (reload_timer_id)
        g_source_remove(reload_timer_id);
    reload_timer_id = g_timeout_add(300, debounced_restart, NULL);
}

static gboolean signal_handler(gpointer loop) {
    syslog(LOG_INFO, "stopping");
    stop_child();
    g_main_loop_quit((GMainLoop *)loop);
    return G_SOURCE_REMOVE;
}

int main(void) {
    GError *error = NULL;

    openlog(APP_NAME, LOG_PID, LOG_USER);
    syslog(LOG_INFO, "starting (root mode)");

    mkdir("/usr/local/packages/Tailscale_VPN/localdata", 0755);

    AXParameter *handle = ax_parameter_new(APP_NAME, &error);
    if (!handle) {
        syslog(LOG_ERR, "ax_parameter_new: %s",
               error ? error->message : "unknown");
        if (error) g_error_free(error);
        return 1;
    }
    g_ax_handle = handle;

    load_config_cache(handle);
    write_config_file();
    start_child();

    const char *params[] = { "CustomServer", "AuthKey" };
    for (size_t i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
        if (!ax_parameter_register_callback(handle, params[i],
                                            parameter_changed, handle, &error)) {
            syslog(LOG_WARNING, "register callback %s: %s",
                   params[i], error ? error->message : "unknown");
            if (error) { g_error_free(error); error = NULL; }
        }
    }

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, signal_handler, loop);
    g_unix_signal_add(SIGINT,  signal_handler, loop);
    g_timeout_add_seconds(60, watchdog_cb, NULL);

    syslog(LOG_INFO, "running — watching for parameter changes");
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    ax_parameter_free(handle);
    return 0;
}
