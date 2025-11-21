//
// Copyright (C) 2013 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package researchproject.routing.queuegpsr;

import inet.routing.contract.IManetRouting;

//
// This module provides Greedy Perimeter Stateless Routing for Wireless Networks.
// The implementation supports both GG and RNG planarization algorithms.
//
// For more information on the routing algorithm, see the GPSR paper
// http://www.eecs.harvard.edu/~~htk/publication/2000-mobi-karp-kung.pdf
//
simple QueueGpsr like IManetRouting
{
    parameters:
        // properties
        @display("i=block/routing");

        // context parameters
        string interfaceTableModule = default("^.interfaceTable");   // The path to the InterfaceTable module
        string routingTableModule = default("^.ipv4.routingTable");
        string networkProtocolModule = default("^.ipv4.ip");
        string outputInterface = default("wlan0");

        // GPSR parameters
        string planarizationMode @enum("", "GG", "RNG") = default("GG");
        string interfaces = default("*");
        double beaconInterval @unit(s) = default(10s);
        double maxJitter @unit(s) = default(0.5 * beaconInterval);
        double neighborValidityInterval @unit(s) = default(4.5 * beaconInterval);
        int positionByteLength @unit(B) = default(2 * 4B);

        // delay tiebreaker parameters (Phase 2/3)
        bool enableDelayTiebreaker = default(false);
        double distanceEqualityThreshold @unit(m) = default(1.0m);  // Neighbors within this distance are considered "equal"
        double delayEstimationFactor @unit(s) = default(0.001s);    // Estimated delay per meter (Phase 2 uses distance-based simulation)
    // Phase 3: queue-aware delay estimation
    bool enableQueueDelay = default(false); // if true, include TX backlog / bitrate term in delay estimate

        // CPU offload capacity parameters (Phase 4: compute offloading)
        double cpuTotalHz = default(2e9);  // total CPU capacity in Hz (e.g., 2 GHz)
        double offloadShareMin = default(0.2);        // minimum fraction of CPU available for offloading
        double offloadShareMax = default(0.6);        // maximum fraction of CPU available for offloading

        // visualization parameters
        bool displayBubbles = default(false);   // display bubble messages about changes in routing state for packets
        
        // statistics
        @signal[tiebreakerActivations](type=long);
        @statistic[tiebreakerActivations](title="Tiebreaker activations"; source=tiebreakerActivations; record=count,vector?; interpolationmode=none);
    gates:
        input ipIn;
        output ipOut;
}

