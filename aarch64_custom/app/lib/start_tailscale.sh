#!/bin/sh
# Make sure this script terminates any existing Tailscale processes before starting new ones

# Kill any existing tailscaled processes
pkill -f tailscaled || true

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
    --tun=userspace-networking &
TAILSCALED_PID=$!

# Wait for tailscaled to initialize
sleep 2

# Build up the command based on available parameters
TAILSCALE_CMD="$TAILSCALE_PATH --socket=$SOCKET_PATH up"

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

# Keep the script running to maintain the tailscaled process
logger -t "tailscale_script" "Tailscale VPN is running"
logger -t "tailscale_script" "To change settings, modify parameters in ACAP web interface"

# Wait for tailscaled process to exit
wait $TAILSCALED_PID
