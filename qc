#!/bin/bash
#
# qc 
#
# script that uses chewie to implement a c-specific chat bot.
#

# API selection: CHEWIE_API, then CHEWIE_AI_PROVIDER, then ollama. "groq" uses
# Groq's OpenAI-compatible endpoint with the key from GROQ_API_KEY.
CHEWIE_API="${CHEWIE_API:-${CHEWIE_AI_PROVIDER:-ollama}}"
case "$CHEWIE_API" in
openai)
    QC_MODEL="gpt-3.5-turbo"
    CHEWIE_AIP=(aip=openai)
    ;;
groq)
    QC_MODEL="llama3-8b-8192"
    CHEWIE_AIP=(aip=openai aih=https://api.groq.com/openai)
    export OPENAI_API_KEY="${GROQ_API_KEY:-$OPENAI_API_KEY}"
    ;;
*)
    CHEWIE_API="ollama"
    QC_MODEL="codellama:7b-instruct"
    CHEWIE_AIP=(aip=ollama)
    ;;
esac

QC_CONTEXT_PATH=~/.cache/chewie
QC_CONTEXT=$QC_CONTEXT_PATH/qc-context.json

if [ "$1" == "-h" ] ; then
    echo "Usage: qc [options]"
    echo "Options:"
    echo "  -h  Display this help message."
    echo "  -s  Setup (or reset) qc."
    exit 0
fi

if [ "$1" == "-s" ] ; then
    echo "Setting up chewie... "
    mkdir -p $QC_CONTEXT_PATH
    echo "Using model \"$QC_MODEL\""
    echo "Using API \"$CHEWIE_API\""
    echo "Using context file \"$QC_CONTEXT\""
    chewie "${CHEWIE_AIP[@]}" r u ctx="$QC_CONTEXT" > /dev/null
    chewie "${CHEWIE_AIP[@]}" u ctx="$QC_CONTEXT" mdl="$QC_MODEL" sys="You are an intelligent assistant helping me write computer programs in the C language on Linux. All of your responses should be in the context of using the C programming language on Linux. If you do not know the answer to a question, just say \"I don't know\"." > /dev/null
    echo "done!"
    exit 0
fi

echo "$*" | chewie "${CHEWIE_AIP[@]}" ctx="$QC_CONTEXT" mdl="$QC_MODEL"
