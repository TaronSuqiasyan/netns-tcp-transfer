#!/usr/bin/env bash
# Creates two network namespaces connected by a veth pair:
#
#   [ ns_sender ] veth-s 10.0.0.1/24  <----->  veth-r 10.0.0.2/24 [ ns_receiver ]
#
# Must be run as root (sudo).
set -euo pipefail

NS_SENDER="ns_sender"
NS_RECEIVER="ns_receiver"
IF_SENDER="veth-s"
IF_RECEIVER="veth-r"
IP_SENDER="10.0.0.1/24"
IP_RECEIVER="10.0.0.2/24"

if [[ $EUID -ne 0 ]]; then
    echo "Please run as root: sudo $0" >&2
    exit 1
fi

# Start clean if a previous run left things behind
ip netns del "$NS_SENDER"   2>/dev/null || true
ip netns del "$NS_RECEIVER" 2>/dev/null || true

# 1. Create the namespaces
ip netns add "$NS_SENDER"
ip netns add "$NS_RECEIVER"

# 2. Create the veth pair (virtual patch cable)
ip link add "$IF_SENDER" type veth peer name "$IF_RECEIVER"

# 3. Put one end in each namespace
ip link set "$IF_SENDER"   netns "$NS_SENDER"
ip link set "$IF_RECEIVER" netns "$NS_RECEIVER"

# 4. Assign IP addresses
ip netns exec "$NS_SENDER"   ip addr add "$IP_SENDER"   dev "$IF_SENDER"
ip netns exec "$NS_RECEIVER" ip addr add "$IP_RECEIVER" dev "$IF_RECEIVER"

# 5. Bring interfaces up (including loopback)
ip netns exec "$NS_SENDER"   ip link set lo up
ip netns exec "$NS_SENDER"   ip link set "$IF_SENDER" up
ip netns exec "$NS_RECEIVER" ip link set lo up
ip netns exec "$NS_RECEIVER" ip link set "$IF_RECEIVER" up

echo "Namespaces ready. Testing connectivity with ping..."
ip netns exec "$NS_SENDER" ping -c 2 10.0.0.2
