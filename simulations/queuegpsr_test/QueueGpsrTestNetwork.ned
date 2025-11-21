//
// QueueGPSR Test Network
// Phase 2/3: Testing delay-aware tiebreaker mechanism
//

package simulations.queuegpsr_test;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import researchproject.node.QueueGpsrRouter;

network QueueGpsrTestNetwork
{
    parameters:
        int numHosts = default(20);
        @display("bgb=1400,1200");

    submodules:
        radioMedium: UnitDiskRadioMedium {
            @display("p=100,50");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=100,150");
            config = xml("<config><interface hosts='**' address='10.0.0.x' netmask='255.255.255.0'/></config>");
        }

        host[numHosts]: QueueGpsrRouter {
            @display("p=,,grid,5,250,250");
        }
}
