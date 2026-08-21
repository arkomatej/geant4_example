#!/bin/bash
#
# Batch driver: sweeps proton energy x hadronic inelastic model.
#
#   ./run_batch.sh                      # full sweep, 8 energies x 4 models
#   DRY_RUN=1 ./run_batch.sh            # list what would run, touch nothing
#   NUM_PROTONS=10000 ./run_batch.sh    # quick end-to-end shakedown
#   JOBS=4 ./run_batch.sh               # 4 runs concurrently
#   ENERGIES="185" MODELS="BERT BIC" ./run_batch.sh
#
# Every run writes into $OUTDIR. Re-running skips any run whose output is
# already there, so an interrupted sweep resumes where it stopped (FORCE=1
# overrides). Per-run stdout goes to $OUTDIR/logs/<tag>.log and a summary
# table lands in $OUTDIR/manifest.csv.

set -uo pipefail   # deliberately NOT -e: a failed run must not kill the sweep

# ----------------------------------------------------------------------------
# Configuration -- all overridable from the environment
# ----------------------------------------------------------------------------
GEANT4_ENV="${GEANT4_ENV:-/nfs/Tango/users/mateja/geant4/geant4-install/bin/geant4.sh}"
EXE="${EXE:-build/exampleB1}"
OUTDIR="${OUTDIR:-results}"
NUM_PROTONS="${NUM_PROTONS:-100000000}"
PHYS_OPT="${PHYS_OPT:-MSC}"          # MSC or SS
JOBS="${JOBS:-1}"                     # concurrent runs
DRY_RUN="${DRY_RUN:-0}"
FORCE="${FORCE:-0}"                   # 1 = re-run even if output exists

read -r -a ENERGIES <<< "${ENERGIES:-10 30 60 100 185 500 1000 5000}"
read -r -a MODELS   <<< "${MODELS:-BERT BERT_HP BIC BIC_HP}"

LOGDIR="$OUTDIR/logs"
STATDIR="$OUTDIR/.status"
MACRODIR="$OUTDIR/.macros"

# ----------------------------------------------------------------------------
# Pre-flight -- fail here rather than three days into a sweep
# ----------------------------------------------------------------------------
if [[ -f "$GEANT4_ENV" ]]; then
    # shellcheck disable=SC1090
    source "$GEANT4_ENV"
else
    echo "WARNING: Geant4 env script not found at $GEANT4_ENV" >&2
    echo "         Continuing in case the environment is already set up." >&2
fi

if [[ ! -x "$EXE" ]]; then
    echo "ERROR: executable '$EXE' not found or not executable." >&2
    echo "       Build first, or point EXE= at the right path." >&2
    exit 1
fi

# _HP lists need the G4NDL neutron data. Missing data is a fatal Geant4 error
# at initialisation, so check once up front instead of discovering it on run 3.
if [[ " ${MODELS[*]} " == *"_HP"* ]]; then
    if [[ -z "${G4NEUTRONHPDATA:-}" ]]; then
        echo "ERROR: an _HP model was requested but G4NEUTRONHPDATA is unset." >&2
        echo "       The G4NDL dataset is required; source geant4.sh or install it." >&2
        exit 1
    elif [[ ! -d "$G4NEUTRONHPDATA" ]]; then
        echo "ERROR: G4NEUTRONHPDATA points at '$G4NEUTRONHPDATA', which does not exist." >&2
        exit 1
    fi
fi

if (( NUM_PROTONS < 1 )); then
    echo "ERROR: NUM_PROTONS must be >= 1 (got $NUM_PROTONS)." >&2
    exit 1
fi

mkdir -p "$OUTDIR" "$LOGDIR" "$STATDIR" "$MACRODIR" || exit 1

# Progress every ~5%, but never 0 (Geant4 rejects a zero interval).
PROGRESS=$(( NUM_PROTONS / 20 ))
(( PROGRESS < 1 )) && PROGRESS=1

TOTAL=$(( ${#ENERGIES[@]} * ${#MODELS[@]} ))

echo "====================================================================="
echo " Energies      : ${ENERGIES[*]}"
echo " Models        : ${MODELS[*]}"
echo " Scattering    : $PHYS_OPT"
echo " Protons/run   : $NUM_PROTONS"
echo " Total runs    : $TOTAL"
echo " Output dir    : $OUTDIR"
echo " Concurrency   : $JOBS"
[[ "$DRY_RUN" == "1" ]] && echo " MODE          : DRY RUN (nothing will be executed)"
echo "====================================================================="

# ----------------------------------------------------------------------------
# One run
# ----------------------------------------------------------------------------
run_one() {
    local energy="$1" model="$2" idx="$3"
    local tag="E${energy}MeV_${PHYS_OPT}_${model}"
    local macro="$MACRODIR/${tag}.mac"
    local log="$LOGDIR/${tag}.log"
    local status_file="$STATDIR/${tag}.csv"

    # Resume: the ntuple file Geant4 actually produced is the completion marker.
    if [[ "$FORCE" != "1" ]] && compgen -G "$OUTDIR/${tag}_nt_*.csv" > /dev/null; then
        echo "[$idx/$TOTAL] SKIP  $tag (output present; FORCE=1 to redo)"
        echo "$energy,$model,$PHYS_OPT,$NUM_PROTONS,skipped,0,," > "$status_file"
        return 0
    fi

    if [[ "$DRY_RUN" == "1" ]]; then
        echo "[$idx/$TOTAL] WOULD RUN  $tag  ->  $OUTDIR/${tag}_nt_PKAs*.csv"
        return 0
    fi

    # Written fresh each time. /analysis/setFileName must come after
    # /run/initialize -- the messenger only exists once RunAction is built.
    cat > "$macro" <<EOF
/run/initialize
/analysis/setFileName ${tag}
/run/printProgress ${PROGRESS}
/gps/particle proton
/gps/pos/type Plane
/gps/pos/shape Square
/gps/pos/halfx 0.5 cm
/gps/pos/halfy 0.5 cm
/gps/pos/centre 0 0 -4 cm
/gps/direction 0 0 1
/gps/energy ${energy} MeV
/run/beamOn ${NUM_PROTONS}
EOF

    echo "[$idx/$TOTAL] START $tag"
    local t0 t1 elapsed rc
    t0=$(date +%s)
    "$EXE" "$macro" "$PHYS_OPT" "$model" > "$log" 2>&1
    rc=$?
    t1=$(date +%s)
    elapsed=$(( t1 - t0 ))

    if (( rc != 0 )); then
        echo "[$idx/$TOTAL] FAIL  $tag (exit $rc, ${elapsed}s) -- see $log" >&2
        echo "$energy,$model,$PHYS_OPT,$NUM_PROTONS,failed,$elapsed,,$rc" > "$status_file"
        return "$rc"
    fi

    # Geant4 writes <name>_nt_<ntuple>[_tN].csv into the CWD. Move them into
    # OUTDIR rather than relying on setFileName accepting a directory prefix,
    # which is version-dependent for the CSV backend.
    local moved=0 f
    for f in ${tag}_nt_*.csv; do
        [[ -e "$f" ]] || continue
        mv "$f" "$OUTDIR/" && moved=$(( moved + 1 ))
    done

    if (( moved == 0 )); then
        # Maybe setFileName did honour a path, or the run produced nothing.
        if compgen -G "$OUTDIR/${tag}_nt_*.csv" > /dev/null; then
            moved=$(ls -1 "$OUTDIR/${tag}"_nt_*.csv 2>/dev/null | wc -l)
        else
            echo "[$idx/$TOTAL] WARN  $tag produced no ntuple files" >&2
            echo "$energy,$model,$PHYS_OPT,$NUM_PROTONS,no-output,$elapsed,0,0" > "$status_file"
            return 0
        fi
    fi

    # Row count across this run's PKA files, minus the '#' comment/header lines.
    local rows
    rows=$(cat "$OUTDIR/${tag}"_nt_PKAs*.csv 2>/dev/null | grep -vc '^#' || echo 0)

    echo "[$idx/$TOTAL] DONE  $tag (${elapsed}s, ${rows} PKA rows)"
    echo "$energy,$model,$PHYS_OPT,$NUM_PROTONS,ok,$elapsed,$rows,0" > "$status_file"
    return 0
}

# ----------------------------------------------------------------------------
# Sweep
# ----------------------------------------------------------------------------
SWEEP_START=$(date +%s)
idx=0
first_run_done=0

for energy in "${ENERGIES[@]}"; do
    for model in "${MODELS[@]}"; do
        idx=$(( idx + 1 ))

        if (( JOBS > 1 )) && (( first_run_done == 1 )); then
            # Throttle to JOBS concurrent children.
            while (( $(jobs -rp | wc -l) >= JOBS )); do
                wait -n 2>/dev/null || sleep 1
            done
            run_one "$energy" "$model" "$idx" &
        else
            # The very first real run is executed in the foreground: if the
            # setup is broken (bad physics list, missing data) it fails here
            # and we abort, instead of launching 31 more doomed runs.
            run_one "$energy" "$model" "$idx"
            rc=$?
            if (( rc != 0 )) && (( first_run_done == 0 )) && [[ "$DRY_RUN" != "1" ]]; then
                echo >&2
                echo "ERROR: the first run failed (exit $rc). Aborting the sweep --" >&2
                echo "       this usually means a configuration problem, not a" >&2
                echo "       one-off. Fix it, then re-run; completed runs are skipped." >&2
                exit "$rc"
            fi
            first_run_done=1
        fi
    done
done

wait   # let any backgrounded runs finish

# ----------------------------------------------------------------------------
# Manifest + summary
# ----------------------------------------------------------------------------
MANIFEST="$OUTDIR/manifest.csv"
echo "energy_MeV,model,scattering,protons,status,seconds,pka_rows,exit_code" > "$MANIFEST"
# Numeric sort on energy so the table reads 10,30,60,... not 10,100,1000,185.
cat "$STATDIR"/*.csv 2>/dev/null | sort -t, -k1,1n -k2,2 >> "$MANIFEST"

SWEEP_END=$(date +%s)
TOTAL_S=$(( SWEEP_END - SWEEP_START ))

if [[ "$DRY_RUN" == "1" ]]; then
    echo "====================================================================="
    echo "Dry run complete -- $TOTAL runs would be executed."
    exit 0
fi

# grep -c already prints 0 when there is no match (and exits 1), so an
# `|| echo 0` fallback would emit a second line and break the arithmetic.
count_status() {
    local n
    n=$(grep -c ",$1," "$MANIFEST" 2>/dev/null)
    echo "${n:-0}"
}
n_ok=$(count_status ok)
n_skip=$(count_status skipped)
n_fail=$(count_status failed)
n_none=$(count_status no-output)

echo "====================================================================="
printf 'Sweep finished in %02d:%02d:%02d\n' \
    $(( TOTAL_S/3600 )) $(( (TOTAL_S%3600)/60 )) $(( TOTAL_S%60 ))
echo "  ok=$n_ok  skipped=$n_skip  failed=$n_fail  no-output=$n_none"
echo "  manifest: $MANIFEST"
echo "====================================================================="

if (( n_fail > 0 )); then
    echo "Failed runs:" >&2
    grep ',failed,' "$MANIFEST" | cut -d, -f1,2 | sed 's/^/  /' >&2
    exit 1
fi
