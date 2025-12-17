/**
 * Simple file-based configuration updater for Tailscale
 * This avoids the AXParameter system and just writes directly to a config file
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

#define APP_NAME "serverconfig"
#define APP_DIR "/usr/local/packages/serverconfig"
#define STATE_DIR APP_DIR "/localdata"
#define CONFIG_FILE STATE_DIR "/config.txt"
#define SCRIPT_PATH "/usr/local/packages/serverconfig/start_tailscale.sh"
#define SCRIPT_SOURCE "/usr/local/packages/serverconfig/lib/start_tailscale.sh"

static gboolean signal_handler(gpointer loop) {
    g_main_loop_quit((GMainLoop*)loop);
    syslog(LOG_INFO, "Configuration updater stopping.");
    return G_SOURCE_REMOVE;
}

// Create localdata directory
static void ensure_localdata_exists(void) {
    struct stat st = {0};
    
    if (stat(STATE_DIR, &st) == -1) {
        if (mkdir(STATE_DIR, 0755) != 0) {
            syslog(LOG_ERR, "Failed to create localdata directory: %s", strerror(errno));
        } else {
            syslog(LOG_INFO, "Created localdata directory: %s", STATE_DIR);
        }
    }
}

// Copy script from lib folder to main directory
static void copy_script_file(void) {
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    int source_fd, dest_fd;
    
    syslog(LOG_INFO, "Copying script from %s to %s", SCRIPT_SOURCE, SCRIPT_PATH);
    
    // Open source file
    source_fd = open(SCRIPT_SOURCE, O_RDONLY);
    if (source_fd < 0) {
        syslog(LOG_ERR, "Failed to open source script: %s", strerror(errno));
        return;
    }
    
    // Open destination file (create if doesn't exist, truncate if exists)
    dest_fd = open(SCRIPT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (dest_fd < 0) {
        syslog(LOG_ERR, "Failed to open destination script: %s", strerror(errno));
        close(source_fd);
        return;
    }
    
    // Copy the file
    while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            syslog(LOG_ERR, "Error writing to destination file: %s", strerror(errno));
            close(source_fd);
            close(dest_fd);
            return;
        }
    }
    
    // Close file descriptors
    close(source_fd);
    close(dest_fd);
    
    // Make the script executable
    if (chmod(SCRIPT_PATH, 0755) != 0) {
        syslog(LOG_ERR, "Failed to make script executable: %s", strerror(errno));
        return;
    }
    
    syslog(LOG_INFO, "Script copied and made executable successfully");
}

// Execute the Tailscale script
static void start_tailscale(void) {
    syslog(LOG_INFO, "Starting Tailscale VPN script");
    
    // Check if script exists, if not, copy it
    struct stat st;
    if (stat(SCRIPT_PATH, &st) != 0) {
        syslog(LOG_INFO, "Script not found at %s, copying from lib folder", SCRIPT_PATH);
        copy_script_file();
    }
    
    // Fork and execute the script
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Failed to fork for Tailscale script: %s", strerror(errno));
        return;
    } else if (pid == 0) {
        // Child process - execute the script
        execl(SCRIPT_PATH, "start_tailscale.sh", NULL);
        
        // If we get here, execl failed
        syslog(LOG_ERR, "Failed to execute Tailscale script: %s", strerror(errno));
        _exit(1);
    }
    
    syslog(LOG_INFO, "Tailscale script started with PID: %d", pid);
}

// Update the configuration file with current parameter values
static void update_config_file(AXParameter* handle) {
    GError* error = NULL;
    gchar* server_value = NULL;
    gchar* key_value = NULL;
    FILE* file;
    
    // Ensure localdata directory exists
    ensure_localdata_exists();
    
    // Get parameter values
    if (!ax_parameter_get(handle, "CustomServer", &server_value, &error)) {
        syslog(LOG_ERR, "Failed to get CustomServer: %s", 
               error ? error->message : "unknown error");
        if (error) g_error_free(error);
        error = NULL;
        server_value = g_strdup("");
    }
    
    if (!ax_parameter_get(handle, "AuthKey", &key_value, &error)) {
        syslog(LOG_ERR, "Failed to get AuthKey: %s", 
               error ? error->message : "unknown error");
        if (error) g_error_free(error);
        key_value = g_strdup("");
    }
    
    // Write to config file in localdata
    file = fopen(CONFIG_FILE, "w");
    if (file) {
        fprintf(file, "custom_server=%s\n", server_value ? server_value : "");
        fprintf(file, "auth_key=%s\n", key_value ? key_value : "");
        fclose(file);
        
        // Set permissions to ensure the file is readable
        chmod(CONFIG_FILE, 0644);
        
        syslog(LOG_INFO, "Updated configuration file in local custom_server=%s", 
               server_value ? server_value : "");
        syslog(LOG_INFO, "Updated configuration file in local auth_key=%s", 
               key_value && strlen(key_value) > 0 ? "(set)" : "(empty)");
    } else {
        syslog(LOG_ERR, "Failed to open config file for writing: %s", strerror(errno));
    }
    
    // Clean up
    g_free(server_value);
    g_free(key_value);
}

// Handle parameter changes
static void parameter_changed(const gchar* name, const gchar* value, gpointer handle_void_ptr) {
    AXParameter* handle = handle_void_ptr;
    
    // Extract simple parameter name from the fully qualified name
    const char* simple_name = name;
    const char* prefix = "root." APP_NAME ".";
    if (strncmp(name, prefix, strlen(prefix)) == 0) {
        simple_name = name + strlen(prefix);
    }
    
    syslog(LOG_INFO, "Parameter changed: %s = %s", simple_name, value);
    
    // Update config file whenever any parameter changes
    update_config_file(handle);
    
    // Restart Tailscale to apply the new settings
    start_tailscale();
}

int main(void) {
    GError* error = NULL;
    GMainLoop* loop = NULL;

    // Open syslog for logging
    openlog(APP_NAME, LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Config updater starting");

    // Initialize parameter handling
    AXParameter* handle = ax_parameter_new(APP_NAME, &error);
    if (handle == NULL) {
        syslog(LOG_ERR, "Failed to initialize parameters: %s", 
               error ? error->message : "unknown error");
        if (error) g_error_free(error);
        exit(1);
    }

    // Ensure localdata directory exists
    ensure_localdata_exists();
    
    // Ensure script is copied from lib folder
    copy_script_file();

    // Create initial config file
    update_config_file(handle);
    
    // Start Tailscale VPN script
    start_tailscale();

    // Register for parameter changes
    if (!ax_parameter_register_callback(handle, "CustomServer", parameter_changed, handle, &error)) {
        syslog(LOG_ERR, "Failed to register CustomServer callback: %s", 
               error ? error->message : "unknown error");
        if (error) g_error_free(error);
        error = NULL;
    }
    
    if (!ax_parameter_register_callback(handle, "AuthKey", parameter_changed, handle, &error)) {
        syslog(LOG_ERR, "Failed to register AuthKey callback: %s", 
               error ? error->message : "unknown error");
        if (error) g_error_free(error);
    }
    
    // Register for parameter changes with fully qualified names as fallback
    if (!ax_parameter_register_callback(handle, "root." APP_NAME ".CustomServer", parameter_changed, handle, NULL)) {
        syslog(LOG_INFO, "Fallback CustomServer registration failed (this may be normal)");
    }
    if (!ax_parameter_register_callback(handle, "root." APP_NAME ".AuthKey", parameter_changed, handle, NULL)) {
        syslog(LOG_INFO, "Fallback AuthKey registration failed (this may be normal)");
    }

    // Set up main loop
    loop = g_main_loop_new(NULL, FALSE);
    g_unix_signal_add(SIGTERM, signal_handler, loop);
    g_unix_signal_add(SIGINT, signal_handler, loop);
    
    syslog(LOG_INFO, "Config updater running. Waiting for parameter changes...");
    g_main_loop_run(loop);

    // Clean up
    g_main_loop_unref(loop);
    ax_parameter_free(handle);
    
    return 0;
}
