//
// Copyright (C) 2020 OpenSim Ltd.
// Copyright (C) 2025 Research Project
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package researchproject.node;

import inet.node.inet.AdhocHost;
import researchproject.routing.queuegpsr.QueueGpsr;

//
// Router using QueueGpsr (local copy with delay tiebreaker)
//
module QueueGpsrRouter extends AdhocHost
{
    parameters:
        // Inherit all parameters from AdhocHost (including numApps)
        @display("i=device/cellphone");
        
    submodules:
        queueGpsr: QueueGpsr {
            parameters:
                @display("p=825,226");
        }
    connections:
        queueGpsr.ipOut --> tn.in++;
        queueGpsr.ipIn <-- tn.out++;
}

