#!/bin/sh
# Make sure this script terminates any existing Tailscale processes before starting new ones

# Kill any existing tailscaled processes
killall tailscaled 2>/dev/null || true

# Simple script to start Tailscale with custom configuration
APP_DIR="/usr/local/packages/serverconfig"
STATE_DIR="$APP_DIR/localdata"
CONFIG_FILE="$STATE_DIR/config.txt"
TAILSCALED_PATH="$APP_DIR/lib/tailscaled"
TAILSCALE_PATH="$APP_DIR/lib/tailscale"
SOCKET_PATH="$STATE_DIR/tailscaled.sock"

# Create localdata directory if it doesn't exist
mkdir -p "$STATE_DIR"

# Log to syslog
logger -t "tailscale_script" "Starting Tailscale VPN service"

# Set execute permissions
chmod 755 $TAILSCALED_PATH
chmod 755 $TAILSCALE_PATH

# Read configuration (if exists)
CUSTOM_SERVER=""
AUTH_KEY=""
if [ -f "$CONFIG_FILE" ]; then
    logger -t "tailscale_script" "Reading configuration from $CONFIG_FILE"
    # Read values from config file
    while IFS='=' read -r key value; do
        case "$key" in
            "custom_server") CUSTOM_SERVER="$value" ;;
            "auth_key") AUTH_KEY="$value" ;;
        esac
    done < "$CONFIG_FILE"
fi

# Start tailscaled with state stored in localdata
logger -t "tailscale_script" "Starting tailscaled daemon"
$TAILSCALED_PATH \
    --state="$STATE_DIR/tailscaled.state" \
    --socket=$SOCKET_PATH \
    --socks5-server=localhost:1055 \
    --outbound-http-proxy-listen=localhost:8080 \
    --tun=userspace-networking \
    2>&1 | logger -t "tailscale_script" &
TAILSCALED_PID=$!

# Wait for tailscaled to initialize
sleep 2

# Build up the command based on available parameters
TAILSCALE_CMD="$TAILSCALE_PATH --socket=$SOCKET_PATH up --hostname=$(hostname)"

if [ -n "$CUSTOM_SERVER" ]; then
    logger -t "tailscale_script" "Using custom server: $CUSTOM_SERVER"
    TAILSCALE_CMD="$TAILSCALE_CMD --login-server $CUSTOM_SERVER"
fi

if [ -n "$AUTH_KEY" ]; then
    logger -t "tailscale_script" "Using authentication key"
    TAILSCALE_CMD="$TAILSCALE_CMD --authkey $AUTH_KEY"
fi

# Connect to Tailscale network
logger -t "tailscale_script" "Running: $TAILSCALE_CMD"
eval $TAILSCALE_CMD
UP_EXIT=$?

# Clear auth key from config after first use — Tailscale auth keys are single-use
# and the node identity is persisted in tailscaled.state, so the key is no longer needed.
if [ -n "$AUTH_KEY" ] && [ "$UP_EXIT" -eq 0 ] && [ -f "$CONFIG_FILE" ]; then
    logger -t "tailscale_script" "Clearing auth key from config after successful authentication"
    printf 'custom_server=%s\nauth_key=\n' "$CUSTOM_SERVER" > "$CONFIG_FILE"
fi

# Keep the script running to maintain the tailscaled process
logger -t "tailscale_script" "Tailscale VPN is running"
logger -t "tailscale_script" "HTTP/HTTPS proxy: http://127.0.0.1:8080"
logger -t "tailscale_script" "SOCKS5 proxy:     127.0.0.1:1055"
logger -t "tailscale_script" "To change settings, modify parameters in ACAP web interface"

# Wait for tailscaled process to exit
wait $TAILSCALED_PID
