# Queue-Aware GPSR Research Project - Quick Start

## Overview
This project implements queue-aware extensions to GPSR routing following INET framework conventions and OMNeT++ best practices.

## Project Structure at a Glance

```
Research_project/
├── src/                                    # Model implementation (INET-aligned)
│   └── researchproject/
│       ├── routing/
│       │   └── queuegpsr/                 # Queue-aware GPSR routing
│       ├── linklayer/
│       │   └── queue/                     # Queue inspection utilities
│       └── common/                        # Shared utilities
│
├── simulations/                           # Experiment configurations
│   ├── baseline_gpsr/                     # Phase 1: Standard GPSR
│   ├── delay_tiebreaker/                  # Phase 2: Delay-aware
│   ├── queue_aware/                       # Phase 3: Queue-aware
│   ├── two_hop_peek/                      # Phase 4: Two-hop lookahead
│   └── compute_offload/                   # Phase 5: Compute-aware
│
├── results/                               # Simulation outputs (gitignored)
├── scripts/                               # Automation tools
├── docs/                                  # Documentation
└── README.md                              # This file
```

## Current Status: Phase 1 Verification In Progress

### ✅ Completed
- Project structure created
- INET GPSR research complete (see `docs/PHASE1_RESEARCH_FINDINGS.md`)
- Baseline scenario configured and tested
- Test run (10s, 10 seeds) successful
- Metrics sources identified (see `docs/PHASE1_METRICS_INVENTORY.md`)
- Results validation complete

### ⏳ In Progress
- Full 500s baseline run pending
- Statistics extraction pending
- Baseline results documentation pending

### 🎯 Next Action
Run full baseline simulation:
```bash
./scripts/run_baseline_full.sh
# Or manually: see docs/PHASE1_STATUS.txt
```

**See:** `docs/PHASE1_STATUS.txt` for complete Phase 1 status

---

### 1. Verify Environment
```bash
# Check OMNeT++ installation
which opp_run
omnetpp --version

# Check INET availability
ls ../inet4.5/
```

### 2. Research INET First! 🔍
**CRITICAL: Before writing any code, study INET's structure:**

```bash
# Study INET's GPSR implementation
cat ../inet4.5/src/inet/routing/gpsr/Gpsr.ned
cat ../inet4.5/src/inet/routing/gpsr/Gpsr.h

# Review GPSR examples
cat ../inet4.5/examples/manetrouting/gpsr/omnetpp.ini

# Explore INET structure
ls ../inet4.5/src/inet/routing/
ls ../inet4.5/src/inet/linklayer/
```

**Read:** `docs/INET_RESEARCH_GUIDE.md` for detailed investigation steps.

### 3. Understand the Architecture
Read the documentation:
```bash
cat README.md                      # Project overview
cat docs/DESIGN.md                 # Architecture details
cat docs/STRUCTURE_MAP.md          # Directory organization
cat docs/WORKFLOW.md               # Development phases
```

### 4. Test Baseline Scenario
The baseline scenario is ready to run with INET's standard GPSR:

```bash
cd simulations/baseline_gpsr
cat README.md  # Read scenario description

# Run baseline (uses standard INET GPSR)
opp_run -m -u Cmdenv -c Baseline -n ../../src:.:../inet4.5/src --result-dir=../../results
```

### 5. Build Custom Modules (When Ready)
After researching INET and designing your extensions:

```bash
cd src
opp_makemake -f --deep -O ../out -I../../inet4.5/src -L../../inet4.5/src -lINET
make
```

## Development Workflow

### Research-First Approach
```
1. Research INET → 2. Design → 3. Implement → 4. Configure → 5. Test → 6. Evaluate
        ↑                                                                      |
        └──────────────────────── Document & Iterate ←────────────────────────┘
```

### Key Principles

1. **Always Research INET First**
   - Find analogous modules in INET
   - Study implementation patterns
   - Mirror conventions exactly

2. **Maintain Package-Directory Mapping**
   ```
   Package: researchproject.routing.queuegpsr
   Path:    src/researchproject/routing/queuegpsr/
   ```

3. **Separate Model from Scenarios**
   - Model code → `src/`
   - Experiments → `simulations/`
   - Results → `results/`

4. **Follow INET Conventions**
   - OSI layer organization
   - Naming patterns
   - Module interfaces
   - Parameter conventions

## Implementation Phases

### ✅ Current: Project Setup
- [x] Directory structure
- [x] Package hierarchy
- [x] Documentation framework
- [x] Baseline scenario ready

### 🚧 Phase 1: Baseline GPSR
**Before coding:**
1. Study INET's GPSR thoroughly
2. Document structure in `docs/`
3. Design QueueGpsr module

**Implementation:**
- Create QueueGpsr module (initially identical to INET GPSR)
- Verify behavior matches INET baseline
- Establish metrics collection

### 🚧 Phase 2: Delay Tiebreaker
**Before coding:**
1. Research INET timing utilities
2. Study delay measurement in other protocols

**Implementation:**
- Add delay measurement
- Extend neighbor table
- Modify next-hop selection

### 🚧 Phase 3-5: Advanced Features
See `docs/WORKFLOW.md` for detailed phase plans.

## Running Simulations

### Single Configuration
```bash
cd simulations/baseline_gpsr
opp_run -m -u Cmdenv -c Baseline -n ../../src:. --result-dir=../../results
```

### Using Scripts
```bash
# Run baseline
./scripts/run_baseline.sh

# Run all simulations (when implemented)
./scripts/run_all_simulations.sh

# Analyze results
python3 scripts/analyze_results.py
```

### From IDE
1. Open project in OMNeT++ IDE
2. Build project (Project → Build All)
3. Right-click on `omnetpp.ini` → Run As → OMNeT++ Simulation

## File Organization Rules

### Where to Put What

| Content Type | Location | Example |
|-------------|----------|---------|
| Routing protocol logic | `src/researchproject/routing/queuegpsr/` | `QueueGpsr.cc` |
| Queue inspection | `src/researchproject/linklayer/queue/` | `QueueInspector.cc` |
| Shared utilities | `src/researchproject/common/` | Helper functions |
| Network topologies | `simulations/<scenario>/` | `BaselineNetwork.ned` |
| Run configurations | `simulations/<scenario>/` | `omnetpp.ini` |
| Simulation results | `results/` | `*.sca`, `*.vec` |
| Build outputs | `out/` | `*.o`, executables |
| Helper scripts | `scripts/` | Bash/Python tools |
| Documentation | `docs/` | Design notes |

### Package Naming

All NED packages start with `researchproject` and mirror directory paths:

```ned
package researchproject;                          // src/researchproject/
package researchproject.routing;                  // src/researchproject/routing/
package researchproject.routing.queuegpsr;        // src/researchproject/routing/queuegpsr/
package researchproject.linklayer.queue;          // src/researchproject/linklayer/queue/
package researchproject.simulations.baseline_gpsr; // simulations/baseline_gpsr/
```

## Common Commands

```bash
# Generate Makefile
cd src && opp_makemake -f --deep -O ../out -I../../inet4.5/src -L../../inet4.5/src -lINET

# Build
make

# Clean build artifacts
make clean

# Clean results
rm -rf results/*.sca results/*.vec

# Run simulation (cmdenv)
opp_run -m -u Cmdenv -c ConfigName -n ./src:./simulations --result-dir=./results

# Run simulation (GUI)
opp_run -m -u Qtenv -c ConfigName -n ./src:./simulations --result-dir=./results

# List available configurations
opp_run -m -u Cmdenv -a -n ./src:./simulations

# Process results
scavetool scalar -p "*.sca" -O output.csv
```

## Important Files

### Must Read
- `README.md` - This file
- `docs/INET_RESEARCH_GUIDE.md` - How to study INET
- `docs/DESIGN.md` - Architecture overview
- `docs/WORKFLOW.md` - Development phases

### Configuration
- `Makefile` - Build configuration
- `simulations/baseline_gpsr/omnetpp.ini` - Baseline config
- `src/researchproject/package.ned` - Root package

### Templates
- `simulations/baseline_gpsr/` - Example scenario structure
- `src/researchproject/routing/queuegpsr/package.ned` - Package definition

## Tips & Best Practices

### Before Each Implementation Step
1. ✅ Research analogous INET modules
2. ✅ Document findings in `docs/`
3. ✅ Design module structure
4. ✅ Plan package placement
5. ✅ Then implement

### While Implementing
- Follow INET naming conventions exactly
- Keep package names matching directory paths
- Separate concerns by OSI layer
- Document assumptions inline
- Test incrementally

### After Implementation
- Verify against baseline
- Run multiple seeds for statistics
- Document results
- Update design docs
- Plan next iteration

## Getting Help

### Resources
- **INET User Guide:** https://inet.omnetpp.org/docs/users-guide/
- **OMNeT++ Manual:** https://doc.omnetpp.org/omnetpp/manual/
- **INET API:** https://doc.omnetpp.org/inet/api-current/
- **GPSR Paper:** Karp & Kung, MobiCom 2000

### Local Documentation
```bash
ls docs/                    # All documentation
cat docs/DESIGN.md          # Architecture
cat docs/WORKFLOW.md        # Development process
cat docs/INET_RESEARCH_GUIDE.md  # INET investigation
```

### Check INET Source
The best documentation is INET's source code:
```bash
cd ../inet4.5/src/inet
find . -name "*.ned" | grep routing
find . -name "*.h" | grep -i gpsr
```

## Troubleshooting

### "Package not found"
- Check NED path: `-n ../../src:.`
- Verify package names match directory paths
- Ensure `package.ned` files exist in all directories

### "Module type not found"
- Check NED path includes INET: `-n ../../src:.:../inet4.5/src`
- Verify module is properly imported in NED files

### "Cannot find library"
- Check INET library path: `-L../../inet4.5/src`
- Verify INET is built: `ls ../inet4.5/src/libINET.*`

### Build Errors
- Clean and rebuild: `make clean && make`
- Regenerate Makefile: `make makemake && make`
- Check compiler flags in Makefile

## Next Steps

1. **Read Documentation**
   ```bash
   cat docs/INET_RESEARCH_GUIDE.md
   cat docs/WORKFLOW.md
   ```

2. **Study INET GPSR**
   ```bash
   cat ../inet4.5/src/inet/routing/gpsr/Gpsr.ned
   cat ../inet4.5/examples/manetrouting/gpsr/omnetpp.ini
   ```

3. **Test Baseline**
   ```bash
   cd simulations/baseline_gpsr
   opp_run -m -u Cmdenv -c Baseline -n ../../src:.:../../inet4.5/src --result-dir=../../results
   ```

4. **Plan Phase 1**
   - Document INET GPSR structure
   - Design QueueGpsr module
   - Create implementation plan

---

**Remember: Research INET first, then implement!** 🔍

The structure is ready. The path is clear. Now study INET thoroughly before writing code.
