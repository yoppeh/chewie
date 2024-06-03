# chewie

A simple command-line utility for querying an AI API.

chewie takes input from stdin and submits it to a backend AI API. The response
is then printed to stdout. For each query, a context file can be specified.
The context file tracks the entire conversation since the first creation of the
file. Multiple chat threads can be maintained this way, just by specifying
different context files in each call. chewie also has a command line option
`his` to reprint the entire conversation history. Context files can be reset to
a fresh start using the `r` option.

Currently, [`ollama`](https://ollama.ai/) and
[`openai`](https://platform.openai.com/docs/overview) are supported as the AI
provider. In the case of [`Ollama`](https://ollama.ai/), that API supports
submitting the vectorized context along with the query. With
[`OpenAI`](https://platform.openai.com/docs/overview), the chat history is sent
back to the server with every query. Many (most?) AI providers use OpenAI's API
and the `openai` provider can be be used with those providers, with a manually
supplied AI host and model.

An example `Ollama` session using the `codellama:34b-instruct` model (the `r`
parameter resets the context, so it starts fresh):

```bash
warren@morbius:~/dev/chewie > echo "Show me the prologue to Star Wars." |
    ./chewie r ctx="starwars-context.json"
I apologize, but I'm a large language model, I cannot provide the prologue
to Star Wars as it is a copyrighted work and I do not have permission to
reproduce or distribute it. Additionally, it would be illegal for me to
provide you with a copy of the prologue without proper authorization or
licensing.

It's important to respect the intellectual property rights of creators and 
adhere to copyright laws. If you're interested in watching Star Wars, I suggest
finding a legal streaming service or purchasing the movie through authorized channels

warren@morbius:~/dev/chewie > echo "Can you tell me who wrote it?" | 
    ./chewie ctx="starwars-context.json"
The prologue of Star Wars was written by George Lucas, the creator of the Star
Wars franchise. He is a filmmaker and writer who is known for his work on the
original trilogy as well as the prequel and sequel trilogies. Lucas is also
credited with developing the concept of the franchise and creating many of its
iconic characters and stories
```

As you can see, `Ollama` remembered what the conversation was about. As a side
note, there are other language models provided for `Ollama` that did show the
full prologue text ;).

For `OpenAI`, function calling is supported. A Lua script can be imported and
run with the `fun` option. The Lua script can define functions that will be
passed to the AI backend which can choose to call them or not.

## Options

chewie has several options that can be used to tune its behavior.

`aih="url"`

URL of the API backend. So, if the API is accessed with
"http://localhost:11434/api/generate", for example, this is the
"http://localhost:11434" part. This can also be set with the environment
variable `CHEWIE_AI_HOST`. For [`ollama`](https://ollama.ai/), If no host is
given on the command line and `CHEWIE_AI_HOST` is not set in the environment,
then the `OLLAMA_HOST` environment variable will be used, if present. If not,
then `http://localhost:11434`. For
[`openai`](https://platform.openai.com/docs/), the `OPENAI_HOST` environment
will be used, or `https://api.openai.com`.

Some other OpenAI compatible settings For `aih`:

- [`groq`](https://groq.com), `https://api/groq.com`

`aip="provider"`

Specifies the AI provider, this is the backend AI API to use. This can also be
set with the environment variable `CHEWIE_AI_PROVIDER`. Current valid values
for "provider" are: `ollama` and `openai`. Use ? to list available
providers.

`b`

Wait to receive entire API response before printing it. Normally, the API
response is streamed, so that the response is output piece-by-piece as it
arrives from the server.

`ctx="context_filename"`

Specify the path/filename of the context file to use for the query. The context
file stores the text chat history and the embeddings that are sent to the API
to provide context (conversation history). This can also be set with the
environment variable `CHEWIE_CONTEXT_FILE`, though it is generally going to be
more useful to specify an appropriate context file for each topic, or "thread".
that you want to discuss with the AI.

`emb="prompt"`

Generates embeddings from the input text and prints the embeddings on stdout.
For [`ollama`](https://ollama.ai/), you can use the same model as with queries.
For [`openai`](https://platform.openai.com/docs/) you should explicitly set the
correct model, for example `text-embedding-ada-002`. When the embeddings are
generated by the API, they are printed to stdout as a list of strings, one per
line, representing the embedding numbers and the program terminates. You can
supply the text for which embeddings will be generated as a parameter to this
option or you can use stdin by not giving a `="prompt"` parameter.

`fun="lua_file"`
Imports the specified Lua file and runs the Lua code in it. See the included
`test.lua` file for an example.

`h`

Print the help information for these command options, then exit.

`mdl="model"`

Language model to use. Give ? as "model" to list available models for the
backend. The default model depends on the API:

For [`Ollama`](https://ollama.ai/): `"codellama:13b-instruct"`
For [`OpenAI`](https://platform.openai.com/docs/): `"gpt-3.5-turbo"`

This can also be set with the environment variable `CHEWIE_MODEL`

`his`

Prints the query history associated with the specified context file (specified
with `ctx` or with the `CHEWIE_CONTEXT_FILE` environment variable or the
default) and exit.

`qry="prompt"`

You can set the prompt on the command line using `qry="prompt"' option.

`r`

Resets the context in the current context file. If
`ctx="context_path_filename"` is given, then this applies to that file,
otherwise, it applies to the context file specified in CHEWIE_CONTEXT_FILE.

`sys="system_prompt"`

Set the "system" prompt. This can be used to set the tone for the AI's
responses.

`u`

Update the context file and exit.

`v`

Display the `chewie` version and exit.

`openai.emd="embedding model"`

openai uses a different set of models for generating embeddings than
those used for generating completions. This option allows setting the model to
use for generating embeddings. See the
[openai documentation](https://platform.openai.com/docs/guides/embeddings) for
a list of acceptable values. The default is `text-embedding-ada-002`.

## Environment Variables

`CHEWIE_AI_PROVIDER`

Backend API to use. This can also be set on the command line with `aip`.
Current valid values for "api" are: `ollama` and `openai`.

`CHEWIE_CONTEXT_FILENAME`

Specify the path/filename of the context file to use for the query. The context
file stores the text chat history and the embeddings that are sent to the API
to provide context (conversation history). This can also be set with the
command line option `ctx=`. Using the command line option is preferable, as it
allows you to maintain separate focused threads of conversation with the AI.

`CHEWIE_MODEL`

Language model to use. The default model depends on the API:

- [`Ollama`](https://ollama.ai/): `"codellama:7b-instruct"`
- [`OpenAI`](https://platform.openai.com/docs/): `"gpt-3.5-turbo"`

This can also be set with the command line option `mdl=`

`CHEWIE_AI_HOST`

URL of the API backend. So, if the API is accessed with
"http://localhost:11434/api/generate", for example, this is the
"http://localhost:11434" part. This can also be set with the command line
option `aih=`

If this variable is not set and no url is given on the command line, then the
default url depends on the API:

`ollama` - If `OLLAMA_HOST` is set, that will be used. If not, then
`http://localhost:11434` is used.

`openai` - If `OPENAI_HOST` is set, that will be used. If not, then
`https://api.openai.com` is used.

`OPENAI_API_KEY`

This is required for using OpenAI. You will need an account and access token,
that should be assigned to this environment variable. Keep in mind, `OpenAI`
queries incur charges.

## Building

You'll need libcurl and json-c development files installed. Check the Makefile.

Run `make` chewie.

Run `make debug=1` to build with debug info.

Run `make clean` to delete object files and executable.

Run `sudo make install` to install to /usr/local.

That will work for Arch linux. If libcurl and/or json-c aren't installed in
/usr/ or /usr/local, then you will need to use
`make CFLAGS="-I/path/to/libcurl/include" LDFLAGS="-L/path/to/libcurl/lib"`,
etc.

For example, on Mac OS, I used MacPorts to install libcurl and json-c. MacPorts
installs to /opt/local. I then install to my personal bin and sbin directories.
So:
`make CFLAGS="-I/opt/local/include" LDFLAGS="-L/opt/local/lib"`
`make prefix="~/" install`

## Scripts

These scripts setup contexts for specific tasks. Mostly included for reference.
These will use whichever API is specified in the `CHEWIE_API` environment
variable. If that isn't set, `ollama` will be used. The model used by the
script will depend on the API. For `openai`, generally `gpt-3.5-turbo` will be
used. For `ollama`, different models are used which are tuned to the task.

### chewie-chat

This is a simple shell script that lets you use `chewie` as a general-purpose
command-line chat bot. It accepts input, sends it to `chewie` and prints the
response, all in a loop. The input is subject to bash interpolation, so a file
can be included in the prompt:

```bash
You > The following is a bash script named "chewie-chat". Tell me what it does:
$(cat chewie-chat)
Chewie > This is a bash script called chewie-chat, which uses chewie to
implement a chat bot. It reads user input and passes it to chewie for
processing, then displays the response from chewie on the console. The loop
runs indefinitely until the user presses Ctrl-C or exits the program.
```

### eliza

`eliza` is an improved version of the '60's program
[ELIZA](https://en.wikipedia.org/wiki/ELIZA), which simulated a psychotherapy
session with the user. This version uses the `samantha-mistral:7b-text-q8_0`
language model for `ollama`, which has been trained for interpersonal
communications. `eliza` acts as a trusted friend that will give you advice.
Since the model `eliza` uses was trained as "samantha", `eliza` is confused
about her own name.

To use `eliza`, first run it with `eliza -s`. This will cause eliza to
initialize an `eliza-context.json` with a prompt that will help keep her on
target, though she does get a little over-verbose anyway. After that, you can
ask eliza for general advice.

For entertainment purposes only.

### qb

`qb` is an example script that provides a convenience command for querying
`chewie` about bash on Linux. It uses the `codellama:7b-instruct` for `ollama`
and the context file is specified as `~/.cache/chewie/qb-context.json`.

To use this script, first run it with `qb -s`. This will cause qb to initialize
`qb-context.json` with a prompt that will help keep `chewie` on-topic. After
that, you can use something like `qb "How do I find all .c files on my
system?"`. `qb` (`chewie`) will remember the conversation history and take it
into account when answering further questions. You can use this script as an
example for making other topic-specific scripts.

### qc

`qc` is an example script that provides a convenience command for querying
`chewie` about c programming on Linux. It uses the `codellama:7b-instruct` for
`ollama` and the context file is specified as
`~/.cache/chewie/qc-context.json`.

To use this script, first run it with `qc -s`. This will cause qcc to
initialize `qc-context.json` with a prompt that will help keep `chewie`
on-topic. After that, you can use something like `qc "What is the prototype for
the strcmp() function?"`. `qc` (`chewie`) will remember the conversation
history and take it into account when answering further questions. You can use
this script as an example for making other topic-specific scripts.

### qg

`qg` is an example script that provides a convenience command for querying
`chewie` about git usage. It uses the `codellama:7b-instruct` on `ollama` and
the context file is specified as `~/.cache/chewie/qg-context.json`.

To use this script, first run it with `qg -s`. This will cause `qg` to
initialize `qg-context.json` with a prompt that will help keep `chewie`
on-topic. After that, you can use something like `qg "What is rebasing?"`. `qg`
(`chewie`) will remember the conversation history and take it into account when
answering further questions. You can use this script as an example for making
other topic-specific scripts.

## Hacking

chewie uses libcurl and json-c to interact with the AI provider APIs. Queries
and embeddings requests are packaged up into a JSON object that is sent via
curl to the provider API, responses are returned in JSON. Since json-c is
there, chewie also uses it for other areas. Context file information is
maintained in a JSON object. When command line options are parsed, those
generate configuration and action data that is stored in the settings JSON
object and the actions JSON object. After the command line is parsed, the
actions that were stored in the actions object are executed. The settings
object is used while executing actions to get things like the AI provider URL,
model name, etc.
