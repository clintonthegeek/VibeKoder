### GET /assistants

**Summary:** Returns a list of assistants.

**Description:**
None.

**Parameters:**
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.


---

### POST /assistants

**Summary:** Create an assistant with a model and instructions.

**Description:**
None.

**Parameters:**


---

### GET /assistants/{assistant_id}

**Summary:** Retrieves an assistant.

**Description:**
None.

**Parameters:**
* `assistant_id` (path) - The ID of the assistant to retrieve.

---

### POST /assistants/{assistant_id}

**Summary:** Modifies an assistant.

**Description:**
None.

**Parameters:**
* `assistant_id` (path) - The ID of the assistant to modify.

---

### DELETE /assistants/{assistant_id}

**Summary:** Delete an assistant.

**Description:**
None.

**Parameters:**
* `assistant_id` (path) - The ID of the assistant to delete.

---

### POST /audio/speech

**Summary:** Generates audio from the input text.

**Description:**
None.

**Parameters:**


---

### POST /audio/transcriptions

**Summary:** Transcribes audio into the input language.

**Description:**
None.

**Parameters:**


---

### POST /audio/translations

**Summary:** Translates audio into English.

**Description:**
None.

**Parameters:**


---

### POST /batches

**Summary:** Creates and executes a batch from an uploaded file of requests

**Description:**
None.

**Parameters:**


---

### GET /batches

**Summary:** List your organization's batches.

**Description:**
None.

**Parameters:**
* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.


---

### GET /batches/{batch_id}

**Summary:** Retrieves a batch.

**Description:**
None.

**Parameters:**
* `batch_id` (path) - The ID of the batch to retrieve.

---

### POST /batches/{batch_id}/cancel

**Summary:** Cancels an in-progress batch. The batch will be in status `cancelling` for up to 10 minutes, before changing to `cancelled`, where it will have partial results (if any) available in the output file.

**Description:**
None.

**Parameters:**
* `batch_id` (path) - The ID of the batch to cancel.

---

### GET /chat/completions

**Summary:** List stored Chat Completions. Only Chat Completions that have been stored
with the `store` parameter set to `true` will be returned.


**Description:**
None.

**Parameters:**
* `model` (query) - The model used to generate the Chat Completions.
* `metadata` (query) - A list of metadata keys to filter the Chat Completions by. Example:

`metadata[key1]=value1&metadata[key2]=value2`

* `after` (query) - Identifier for the last chat completion from the previous pagination request.
* `limit` (query) - Number of Chat Completions to retrieve.
* `order` (query) - Sort order for Chat Completions by timestamp. Use `asc` for ascending order or `desc` for descending order. Defaults to `asc`.

---

### POST /chat/completions

**Summary:** **Starting a new project?** We recommend trying [Responses](/docs/api-reference/responses) 
to take advantage of the latest OpenAI platform features. Compare
[Chat Completions with Responses](/docs/guides/responses-vs-chat-completions?api-mode=responses).

---

Creates a model response for the given chat conversation. Learn more in the
[text generation](/docs/guides/text-generation), [vision](/docs/guides/vision),
and [audio](/docs/guides/audio) guides.

Parameter support can differ depending on the model used to generate the
response, particularly for newer reasoning models. Parameters that are only
supported for reasoning models are noted below. For the current state of 
unsupported parameters in reasoning models, 
[refer to the reasoning guide](/docs/guides/reasoning).


**Description:**
None.

**Parameters:**


---

### GET /chat/completions/{completion_id}

**Summary:** Get a stored chat completion. Only Chat Completions that have been created
with the `store` parameter set to `true` will be returned.


**Description:**
None.

**Parameters:**
* `completion_id` (path) - The ID of the chat completion to retrieve.

---

### POST /chat/completions/{completion_id}

**Summary:** Modify a stored chat completion. Only Chat Completions that have been
created with the `store` parameter set to `true` can be modified. Currently,
the only supported modification is to update the `metadata` field.


**Description:**
None.

**Parameters:**
* `completion_id` (path) - The ID of the chat completion to update.

---

### DELETE /chat/completions/{completion_id}

**Summary:** Delete a stored chat completion. Only Chat Completions that have been
created with the `store` parameter set to `true` can be deleted.


**Description:**
None.

**Parameters:**
* `completion_id` (path) - The ID of the chat completion to delete.

---

### GET /chat/completions/{completion_id}/messages

**Summary:** Get the messages in a stored chat completion. Only Chat Completions that
have been created with the `store` parameter set to `true` will be
returned.


**Description:**
None.

**Parameters:**
* `completion_id` (path) - The ID of the chat completion to retrieve messages from.
* `after` (query) - Identifier for the last message from the previous pagination request.
* `limit` (query) - Number of messages to retrieve.
* `order` (query) - Sort order for messages by timestamp. Use `asc` for ascending order or `desc` for descending order. Defaults to `asc`.

---

### POST /completions

**Summary:** Creates a completion for the provided prompt and parameters.

**Description:**
None.

**Parameters:**


---

### POST /embeddings

**Summary:** Creates an embedding vector representing the input text.

**Description:**
None.

**Parameters:**


---

### GET /evals

**Summary:** List evaluations for a project.


**Description:**
None.

**Parameters:**
* `after` (query) - Identifier for the last eval from the previous pagination request.
* `limit` (query) - Number of evals to retrieve.
* `order` (query) - Sort order for evals by timestamp. Use `asc` for ascending order or `desc` for descending order.
* `order_by` (query) - Evals can be ordered by creation time or last updated time. Use
`created_at` for creation time or `updated_at` for last updated time.


---

### POST /evals

**Summary:** Create the structure of an evaluation that can be used to test a model's performance.
An evaluation is a set of testing criteria and a datasource. After creating an evaluation, you can run it on different models and model parameters. We support several types of graders and datasources.
For more information, see the [Evals guide](/docs/guides/evals).


**Description:**
None.

**Parameters:**


---

### GET /evals/{eval_id}

**Summary:** Get an evaluation by ID.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to retrieve.

---

### POST /evals/{eval_id}

**Summary:** Update certain properties of an evaluation.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to update.

---

### DELETE /evals/{eval_id}

**Summary:** Delete an evaluation.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to delete.

---

### GET /evals/{eval_id}/runs

**Summary:** Get a list of runs for an evaluation.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to retrieve runs for.
* `after` (query) - Identifier for the last run from the previous pagination request.
* `limit` (query) - Number of runs to retrieve.
* `order` (query) - Sort order for runs by timestamp. Use `asc` for ascending order or `desc` for descending order. Defaults to `asc`.
* `status` (query) - Filter runs by status. One of `queued` | `in_progress` | `failed` | `completed` | `canceled`.

---

### POST /evals/{eval_id}/runs

**Summary:** Create a new evaluation run. This is the endpoint that will kick off grading.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to create a run for.

---

### GET /evals/{eval_id}/runs/{run_id}

**Summary:** Get an evaluation run by ID.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to retrieve runs for.
* `run_id` (path) - The ID of the run to retrieve.

---

### POST /evals/{eval_id}/runs/{run_id}

**Summary:** Cancel an ongoing evaluation run.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation whose run you want to cancel.
* `run_id` (path) - The ID of the run to cancel.

---

### DELETE /evals/{eval_id}/runs/{run_id}

**Summary:** Delete an eval run.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to delete the run from.
* `run_id` (path) - The ID of the run to delete.

---

### GET /evals/{eval_id}/runs/{run_id}/output_items

**Summary:** Get a list of output items for an evaluation run.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to retrieve runs for.
* `run_id` (path) - The ID of the run to retrieve output items for.
* `after` (query) - Identifier for the last output item from the previous pagination request.
* `limit` (query) - Number of output items to retrieve.
* `status` (query) - Filter output items by status. Use `failed` to filter by failed output
items or `pass` to filter by passed output items.

* `order` (query) - Sort order for output items by timestamp. Use `asc` for ascending order or `desc` for descending order. Defaults to `asc`.

---

### GET /evals/{eval_id}/runs/{run_id}/output_items/{output_item_id}

**Summary:** Get an evaluation run output item by ID.


**Description:**
None.

**Parameters:**
* `eval_id` (path) - The ID of the evaluation to retrieve runs for.
* `run_id` (path) - The ID of the run to retrieve.
* `output_item_id` (path) - The ID of the output item to retrieve.

---

### GET /files

**Summary:** Returns a list of files.

**Description:**
None.

**Parameters:**
* `purpose` (query) - Only return files with the given purpose.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 10,000, and the default is 10,000.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.


---

### POST /files

**Summary:** Upload a file that can be used across various endpoints. Individual files can be up to 512 MB, and the size of all files uploaded by one organization can be up to 100 GB.

The Assistants API supports files up to 2 million tokens and of specific file types. See the [Assistants Tools guide](/docs/assistants/tools) for details.

The Fine-tuning API only supports `.jsonl` files. The input also has certain required formats for fine-tuning [chat](/docs/api-reference/fine-tuning/chat-input) or [completions](/docs/api-reference/fine-tuning/completions-input) models.

The Batch API only supports `.jsonl` files up to 200 MB in size. The input also has a specific required [format](/docs/api-reference/batch/request-input).

Please [contact us](https://help.openai.com/) if you need to increase these storage limits.


**Description:**
None.

**Parameters:**


---

### DELETE /files/{file_id}

**Summary:** Delete a file.

**Description:**
None.

**Parameters:**
* `file_id` (path) - The ID of the file to use for this request.

---

### GET /files/{file_id}

**Summary:** Returns information about a specific file.

**Description:**
None.

**Parameters:**
* `file_id` (path) - The ID of the file to use for this request.

---

### GET /files/{file_id}/content

**Summary:** Returns the contents of the specified file.

**Description:**
None.

**Parameters:**
* `file_id` (path) - The ID of the file to use for this request.

---

### GET /fine_tuning/checkpoints/{fine_tuned_model_checkpoint}/permissions

**Summary:** **NOTE:** This endpoint requires an [admin API key](../admin-api-keys).

Organization owners can use this endpoint to view all permissions for a fine-tuned model checkpoint.


**Description:**
None.

**Parameters:**
* `fine_tuned_model_checkpoint` (path) - The ID of the fine-tuned model checkpoint to get permissions for.

* `project_id` (query) - The ID of the project to get permissions for.
* `after` (query) - Identifier for the last permission ID from the previous pagination request.
* `limit` (query) - Number of permissions to retrieve.
* `order` (query) - The order in which to retrieve permissions.

---

### POST /fine_tuning/checkpoints/{fine_tuned_model_checkpoint}/permissions

**Summary:** **NOTE:** Calling this endpoint requires an [admin API key](../admin-api-keys).

This enables organization owners to share fine-tuned models with other projects in their organization.


**Description:**
None.

**Parameters:**
* `fine_tuned_model_checkpoint` (path) - The ID of the fine-tuned model checkpoint to create a permission for.


---

### DELETE /fine_tuning/checkpoints/{fine_tuned_model_checkpoint}/permissions/{permission_id}

**Summary:** **NOTE:** This endpoint requires an [admin API key](../admin-api-keys).

Organization owners can use this endpoint to delete a permission for a fine-tuned model checkpoint.


**Description:**
None.

**Parameters:**
* `fine_tuned_model_checkpoint` (path) - The ID of the fine-tuned model checkpoint to delete a permission for.

* `permission_id` (path) - The ID of the fine-tuned model checkpoint permission to delete.


---

### POST /fine_tuning/jobs

**Summary:** Creates a fine-tuning job which begins the process of creating a new model from a given dataset.

Response includes details of the enqueued job including job status and the name of the fine-tuned models once complete.

[Learn more about fine-tuning](/docs/guides/fine-tuning)


**Description:**
None.

**Parameters:**


---

### GET /fine_tuning/jobs

**Summary:** List your organization's fine-tuning jobs


**Description:**
None.

**Parameters:**
* `after` (query) - Identifier for the last job from the previous pagination request.
* `limit` (query) - Number of fine-tuning jobs to retrieve.
* `metadata` (query) - Optional metadata filter. To filter, use the syntax `metadata[k]=v`. Alternatively, set `metadata=null` to indicate no metadata.


---

### GET /fine_tuning/jobs/{fine_tuning_job_id}

**Summary:** Get info about a fine-tuning job.

[Learn more about fine-tuning](/docs/guides/fine-tuning)


**Description:**
None.

**Parameters:**
* `fine_tuning_job_id` (path) - The ID of the fine-tuning job.


---

### POST /fine_tuning/jobs/{fine_tuning_job_id}/cancel

**Summary:** Immediately cancel a fine-tune job.


**Description:**
None.

**Parameters:**
* `fine_tuning_job_id` (path) - The ID of the fine-tuning job to cancel.


---

### GET /fine_tuning/jobs/{fine_tuning_job_id}/checkpoints

**Summary:** List checkpoints for a fine-tuning job.


**Description:**
None.

**Parameters:**
* `fine_tuning_job_id` (path) - The ID of the fine-tuning job to get checkpoints for.

* `after` (query) - Identifier for the last checkpoint ID from the previous pagination request.
* `limit` (query) - Number of checkpoints to retrieve.

---

### GET /fine_tuning/jobs/{fine_tuning_job_id}/events

**Summary:** Get status updates for a fine-tuning job.


**Description:**
None.

**Parameters:**
* `fine_tuning_job_id` (path) - The ID of the fine-tuning job to get events for.

* `after` (query) - Identifier for the last event from the previous pagination request.
* `limit` (query) - Number of events to retrieve.

---

### POST /images/edits

**Summary:** Creates an edited or extended image given one or more source images and a prompt. This endpoint only supports `gpt-image-1` and `dall-e-2`.

**Description:**
None.

**Parameters:**


---

### POST /images/generations

**Summary:** Creates an image given a prompt. [Learn more](/docs/guides/images).


**Description:**
None.

**Parameters:**


---

### POST /images/variations

**Summary:** Creates a variation of a given image. This endpoint only supports `dall-e-2`.

**Description:**
None.

**Parameters:**


---

### GET /models

**Summary:** Lists the currently available models, and provides basic information about each one such as the owner and availability.

**Description:**
None.

**Parameters:**


---

### GET /models/{model}

**Summary:** Retrieves a model instance, providing basic information about the model such as the owner and permissioning.

**Description:**
None.

**Parameters:**
* `model` (path) - The ID of the model to use for this request

---

### DELETE /models/{model}

**Summary:** Delete a fine-tuned model. You must have the Owner role in your organization to delete a model.

**Description:**
None.

**Parameters:**
* `model` (path) - The model to delete

---

### POST /moderations

**Summary:** Classifies if text and/or image inputs are potentially harmful. Learn
more in the [moderation guide](/docs/guides/moderation).


**Description:**
None.

**Parameters:**


---

### GET /organization/admin_api_keys

**Summary:** List organization API keys

**Description:**
Retrieve a paginated list of organization admin API keys.

**Parameters:**
* `after` (query) - No description
* `order` (query) - No description
* `limit` (query) - No description

---

### POST /organization/admin_api_keys

**Summary:** Create an organization admin API key

**Description:**
Create a new admin-level API key for the organization.

**Parameters:**


---

### GET /organization/admin_api_keys/{key_id}

**Summary:** Retrieve a single organization API key

**Description:**
Get details for a specific organization API key by its ID.

**Parameters:**
* `key_id` (path) - No description

---

### DELETE /organization/admin_api_keys/{key_id}

**Summary:** Delete an organization admin API key

**Description:**
Delete the specified admin API key.

**Parameters:**
* `key_id` (path) - No description

---

### GET /organization/audit_logs

**Summary:** List user actions and configuration changes within this organization.

**Description:**
None.

**Parameters:**
* `effective_at` (query) - Return only events whose `effective_at` (Unix seconds) is in this range.
* `project_ids[]` (query) - Return only events for these projects.
* `event_types[]` (query) - Return only events with a `type` in one of these values. For example, `project.created`. For all options, see the documentation for the [audit log object](/docs/api-reference/audit-logs/object).
* `actor_ids[]` (query) - Return only events performed by these actors. Can be a user ID, a service account ID, or an api key tracking ID.
* `actor_emails[]` (query) - Return only events performed by users with these emails.
* `resource_ids[]` (query) - Return only events performed on these targets. For example, a project ID updated.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.


---

### GET /organization/certificates

**Summary:** List uploaded certificates for this organization.

**Description:**
None.

**Parameters:**
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.


---

### POST /organization/certificates

**Summary:** Upload a certificate to the organization. This does **not** automatically activate the certificate.

Organizations can upload up to 50 certificates.


**Description:**
None.

**Parameters:**


---

### POST /organization/certificates/activate

**Summary:** Activate certificates at the organization level.

You can atomically and idempotently activate up to 10 certificates at a time.


**Description:**
None.

**Parameters:**


---

### POST /organization/certificates/deactivate

**Summary:** Deactivate certificates at the organization level.

You can atomically and idempotently deactivate up to 10 certificates at a time.


**Description:**
None.

**Parameters:**


---

### GET /organization/certificates/{certificate_id}

**Summary:** Get a certificate that has been uploaded to the organization.

You can get a certificate regardless of whether it is active or not.


**Description:**
None.

**Parameters:**
* `cert_id` (path) - Unique ID of the certificate to retrieve.
* `include` (query) - A list of additional fields to include in the response. Currently the only supported value is `content` to fetch the PEM content of the certificate.

---

### POST /organization/certificates/{certificate_id}

**Summary:** Modify a certificate. Note that only the name can be modified.


**Description:**
None.

**Parameters:**


---

### DELETE /organization/certificates/{certificate_id}

**Summary:** Delete a certificate from the organization.

The certificate must be inactive for the organization and all projects.


**Description:**
None.

**Parameters:**


---

### GET /organization/costs

**Summary:** Get costs details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently only `1d` is supported, default to `1d`.
* `project_ids` (query) - Return only costs for these projects.
* `group_by` (query) - Group the costs by the specified fields. Support fields include `project_id`, `line_item` and any combination of them.
* `limit` (query) - A limit on the number of buckets to be returned. Limit can range between 1 and 180, and the default is 7.

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/invites

**Summary:** Returns a list of invites in the organization.

**Description:**
None.

**Parameters:**
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.


---

### POST /organization/invites

**Summary:** Create an invite for a user to the organization. The invite must be accepted by the user before they have access to the organization.

**Description:**
None.

**Parameters:**


---

### GET /organization/invites/{invite_id}

**Summary:** Retrieves an invite.

**Description:**
None.

**Parameters:**
* `invite_id` (path) - The ID of the invite to retrieve.

---

### DELETE /organization/invites/{invite_id}

**Summary:** Delete an invite. If the invite has already been accepted, it cannot be deleted.

**Description:**
None.

**Parameters:**
* `invite_id` (path) - The ID of the invite to delete.

---

### GET /organization/projects

**Summary:** Returns a list of projects.

**Description:**
None.

**Parameters:**
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `include_archived` (query) - If `true` returns all projects including those that have been `archived`. Archived projects are not included by default.

---

### POST /organization/projects

**Summary:** Create a new project in the organization. Projects can be created and archived, but cannot be deleted.

**Description:**
None.

**Parameters:**


---

### GET /organization/projects/{project_id}

**Summary:** Retrieves a project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.

---

### POST /organization/projects/{project_id}

**Summary:** Modifies a project in the organization.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.

---

### GET /organization/projects/{project_id}/api_keys

**Summary:** Returns a list of API keys in the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.


---

### GET /organization/projects/{project_id}/api_keys/{key_id}

**Summary:** Retrieves an API key in the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `key_id` (path) - The ID of the API key.

---

### DELETE /organization/projects/{project_id}/api_keys/{key_id}

**Summary:** Deletes an API key from the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `key_id` (path) - The ID of the API key.

---

### POST /organization/projects/{project_id}/archive

**Summary:** Archives a project in the organization. Archived projects cannot be used or updated.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.

---

### GET /organization/projects/{project_id}/certificates

**Summary:** List certificates for this project.

**Description:**
None.

**Parameters:**
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.


---

### POST /organization/projects/{project_id}/certificates/activate

**Summary:** Activate certificates at the project level.

You can atomically and idempotently activate up to 10 certificates at a time.


**Description:**
None.

**Parameters:**


---

### POST /organization/projects/{project_id}/certificates/deactivate

**Summary:** Deactivate certificates at the project level.

You can atomically and idempotently deactivate up to 10 certificates at a time.


**Description:**
None.

**Parameters:**


---

### GET /organization/projects/{project_id}/rate_limits

**Summary:** Returns the rate limits per model for a project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `limit` (query) - A limit on the number of objects to be returned. The default is 100.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, beginning with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.


---

### POST /organization/projects/{project_id}/rate_limits/{rate_limit_id}

**Summary:** Updates a project rate limit.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `rate_limit_id` (path) - The ID of the rate limit.

---

### GET /organization/projects/{project_id}/service_accounts

**Summary:** Returns a list of service accounts in the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.


---

### POST /organization/projects/{project_id}/service_accounts

**Summary:** Creates a new service account in the project. This also returns an unredacted API key for the service account.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.

---

### GET /organization/projects/{project_id}/service_accounts/{service_account_id}

**Summary:** Retrieves a service account in the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `service_account_id` (path) - The ID of the service account.

---

### DELETE /organization/projects/{project_id}/service_accounts/{service_account_id}

**Summary:** Deletes a service account from the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `service_account_id` (path) - The ID of the service account.

---

### GET /organization/projects/{project_id}/users

**Summary:** Returns a list of users in the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.


---

### POST /organization/projects/{project_id}/users

**Summary:** Adds a user to the project. Users must already be members of the organization to be added to a project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.

---

### GET /organization/projects/{project_id}/users/{user_id}

**Summary:** Retrieves a user in the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `user_id` (path) - The ID of the user.

---

### POST /organization/projects/{project_id}/users/{user_id}

**Summary:** Modifies a user's role in the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `user_id` (path) - The ID of the user.

---

### DELETE /organization/projects/{project_id}/users/{user_id}

**Summary:** Deletes a user from the project.

**Description:**
None.

**Parameters:**
* `project_id` (path) - The ID of the project.
* `user_id` (path) - The ID of the user.

---

### GET /organization/usage/audio_speeches

**Summary:** Get audio speeches usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `project_ids` (query) - Return only usage for these projects.
* `user_ids` (query) - Return only usage for these users.
* `api_key_ids` (query) - Return only usage for these API keys.
* `models` (query) - Return only usage for these models.
* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`, `user_id`, `api_key_id`, `model` or any combination of them.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/usage/audio_transcriptions

**Summary:** Get audio transcriptions usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `project_ids` (query) - Return only usage for these projects.
* `user_ids` (query) - Return only usage for these users.
* `api_key_ids` (query) - Return only usage for these API keys.
* `models` (query) - Return only usage for these models.
* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`, `user_id`, `api_key_id`, `model` or any combination of them.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/usage/code_interpreter_sessions

**Summary:** Get code interpreter sessions usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `project_ids` (query) - Return only usage for these projects.
* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/usage/completions

**Summary:** Get completions usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `project_ids` (query) - Return only usage for these projects.
* `user_ids` (query) - Return only usage for these users.
* `api_key_ids` (query) - Return only usage for these API keys.
* `models` (query) - Return only usage for these models.
* `batch` (query) - If `true`, return batch jobs only. If `false`, return non-batch jobs only. By default, return both.

* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`, `user_id`, `api_key_id`, `model`, `batch` or any combination of them.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/usage/embeddings

**Summary:** Get embeddings usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `project_ids` (query) - Return only usage for these projects.
* `user_ids` (query) - Return only usage for these users.
* `api_key_ids` (query) - Return only usage for these API keys.
* `models` (query) - Return only usage for these models.
* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`, `user_id`, `api_key_id`, `model` or any combination of them.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/usage/images

**Summary:** Get images usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `sources` (query) - Return only usages for these sources. Possible values are `image.generation`, `image.edit`, `image.variation` or any combination of them.
* `sizes` (query) - Return only usages for these image sizes. Possible values are `256x256`, `512x512`, `1024x1024`, `1792x1792`, `1024x1792` or any combination of them.
* `project_ids` (query) - Return only usage for these projects.
* `user_ids` (query) - Return only usage for these users.
* `api_key_ids` (query) - Return only usage for these API keys.
* `models` (query) - Return only usage for these models.
* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`, `user_id`, `api_key_id`, `model`, `size`, `source` or any combination of them.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/usage/moderations

**Summary:** Get moderations usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `project_ids` (query) - Return only usage for these projects.
* `user_ids` (query) - Return only usage for these users.
* `api_key_ids` (query) - Return only usage for these API keys.
* `models` (query) - Return only usage for these models.
* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`, `user_id`, `api_key_id`, `model` or any combination of them.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/usage/vector_stores

**Summary:** Get vector stores usage details for the organization.

**Description:**
None.

**Parameters:**
* `start_time` (query) - Start time (Unix seconds) of the query time range, inclusive.
* `end_time` (query) - End time (Unix seconds) of the query time range, exclusive.
* `bucket_width` (query) - Width of each time bucket in response. Currently `1m`, `1h` and `1d` are supported, default to `1d`.
* `project_ids` (query) - Return only usage for these projects.
* `group_by` (query) - Group the usage data by the specified fields. Support fields include `project_id`.
* `limit` (query) - Specifies the number of buckets to return.
- `bucket_width=1d`: default: 7, max: 31
- `bucket_width=1h`: default: 24, max: 168
- `bucket_width=1m`: default: 60, max: 1440

* `page` (query) - A cursor for use in pagination. Corresponding to the `next_page` field from the previous response.

---

### GET /organization/users

**Summary:** Lists all of the users in the organization.

**Description:**
None.

**Parameters:**
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `emails` (query) - Filter by the email address of users.

---

### GET /organization/users/{user_id}

**Summary:** Retrieves a user by their identifier.

**Description:**
None.

**Parameters:**
* `user_id` (path) - The ID of the user.

---

### POST /organization/users/{user_id}

**Summary:** Modifies a user's role in the organization.

**Description:**
None.

**Parameters:**
* `user_id` (path) - The ID of the user.

---

### DELETE /organization/users/{user_id}

**Summary:** Deletes a user from the organization.

**Description:**
None.

**Parameters:**
* `user_id` (path) - The ID of the user.

---

### POST /realtime/sessions

**Summary:** Create an ephemeral API token for use in client-side applications with the
Realtime API. Can be configured with the same session parameters as the
`session.update` client event.

It responds with a session object, plus a `client_secret` key which contains
a usable ephemeral API token that can be used to authenticate browser clients
for the Realtime API.


**Description:**
None.

**Parameters:**


---

### POST /realtime/transcription_sessions

**Summary:** Create an ephemeral API token for use in client-side applications with the
Realtime API specifically for realtime transcriptions. 
Can be configured with the same session parameters as the `transcription_session.update` client event.

It responds with a session object, plus a `client_secret` key which contains
a usable ephemeral API token that can be used to authenticate browser clients
for the Realtime API.


**Description:**
None.

**Parameters:**


---

### POST /responses

**Summary:** Creates a model response. Provide [text](/docs/guides/text) or
[image](/docs/guides/images) inputs to generate [text](/docs/guides/text)
or [JSON](/docs/guides/structured-outputs) outputs. Have the model call
your own [custom code](/docs/guides/function-calling) or use built-in
[tools](/docs/guides/tools) like [web search](/docs/guides/tools-web-search)
or [file search](/docs/guides/tools-file-search) to use your own data
as input for the model's response.


**Description:**
None.

**Parameters:**


---

### GET /responses/{response_id}

**Summary:** Retrieves a model response with the given ID.


**Description:**
None.

**Parameters:**
* `response_id` (path) - The ID of the response to retrieve.
* `include` (query) - Additional fields to include in the response. See the `include`
parameter for Response creation above for more information.


---

### DELETE /responses/{response_id}

**Summary:** Deletes a model response with the given ID.


**Description:**
None.

**Parameters:**
* `response_id` (path) - The ID of the response to delete.

---

### GET /responses/{response_id}/input_items

**Summary:** Returns a list of input items for a given response.

**Description:**
None.

**Parameters:**
* `response_id` (path) - The ID of the response to retrieve input items for.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between
1 and 100, and the default is 20.

* `order` (query) - The order to return the input items in. Default is `asc`.
- `asc`: Return the input items in ascending order.
- `desc`: Return the input items in descending order.

* `after` (query) - An item ID to list items after, used in pagination.

* `before` (query) - An item ID to list items before, used in pagination.

* `include` (query) - Additional fields to include in the response. See the `include`
parameter for Response creation above for more information.


---

### POST /threads

**Summary:** Create a thread.

**Description:**
None.

**Parameters:**


---

### POST /threads/runs

**Summary:** Create a thread and run it in one request.

**Description:**
None.

**Parameters:**


---

### GET /threads/{thread_id}

**Summary:** Retrieves a thread.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to retrieve.

---

### POST /threads/{thread_id}

**Summary:** Modifies a thread.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to modify. Only the `metadata` can be modified.

---

### DELETE /threads/{thread_id}

**Summary:** Delete a thread.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to delete.

---

### GET /threads/{thread_id}/messages

**Summary:** Returns a list of messages for a given thread.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the [thread](/docs/api-reference/threads) the messages belong to.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.

* `run_id` (query) - Filter messages by the run ID that generated them.


---

### POST /threads/{thread_id}/messages

**Summary:** Create a message.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the [thread](/docs/api-reference/threads) to create a message for.

---

### GET /threads/{thread_id}/messages/{message_id}

**Summary:** Retrieve a message.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the [thread](/docs/api-reference/threads) to which this message belongs.
* `message_id` (path) - The ID of the message to retrieve.

---

### POST /threads/{thread_id}/messages/{message_id}

**Summary:** Modifies a message.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to which this message belongs.
* `message_id` (path) - The ID of the message to modify.

---

### DELETE /threads/{thread_id}/messages/{message_id}

**Summary:** Deletes a message.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to which this message belongs.
* `message_id` (path) - The ID of the message to delete.

---

### GET /threads/{thread_id}/runs

**Summary:** Returns a list of runs belonging to a thread.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread the run belongs to.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.


---

### POST /threads/{thread_id}/runs

**Summary:** Create a run.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to run.
* `include[]` (query) - A list of additional fields to include in the response. Currently the only supported value is `step_details.tool_calls[*].file_search.results[*].content` to fetch the file search result content.

See the [file search tool documentation](/docs/assistants/tools/file-search#customizing-file-search-settings) for more information.


---

### GET /threads/{thread_id}/runs/{run_id}

**Summary:** Retrieves a run.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the [thread](/docs/api-reference/threads) that was run.
* `run_id` (path) - The ID of the run to retrieve.

---

### POST /threads/{thread_id}/runs/{run_id}

**Summary:** Modifies a run.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the [thread](/docs/api-reference/threads) that was run.
* `run_id` (path) - The ID of the run to modify.

---

### POST /threads/{thread_id}/runs/{run_id}/cancel

**Summary:** Cancels a run that is `in_progress`.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to which this run belongs.
* `run_id` (path) - The ID of the run to cancel.

---

### GET /threads/{thread_id}/runs/{run_id}/steps

**Summary:** Returns a list of run steps belonging to a run.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread the run and run steps belong to.
* `run_id` (path) - The ID of the run the run steps belong to.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.

* `include[]` (query) - A list of additional fields to include in the response. Currently the only supported value is `step_details.tool_calls[*].file_search.results[*].content` to fetch the file search result content.

See the [file search tool documentation](/docs/assistants/tools/file-search#customizing-file-search-settings) for more information.


---

### GET /threads/{thread_id}/runs/{run_id}/steps/{step_id}

**Summary:** Retrieves a run step.

**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the thread to which the run and run step belongs.
* `run_id` (path) - The ID of the run to which the run step belongs.
* `step_id` (path) - The ID of the run step to retrieve.
* `include[]` (query) - A list of additional fields to include in the response. Currently the only supported value is `step_details.tool_calls[*].file_search.results[*].content` to fetch the file search result content.

See the [file search tool documentation](/docs/assistants/tools/file-search#customizing-file-search-settings) for more information.


---

### POST /threads/{thread_id}/runs/{run_id}/submit_tool_outputs

**Summary:** When a run has the `status: "requires_action"` and `required_action.type` is `submit_tool_outputs`, this endpoint can be used to submit the outputs from the tool calls once they're all completed. All outputs must be submitted in a single request.


**Description:**
None.

**Parameters:**
* `thread_id` (path) - The ID of the [thread](/docs/api-reference/threads) to which this run belongs.
* `run_id` (path) - The ID of the run that requires the tool output submission.

---

### POST /uploads

**Summary:** Creates an intermediate [Upload](/docs/api-reference/uploads/object) object
that you can add [Parts](/docs/api-reference/uploads/part-object) to.
Currently, an Upload can accept at most 8 GB in total and expires after an
hour after you create it.

Once you complete the Upload, we will create a
[File](/docs/api-reference/files/object) object that contains all the parts
you uploaded. This File is usable in the rest of our platform as a regular
File object.

For certain `purpose` values, the correct `mime_type` must be specified. 
Please refer to documentation for the 
[supported MIME types for your use case](/docs/assistants/tools/file-search#supported-files).

For guidance on the proper filename extensions for each purpose, please
follow the documentation on [creating a
File](/docs/api-reference/files/create).


**Description:**
None.

**Parameters:**


---

### POST /uploads/{upload_id}/cancel

**Summary:** Cancels the Upload. No Parts may be added after an Upload is cancelled.


**Description:**
None.

**Parameters:**
* `upload_id` (path) - The ID of the Upload.


---

### POST /uploads/{upload_id}/complete

**Summary:** Completes the [Upload](/docs/api-reference/uploads/object). 

Within the returned Upload object, there is a nested [File](/docs/api-reference/files/object) object that is ready to use in the rest of the platform.

You can specify the order of the Parts by passing in an ordered list of the Part IDs.

The number of bytes uploaded upon completion must match the number of bytes initially specified when creating the Upload object. No Parts may be added after an Upload is completed.


**Description:**
None.

**Parameters:**
* `upload_id` (path) - The ID of the Upload.


---

### POST /uploads/{upload_id}/parts

**Summary:** Adds a [Part](/docs/api-reference/uploads/part-object) to an [Upload](/docs/api-reference/uploads/object) object. A Part represents a chunk of bytes from the file you are trying to upload. 

Each Part can be at most 64 MB, and you can add Parts until you hit the Upload maximum of 8 GB.

It is possible to add multiple Parts in parallel. You can decide the intended order of the Parts when you [complete the Upload](/docs/api-reference/uploads/complete).


**Description:**
None.

**Parameters:**
* `upload_id` (path) - The ID of the Upload.


---

### GET /vector_stores

**Summary:** Returns a list of vector stores.

**Description:**
None.

**Parameters:**
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.


---

### POST /vector_stores

**Summary:** Create a vector store.

**Description:**
None.

**Parameters:**


---

### GET /vector_stores/{vector_store_id}

**Summary:** Retrieves a vector store.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store to retrieve.

---

### POST /vector_stores/{vector_store_id}

**Summary:** Modifies a vector store.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store to modify.

---

### DELETE /vector_stores/{vector_store_id}

**Summary:** Delete a vector store.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store to delete.

---

### POST /vector_stores/{vector_store_id}/file_batches

**Summary:** Create a vector store file batch.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store for which to create a File Batch.


---

### GET /vector_stores/{vector_store_id}/file_batches/{batch_id}

**Summary:** Retrieves a vector store file batch.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store that the file batch belongs to.
* `batch_id` (path) - The ID of the file batch being retrieved.

---

### POST /vector_stores/{vector_store_id}/file_batches/{batch_id}/cancel

**Summary:** Cancel a vector store file batch. This attempts to cancel the processing of files in this batch as soon as possible.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store that the file batch belongs to.
* `batch_id` (path) - The ID of the file batch to cancel.

---

### GET /vector_stores/{vector_store_id}/file_batches/{batch_id}/files

**Summary:** Returns a list of vector store files in a batch.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store that the files belong to.
* `batch_id` (path) - The ID of the file batch that the files belong to.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.

* `filter` (query) - Filter by file status. One of `in_progress`, `completed`, `failed`, `cancelled`.

---

### GET /vector_stores/{vector_store_id}/files

**Summary:** Returns a list of vector store files.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store that the files belong to.
* `limit` (query) - A limit on the number of objects to be returned. Limit can range between 1 and 100, and the default is 20.

* `order` (query) - Sort order by the `created_at` timestamp of the objects. `asc` for ascending order and `desc` for descending order.

* `after` (query) - A cursor for use in pagination. `after` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, ending with obj_foo, your subsequent call can include after=obj_foo in order to fetch the next page of the list.

* `before` (query) - A cursor for use in pagination. `before` is an object ID that defines your place in the list. For instance, if you make a list request and receive 100 objects, starting with obj_foo, your subsequent call can include before=obj_foo in order to fetch the previous page of the list.

* `filter` (query) - Filter by file status. One of `in_progress`, `completed`, `failed`, `cancelled`.

---

### POST /vector_stores/{vector_store_id}/files

**Summary:** Create a vector store file by attaching a [File](/docs/api-reference/files) to a [vector store](/docs/api-reference/vector-stores/object).

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store for which to create a File.


---

### GET /vector_stores/{vector_store_id}/files/{file_id}

**Summary:** Retrieves a vector store file.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store that the file belongs to.
* `file_id` (path) - The ID of the file being retrieved.

---

### DELETE /vector_stores/{vector_store_id}/files/{file_id}

**Summary:** Delete a vector store file. This will remove the file from the vector store but the file itself will not be deleted. To delete the file, use the [delete file](/docs/api-reference/files/delete) endpoint.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store that the file belongs to.
* `file_id` (path) - The ID of the file to delete.

---

### POST /vector_stores/{vector_store_id}/files/{file_id}

**Summary:** Update attributes on a vector store file.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store the file belongs to.
* `file_id` (path) - The ID of the file to update attributes.

---

### GET /vector_stores/{vector_store_id}/files/{file_id}/content

**Summary:** Retrieve the parsed contents of a vector store file.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store.
* `file_id` (path) - The ID of the file within the vector store.

---

### POST /vector_stores/{vector_store_id}/search

**Summary:** Search a vector store for relevant chunks based on a query and file attributes filter.

**Description:**
None.

**Parameters:**
* `vector_store_id` (path) - The ID of the vector store to search.

---

