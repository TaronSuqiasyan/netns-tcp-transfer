#!/usr/bin/env bash
# End-to-end demo: builds, sets up namespaces, transfers a file, verifies it.
# Run from the repository root:  sudo ./scripts/run_demo.sh
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Please run as root: sudo $0" >&2
    exit 1
fi

cd "$(dirname "$0")/.."

PORT=5000
FILE="sample.txt"
OUT="received_$FILE"

make
./scripts/setup_netns.sh

rm -f "receiver/$OUT"

# Start the receiver inside its namespace (background)
ip netns exec ns_receiver ./receiver/receiver 10.0.0.2 "$PORT" "$OUT" &
RECV_PID=$!
sleep 1

# Run the sender inside its namespace
ip netns exec ns_sender ./sender/sender 10.0.0.2 "$PORT" "$FILE"

wait "$RECV_PID"

echo
if cmp -s "sender/$FILE" "receiver/$OUT"; then
    echo "SUCCESS: files are identical."
    sha256sum "sender/$FILE" "receiver/$OUT"
else
    echo "FAILURE: files differ." >&2
    exit 1
fi

echo
echo "Run ./scripts/teardown_netns.sh (as root) to clean up."
