/**
 * context.h
 *
 * Manage context file. The context file is used to maintain state between
 * calls to the API. The context file is a JSON file with the following
 * two standard fields:
 * 
 * "system-prompt"
 * This is the system prompt to use.
 * 
 * "history": 
 * This is an array of objects containing the query history. Each object in the
 * array has the following fields:
 * 
 *     "prompt" - The prompt for the query.
 *     "response" - The response to the query.
 *     "timestamp" - The timestamp of the query.
 * 
 * The context file may also contain other fields that are private to the API
 * being used. These fields should be stored in an object that is itself a
 * field in the top-level context object, named after the API:
 * "ollama", "openai", etc.
 * 
 * These API-specific fields are not touched by the context manager. Keeping 
 * these values up-to-date/synchronized with the API is the responsibility of
 * the API interface, using the "updates_obj" parameter of the 
 * context_update(). Values in "updates_obj" will overwrite what is in the
 * context file when context_update() runs. Values in the context file that
 * are not in "updates_obj" will be left alone.
 */

#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <stdint.h>

#include <json-c/json.h>

/// Default context filename.
extern const char context_fn_default[];
/// Default context directory.
extern const char context_dir_default[];

/// Add a query/response/timestamp to the context.
extern void context_add_history(const char *prompt, const char *response, const int64_t timestamp);
/// Load context from file into json object.
extern void context_load(const char *fn);
/// Create a new context.
extern void context_new(const char *fn);
/// Delete the system prompt from the context file.
extern int context_delete_system_prompt(void);
/// Dump the chat history from the context file.
extern int context_dump_history(void);
/// get an arbitrary object from a named field in the context.
extern json_object *context_get(const char *field);
/// get the AI host from the context.
extern const char *context_get_ai_host(void);
/// Gwt the AI provider from the context.
extern const char *context_get_ai_provider(void);
/// Get the model from the contest.
extern const char *context_get_model(void);
/// Get the system prompt from the context.
extern const char *context_get_system_prompt(void);
/// Read the chat history from the context file.
extern json_object *context_get_history(void);
/// Get the prompt from a given history entry.
extern const char *context_get_history_prompt(json_object *entry);
/// Get the response from a given history entry.
extern const char *context_get_history_response(json_object *entry);
/// Get the timestamp from a given history entry.
extern int64_t context_get_history_timestamp(json_object *entry);
/// Set an arbitrary field to a given json object.
extern int context_set(const char *field, json_object *obj);
/// Set the AI host in the context file.
extern int context_set_ai_host(const char *s);
/// Set the AI provider in the context file.
extern int context_set_ai_provider(const char *s);
/// Set the model in the context file.
extern int context_set_model(const char *s);
/// Set the system prompt in the context file.
extern int context_set_system_prompt(const char *s);
/// Update the context file with new history and embeddings.
extern void context_update(void);

#endif // _CONTEXT_H