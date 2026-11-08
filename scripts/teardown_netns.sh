#!/usr/bin/env bash
# Removes the namespaces (the veth pair is deleted automatically with them).
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "Please run as root: sudo $0" >&2
    exit 1
fi

ip netns del ns_sender   2>/dev/null || true
ip netns del ns_receiver 2>/dev/null || true
echo "Namespaces removed."
