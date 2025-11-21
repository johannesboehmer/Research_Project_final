//
// Delay-Aware GPSR Network Topology
// Phase 2: Testing delay tiebreaker with congestion
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

package simulations.delay_tiebreaker;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.node.inet.ManetRouter;

//
// Network topology for delay tiebreaker evaluation
// Uses QueueGpsrRouter nodes with delay-aware next-hop selection
//
network DelayTiebreakerNetwork
{
    parameters:
        int numHosts = default(20);

        @display("bgb=1400,1200;bgg=100,1,grey95");
        @figure[title](type=label; pos=10,10; anchor=nw; color=darkblue; text="Delay Tiebreaker GPSR Network");

    submodules:
        // Network configurator
        configurator: Ipv4NetworkConfigurator {
            @display("p=99,82;is=s");
            config = xml("<config><interface hosts='**' address='10.0.0.x' netmask='255.255.255.0'/></config>");
        }

        // Radio medium (IEEE 802.11 Scalar for realistic MAC/PHY)
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=99,267;is=s");
        }

        // Host nodes using delay-aware GPSR (ManetRouter with routing submodule)
        host[numHosts]: ManetRouter {
            @display("p=,,ring");
        }
}
