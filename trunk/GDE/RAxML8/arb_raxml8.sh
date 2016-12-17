#!/bin/bash
set -e

BASES_PER_THREAD=300
SELF=`basename "$0"`

# set up environment
if [ -z $LD_LIBRARY_PATH ]; then
    LD_LIBRARY_PATH="$ARBHOME/lib"
else
    LD_LIBRARY_PATH="$ARBHOME/lib:$LD_LIBRARY_PATH"
fi
export LD_LIBRARY_PATH

# always wait on exit
# called at exit of script (by trap) and on error
# e.g. if ctrl-c is pressed
wait_and_exit() {
    # do not recurse
    trap EXIT

    # kill any backgrounded processes
    local JOBS=`jobs -r`
    if [ -n "${JOBS}" ]; then
        read -p "Press ENTER to terminate child processes and close window"
        local JOBPIDS=`jobs -p`
        kill ${JOBPIDS}
    else
        read -p "Press ENTER to close window"
    fi
    exit
}

on_exit() {
    wait_and_exit
}
trap on_exit EXIT

# return true if argument is file in path executable by user
can_run() {
    which "$1" &>/dev/null && test -x `which "$1" &>/dev/null`
}

# show error in ARB and exit
report_error() {
    echo "ARB ERROR: $*"
    arb_message "$SELF failed with: $*"
    wait_and_exit
}

# create named temporary directory
prepare_tmp_dir() {
    ARB_TMP="$HOME/.arb_tmp"
    NAME="$1"
    DATE=`date +%Y-%m-%d--%H-%M-%S`
    for N in "" ".2" ".3" ".4"; do
        DIR="$ARB_TMP/$NAME--$DATE$N"
        if [ -e "$DIR" ]; then
            continue;
        else
            mkdir -p "$DIR"
            echo "$DIR"
            return
        fi
    done
    report_error "Unable to create temporary directory"
}

dump_features() {
    case `uname` in
        Darwin)
            sysctl machdep.cpu.features
            ;;
        Linux)
            grep -m1 flags /proc/cpuinfo 2>/dev/null
            ;;
    esac
}
cpu_has_feature() {
    dump_features | grep -qi "$1" &>/dev/null
}

cpu_get_cores() {
    # honor Torque/PBS num processes (or make sure we follow, if enforced)
    if [ ! -z $PBS_NP ]; then
        echo $PBS_NP
        return
    fi
    # extract physical CPUs from host
    case `uname` in
        Darwin)
            sysctl -n hw.ncpu
            ;;
        Linux)
            grep -c "^processor" /proc/cpuinfo
            ;;
    esac
}

extract_line_suffix() {
    local LOG=$1
    local PREFIX=$2
    grep -P "^${PREFIX}\s*" $LOG | sed "s/${PREFIX}\s*//"
}

extract_likelihood() {
    local LOG=$1
    local PREFIX=$2
    local SUFFIX=`extract_line_suffix $LOG $PREFIX`
    if [ -z "$SUFFIX" ]; then
        local FAILED_DETECTION="failed to detect likelyhood"
        echo $FAILED_DETECTION
        arb_message "$SELF warning: $FAILED_DETECTION"
    else
        echo "$SUFFIX"
    fi
}

TREEFILE=arb_export.tree

export_input_tree() {
    # expect user selected an 'Input tree' in arb and export it to $TREEFILE
    if [ -z "$INPUTTREE" ]; then
        report_error "you have to select an 'Input tree'"
    fi

    arb_export_tree $INPUTTREE > $TREEFILE
}

# --------------------------
#      protocols helpers

bootstrap_and_consenseIfReq() {
    # run $BOOTSTRAP BS searches
    $RAXML -b "$SEED" -m $MODEL -p "$SEED" -s "$SEQFILE" \
        -N "$BOOTSTRAPS" \
        -n BOOTSTRAP

    if [ -n "$MRE" ]; then
        # compute extended majority rule consensus tree
        $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.BOOTSTRAP -n BOOTSTRAP_CONSENSUS
    fi
}

bootstrapAsyncIfRequested_and_wait() {
    if [ "$BOOTSTRAPS" != "no" ]; then
        if [ "$TRY_ASYNC" = "1" ]; then
            if [ $(($THREADS * 2)) -gt $CORES ]; then
            # wait for raxml (started by caller) to complete,
            # if we have not enough cores to run bootstrap search at the same time
                sleep 4 # just cosmetic (raxml output goes 1st)
                echo "Note: Not enough cores found ($CORES) to run ML search and"
                echo "      BS in parallel with $THREADS threads. Waiting..."
                wait
            fi
        else
            # otherwise always sync silently
            wait
        fi
        bootstrap_and_consenseIfReq &
    fi
    wait # for all jobs
}

import_trees() {
    local TPREFIX=$1
    local RUN=$2
    local COMMENT="$3"

    local MAINTREE=${TPREFIX}.${RUN}
    # imports tree MAINTREE
    # - with support values (if bootstrapping requested)
    # - else "as is"
    # imports MRE tree (if requested)

    if [ "$BOOTSTRAPS" != "no" ]; then
        # draw bipartition information
        $RAXML -f b -m $MODEL \
          -t ${TPREFIX}.${RUN} \
          -z RAxML_bootstrap.BOOTSTRAP \
          -n ${RUN}_WITH_SUPPORT

        MAINTREE=RAxML_bipartitions.${RUN}_WITH_SUPPORT
        COMMENT="${COMMENT} BOOTSTRAPS=${BOOTSTRAPS}"
    fi

    arb_read_tree ${TREENAME} ${MAINTREE} "${COMMENT}"

    if [ -n "$MRE" ]; then
        if [ "$BOOTSTRAPS" != "no" ]; then
            # otherwise no MRE tree possible
            arb_read_tree ${TREENAME}_mre RAxML_MajorityRuleExtendedConsensusTree.BOOTSTRAP_CONSENSUS \
              "PRG=RAxML8 MRE consensus tree of $BOOTSTRAPS bootstrap searches performed for species in ${TREENAME}"
        fi
    fi
}

# -------------------
#      protocols

dna_tree_thorough() {
    # do $REPEATS searches for best ML tree
    $RAXML -f d -m $MODEL -p "$SEED" -s "$SEQFILE"  \
        -N "$REPEATS" \
        -n TREE_INFERENCE &

    bootstrapAsyncIfRequested_and_wait

    LIKELIHOOD=`extract_likelihood RAxML_info.TREE_INFERENCE 'Final\s*GAMMA-based\s*Score\s*of\s*best\s*tree'`
    import_trees RAxML_bestTree TREE_INFERENCE "PRG=RAxML8 FILTER=$FILTER DIST=$MODEL LIKELIHOOD=${LIKELIHOOD} PROTOCOL=thorough"
}

dna_tree_quick() {
    if [ "$BOOTSTRAPS" = "no" ]; then
        report_error You have to select the number of bootstraps to perform
    fi

    # run fast bootstraps
    $RAXML -f a -m $MODEL -p "$SEED" -x "$SEED" -s "$SEQFILE" \
      -N "$BOOTSTRAPS" \
      -n FAST_BS

    # import
    LIKELIHOOD=`extract_likelihood RAxML_info.FAST_BS 'Final\s*ML\s*Optimization\s*Likelihood:'`
    arb_read_tree ${TREENAME} RAxML_bipartitions.FAST_BS "PRG=RAxML8 FILTER=$FILTER DIST=$MODEL LIKELIHOOD=${LIKELIHOOD} PROTOCOL=quick"

    # create consensus tree
    if [ -n "$MRE" ]; then
        $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.FAST_BS -n FAST_BS_MAJORITY
        # import
        arb_read_tree ${TREENAME}_mre RAxML_MajorityRuleExtendedConsensusTree.FAST_BS_MAJORITY \
          "PRG=RAxML8 MRE consensus tree of $BOOTSTRAPS rapid-bootstraps performed while calculating ${TREENAME}"
    fi
}   

dna_tree_add() {
    export_input_tree

    $RAXML -f d -m $MODEL -p "$SEED" -s "$SEQFILE" \
      -g $TREEFILE \
      -n ADD &

    bootstrapAsyncIfRequested_and_wait

    LIKELIHOOD=`extract_likelihood RAxML_info.ADD 'Final\s*GAMMA-based\s*Score\s*of\s*best\s*tree'`
    import_trees RAxML_bestTree ADD "PRG=RAxML8 FILTER=$FILTER DIST=$MODEL LIKELIHOOD=${LIKELIHOOD} PROTOCOL=add INPUTTREE=$INPUTTREE"
}

dna_tree_optimize() {
    export_input_tree

    $RAXML -f t -m $MODEL -p "$SEED" -s "$SEQFILE" \
      -N "$REPEATS" \
      -t $TREEFILE \
      -n OPTIMIZE &

    bootstrapAsyncIfRequested_and_wait

    LIKELIHOOD=`extract_likelihood RAxML_info.OPTIMIZE 'Final\s*GAMMA-based\s*Score\s*of\s*best\s*tree'`
    import_trees RAxML_bestTree OPTIMIZE "PRG=RAxML8 FILTER=$FILTER DIST=$MODEL LIKELIHOOD=${LIKELIHOOD} PROTOCOL=optimize INPUTTREE=$INPUTTREE"
}

dna_tree_calcblen() {
    export_input_tree

    $RAXML -f e -m $MODEL -s "$SEQFILE" \
      -t $TREEFILE \
      -n CALCBLEN &

    bootstrapAsyncIfRequested_and_wait

    LIKELIHOOD=`extract_likelihood RAxML_info.CALCBLEN 'Final\s*GAMMA\s*likelihood:'`
    import_trees RAxML_result CALCBLEN "PRG=RAxML8 FILTER=$FILTER DIST=$MODEL LIKELIHOOD=${LIKELIHOOD} PROTOCOL=calcblen INPUTTREE=$INPUTTREE"
}

dna_tree_bootstrap() {
    if [ "$BOOTSTRAPS" = "no" ]; then
        report_error You have to select the number of bootstraps to perform
    fi

    export_input_tree
    bootstrapAsyncIfRequested_and_wait
    import_trees arb_export tree "PRG=RAxML8 FILTER=$FILTER DIST=$MODEL PROTOCOL=bootstrap INPUTTREE=$INPUTTREE"
}

dna_tree_score() {
    export_input_tree

    $RAXML -f n -m $MODEL -s "$SEQFILE" \
      -z $TREEFILE \
      -n SCORE

    RESULT=`extract_likelihood RAxML_info.SCORE 'Tree\s*0\s*Likelihood'`
    # RESULT contains sth like: -27819.642837 Tree-Length 6.899222
    LIKELIHOOD=${RESULT/ Tree-Length */}
    TREELEN=${RESULT/* Tree-Length /}

    arb_write_tree_comment $INPUTTREE "RAxML8-score: FILTER=$FILTER DIST=$MODEL LIKELIHOOD=$LIKELIHOOD TREELEN=$TREELEN"
}

# -------------- 
#      main     

MRE=Y
TREENAME=raxml
FILTER=unknown
INPUTTREE=

while [ -n "$1" ]; do 
  case "$1" in
      -p) # protocol
          PROTOCOL="$2"
          shift
          ;;
      -m) # subst model
          MODEL="$2"
          shift
          ;;
      -s) # random seed
          SEED="$2"
          shift
          ;;
      -b) # bootstraps
          BOOTSTRAPS="$2"
          shift
          ;;
      -r) # number of tree searches
          REPEATS="$2"
          shift
          ;;
      -n) # tree name
          TREENAME="$2"
          shift
          ;;
      -nomre) # don't create mre tree
          MRE=
          ;;
      -nt) # seqtype dna
          SEQTYPE=N
          ;;
      -aa) # seqtype proto
          SEQTYPE=A
          ;;
      -f) # input file
          FILE="$2"
          shift
          ;;
      -t) # threads
          THREADS="$2"
          shift
          ;;
      -it) # inputtree
          INPUTTREE="$2"
          if [ "$INPUTTREE" = 'tree_?????' ]; then # has to match ../../TEMPLATES/arb_global_defs.h@NO_TREE_SELECTED
              INPUTTREE=
          fi
          shift
          ;;
      -fi) # filtername for comment
          FILTER="$2"
          shift
          ;;
      *)
          report_error argument not understood: "$1"
          ;;
  esac
  shift
done

# correct output treename (ensure prefix 'tree_', avoid things like 'tree_tree' etc)
TREENAME=${TREENAME##tree}
TREENAME=${TREENAME#_}
TREENAME=${TREENAME#_}
TREENAME=tree_${TREENAME}

# use time as random SEED if empty
if [ -z "$SEED" ]; then
    # seconds since 1970
    SEED=`date +%s`
fi

# prepare data in tempdir
DIR="`prepare_tmp_dir raxml`"
SEQFILE="dna.phy"
FULLSEQFILE="${DIR}/${SEQFILE}"

arb_convert_aln --arb-notify -GenBank "$FILE" -phylip "${FULLSEQFILE}" 2>&1 |\
  grep -v "^WARNING(14): LOCUS" || true # remove spurious warning
rm $FILE

cd "$DIR"

# decide whether async execution of BS and main algo makes sense (ie. runtimes of both are expected similar)
TRY_ASYNC=0
case "${PROTOCOL}" in
    (thorough|optimize)
        if [ "$BOOTSTRAPS" != "no" ]; then
            TRY_ASYNC=1
        fi
        ;;
esac

# select RAxML binary
if cpu_has_feature avx && can_run raxmlHPC8-AVX.PTHREADS; then
    RAXML=raxmlHPC8-AVX.PTHREADS
elif cpu_has_feature sse3 && can_run raxmlHPC8-SSE3.PTHREADS; then
    RAXML=raxmlHPC8-SSE3.PTHREADS
elif can_run raxmlHPC8-PTHREADS; then
    RAXML=raxmlHPC8-PTHREADS
else
    report_error Could not find working RAxML binary. 
fi

# get some numbers
CORES=`cpu_get_cores`
read NSEQS BP_ARB < <(head -n 1 $SEQFILE)

# retrieve number of alignment patterns recognized by RAxML
$RAXML -T 2 -f j -s "$SEQFILE" -b 123 -N 1 -m "$MODEL" -n PATTERNS
BP=`extract_line_suffix RAxML_info.PATTERNS "Alignment Patterns:"`

# warn if model is not recommended for given number of sequences
BAD_PRACTICE="This is not considered good practice.\nPlease refer to the RAxML manual for details."
if [ "$MODEL" == "GTRGAMMA" -a $NSEQS -gt 10000 ]; then
    arb_message "Using the GTRGAMMA model on more than 10,000 sequences.\n$BAD_PRACTICE"
fi
if [ "$MODEL" == "GTRCAT" -a $NSEQS -lt 150 ]; then
    arb_message "Using the GTRCAT model on less than 150 sequences.\n$BAD_PRACTICE"
fi

# ----------------------------------- 
#      threads / cores detection     

CORES=$(( $CORES + 1 - 1 ))
# calculate number of threads (if not passed)
MAX_THREADS=$(( ( $BP - 1 ) / $BASES_PER_THREAD + 2))
# +1 is for master thread,
# another +1 for the first $BASES_PER_THREAD (bash truncates); -1 to avoid extra thread if BP is divisible by BASES_PER_THREAD

if [ $CORES -lt 1 ]; then
    # failed to detect CORES
    SETENVAR_HINT="set the environment variable PBS_NP to the number of cores available (before you start arb)"
    if [ -z "$THREADS" ]; then
        report_error "failed to detect number of cores.\nPlease specify 'CPU thread override' or\n${SETENVAR_HINT}."
    else
        CORES=$THREADS
        if [ "$TRY_ASYNC" = "1" ]; then
            echo "Warning: failed to detect number of cores (assuming ${CORES} from 'CPU thread override')"
            echo "Please ${SETENVAR_HINT}."
        else
            arb_message "Warning: failed to detect number of cores\n=> parallel bootstrapping disabled\nPlease ${SETENVAR_HINT} to avoid that."
        fi
    fi
else
    MAX_TH_NOTE="maximum useful thread-count for alignment with ${BP} bp would be ${MAX_THREADS}"
    if [ -z "$THREADS" ]; then
        if [ $CORES -lt $MAX_THREADS ]; then
            echo "Note: Limiting threads to $CORES available cores (${MAX_TH_NOTE})"
            THREADS=$CORES
        else
            THREADS=$MAX_THREADS
            if [ "$TRY_ASYNC" = "1" ]; then
                if [ $(($THREADS * 2)) -gt $CORES ]; then
                    # split threads between BS and ML search
                    if [ $((($THREADS-1) * 2)) -le $CORES ]; then
                        THREADS=$(($THREADS-1))
                    else
                        if [ $((($THREADS-2) * 2)) -le $CORES ]; then
                            THREADS=$(($THREADS-2))
                        fi
                    fi
                fi
                if [ $THREADS -lt $MAX_THREADS ]; then
                    arb_message "Note: reduced threads to $THREADS to allow parallel execution of bootstrapping\nset 'CPU thread override' to ${MAX_THREADS} to avoid that"
                fi
            fi
        fi
        if [ $THREADS -lt 2 ]; then
            # use at least 2 threads (required by PTHREADS version)
            THREADS=2
        fi
    else
        echo "Note: Threads forced to ${THREADS} (${MAX_TH_NOTE})"
    fi
fi

if [ $CORES -lt $THREADS ]; then
    arb_message "Performance-Warning: Using $THREADS threads on $CORES cores"
fi

RAXML="$RAXML -T $THREADS"

case "${SEQTYPE}.${PROTOCOL}" in
    N.thorough)
        time dna_tree_thorough
        ;;
    N.quick)
        time dna_tree_quick
        ;;
    N.add)
        time dna_tree_add
        ;;
    N.optimize)
        time dna_tree_optimize
        ;;
    N.calcblen)
        time dna_tree_calcblen
        ;;
    N.bootstrap)
        time dna_tree_bootstrap
        ;;
    N.score)
        time dna_tree_score
        ;;
    *)
        report_error Unknown protocol "${SEQTYPE}.${PROTOCOL}"
        ;;
esac

# @@@ FIXME: cleanup temp dir

exit
