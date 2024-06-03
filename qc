#!/bin/bash
#
# qc 
#
# script that uses chewie to implement a c-specific chat bot.
#

if [ "$CHEWIE_API" == "ollama" ]; then
QC_MODEL="codellama:7b-instruct"
elif [ "$CHEWIE_API" == "openai" ]; then
QC_MODEL="gpt-3.5-turbo"
elif [ "$CHEWIE_API" == "groq" ]; then
QC_MODEL="llama-8b-8192"
else
QC_MODEL="codellama:7b-instruct"
CHEWIE_API="ollama"
fi

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
    echo "Using API \"$CHEWIE_API\""d
    echo "Using context file \"$QC_CONTEXT\""
    chewie r u ctx="$QC_CONTEXT" mdl="$QC_MODEL" sys="You are an intelligent assistant helping me write computer programs in the C language on Linux. All of your responses should be in the context of using the C programming language on Linux. If you do not know the answer to a question, just say \"I don't know\"." > /dev/null
    echo "done!"
    exit 0
fi

printf "%q" "$@" | chewie ctx="$QC_CONTEXT" mdl="$QC_MODEL"
