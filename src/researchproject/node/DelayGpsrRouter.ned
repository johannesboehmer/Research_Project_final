//
// Copyright (C) 2025 Research Project
// GPSR Router with Delay Tiebreaker
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

package researchproject.node;

import inet.node.inet.AdhocHost;
import researchproject.routing.gpsr.GpsrWithDelayTiebreaker;

//
// Ad-hoc wireless router using GPSR with delay-aware tiebreaker.
// 
// Extends INET's AdhocHost and replaces standard GPSR routing module
// with delay-aware version for Phase 2 evaluation.
//
module DelayGpsrRouter extends AdhocHost
{
    submodules:
        gpsr: GpsrWithDelayTiebreaker {
            parameters:
                @display("p=825,226");
        }
    connections:
        gpsr.ipOut --> tn.in++;
        gpsr.ipIn <-- tn.out++;
}
