#!/bin/bash
set -e

BASES_PER_THREAD=300

trap on_exit EXIT

on_exit() {
    wait_and_exit
}

# return true if argument is file in path executable by user
can_run() {
    which "$1" && test -x `which "$1"`
}

wait_and_exit() {
    read -p "Press key to close window"
    exit
}

# show error in ARB and exit
report_error() {
    SELF=`basename "$0"`
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

cpu_has_feature() {
    case `uname` in
        Darwin)
            SHOW="sysctl machdep.cpu.features"
            ;;
        Linux)
            SHOW="grep flags /proc/cpuinfo"
            ;;
    esac
    $SHOW | grep -qi "$1"
}

cpu_get_cores() {
    case `uname` in
        Darwin)
            sysctl -n hw.ncpu
            ;;
        Linux)
            grep -c "^processor" /proc/cpuinfo
            ;;
    esac
}

dna_tree_thorough() {
    # try N ML searches
    $RAXML -p "$SEED" -s "$SEQFILE" -m $MODEL \
        -N "$NTREES" \
        -n TREE_INFERENCE &

    # wait for raxml to complete if we have not enough
    # cores to run bootstrap search at the same time
    if [ $(($THREADS * 2)) -lt $CORES ]; then
        wait
    fi

    # run bootstraps
    $RAXML -p "$SEED" -b "$SEED" -s "$SEQFILE" -m $MODEL \
        -N "$BOOTSTRAPS" \
        -n BOOTSTRAP &
    wait

    # draw bipartition information
    $RAXML -f b -m $MODEL \
        -t RAxML_bestTree.TREE_INFERENCE \
        -z RAxML_bootstrap.BOOTSTRAP \
        -n ML_TREE_WITH_SUPPORT
    # import
    arb_read_tree tree_raxml_ml_with_support RAxML_bipartitions.ML_TREE_WITH_SUPPORT

    # compute extended majority rule consensus
    $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.BOOTSTRAP -n BOOTSTRAP_CONSENSUS
   
    # import
    arb_read_tree tree_raxml_bs_consensus RAxML_MajorityRuleExtendedConsensusTree.BOOTSTRAP_CONSENSUS
}

dna_tree_quick() {
    # run fast bootstraps
    $RAXML -f a -m $MODEL -p "$SEED" -x "$SEED" -s "$SEQFILE" -N "$BOOTSTRAPS" -n FAST_BS
    # create consensus tree
    $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.FAST_BS -n FAST_BS_MAJORITY
    # import
    arb_read_tree tree_raxml_fast_bs_consensus RAxML_MajorityRuleExtendedConsensusTree.FAST_BS_MAJORITY
}   

calc_mem_size() {
    # FIXME
    echo
}

aa_tree() {
    # FIXME
    echo
    # -m PROTGAMMALG4X
}

while [ -n "$1" ]; do 
  case "$1" in
      -p) # protocol
          PROT="$2"
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
          NTREES="$2"
          shift
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
      *)
          report_error argument not understood: "$1"
          ;;
  esac
  shift
done


# prepare data in tempdir
DIR="`prepare_tmp_dir raxml`"
SEQFILE="$DIR/dna.phy"

arb_convert_aln --arb-notify -GenBank "$FILE" -phylip "$SEQFILE" 2>&1 |\
  grep -v "^WARNING(14): LOCUS" || true # remove spurious warning
rm $FILE

cd "$DIR"


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

# calculate number of threads (if not passed)
CORES=`cpu_get_cores`
if [ -z "$THREADS" ]; then
    BP=`head -n 1 $SEQFILE | cut -c 16-`
    THREADS=$(( $BP / $BASES_PER_THREAD + 2))
    # +1 is for master thread,
    # another +1 for the first $BASES_PER_THREAD (bash truncates)

    if [ $THREADS -gt $CORES ]; then
        THREADS=$CORES
    fi
fi
RAXML="$RAXML -T $THREADS"  


case "${SEQTYPE}.${PROT}" in
    N.quick)
        dna_tree_quick
        ;;
    N.thorough)
        dna_tree_thorough
        ;;
esac

# FIXME: cleanup temp dir

wait_and_exit