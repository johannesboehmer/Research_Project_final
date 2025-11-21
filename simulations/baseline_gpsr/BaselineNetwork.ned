//
// Baseline GPSR Network Topology
//
// This network serves as the baseline for comparing queue-aware
// extensions. It uses standard INET GPSR routing.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

package simulations.baseline_gpsr;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.gpsr.GpsrRouter;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;

//
// Baseline network topology for GPSR evaluation
// Uses standard INET GpsrRouter nodes
//
network BaselineNetwork
{
    parameters:
        int numHosts = default(20);
        
        @display("bgb=1000,1000;bgg=100,1,grey95");
        @figure[title](type=label; pos=10,10; anchor=nw; color=darkblue; text="Baseline GPSR Network");
        
    submodules:
        // Network configurator
        configurator: Ipv4NetworkConfigurator {
            @display("p=100,150;is=s");
        }
        
        // Radio medium (UnitDisk for Step 0 verification, can switch to Ieee80211Scalar later)
        radioMedium: UnitDiskRadioMedium {
            @display("p=100,250;is=s");
        }
        
        // Mobile hosts with GPSR routing (using INET's GpsrRouter)
        host[numHosts]: GpsrRouter {
            @display("p=uniform(0,1000),uniform(0,1000)");
        }
}
