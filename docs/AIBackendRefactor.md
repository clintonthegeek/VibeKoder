# Developer Documentation: AI Backend Refactor and Streaming Integration Plan

## Overview

This document outlines the plan to refactor the AI integration in VibeKoder by removing legacy OpenAI-specific classes (`OpenAIMessage` and `OpenAIRequest`) and replacing them with a flexible, extensible abstraction layer to support multiple AI backends (e.g., OpenAI, Ollama, or future providers).

The goal is to create a clean, maintainable architecture that supports:

- Multiple AI backends via a common interface.
- Streaming responses by default, with robust handling.
- Per-request and global configuration.
- Graceful cancellation.
- Integration with the session system using atomic saves.
- Future-proofing for new backends and features.

---

## Current State

- `OpenAIMessage` and `OpenAIRequest` are tightly coupled to OpenAI's API.
- The app directly uses these classes for sending prompts and receiving responses.
- Streaming is not currently implemented; responses are received fully before display.
- Session files are saved only after full response is received.

---

## Refactor Goals

1. **Remove `OpenAIMessage` and `OpenAIRequest` classes entirely.**

2. **Create an abstract base class `AIBackend`**  
   - Defines a Qt `QObject` interface for AI backends.  
   - Supports chat-style messages with roles (system, user, assistant).  
   - Supports generic key-value configuration parameters.  
   - Supports cancellation of requests.  
   - Supports multiple concurrent requests identified by request IDs.  
   - Emits signals for partial (streamed) responses, full responses, errors, and status changes.

3. **Implement `OpenAIBackend` as a concrete subclass of `AIBackend`.**  
   - Implements OpenAI API calls directly (using `QNetworkAccessManager` or similar).  
   - Supports streaming responses natively (using OpenAI streaming API).  
   - Supports all relevant API parameters, including those currently in the project file and additional ones (stop sequences, user ID, logit bias, etc.).  
   - Manages multiple concurrent requests.  
   - Handles cancellation cleanly.  
   - Emits partial responses as data arrives.  
   - Buffers streamed data into a temporary file during the request.

4. **Streaming and Session File Integration**  
   - Streamed response text is appended incrementally to a temporary file on disk.  
   - The UI text widget (session tab editor) is updated incrementally from the temp file, providing near real-time feedback.  
   - Upon stream completion, the temporary file content is atomically saved into the session markdown file, preserving data integrity.  
   - This approach prevents data loss if the app crashes during streaming and avoids UI freezes.

5. **Support for Multiple Backends**  
   - The `AIBackend` interface allows future backends (e.g., OllamaBackend) to be implemented with minimal changes to the app.  
   - Backends can declare capabilities (streaming support, parameter sets).  
   - The app can select or switch backends at runtime or per session.

6. **Configuration Management**  
   - Global backend config (API keys, default model, etc.) is stored in the `AIBackend` instance.  
   - Per-request overrides are supported via key-value parameter maps.  
   - The `Project` class will be extended to include additional API parameters as needed.

7. **Session State and History**  
   - The app continues to manage session state, conversation history, and prompt slices.  
   - The backend is stateless, receiving full message lists per request.

---

## Implementation Notes and Recommendations

- **Network Layer:**  
  Implement OpenAI API calls using `QNetworkAccessManager` and handle streaming via chunked HTTP responses or SSE (Server-Sent Events).  
  Parse partial JSON payloads as they arrive, extract content tokens, and emit `partialResponse` signals.

- **Temporary File Handling:**  
  - Create a unique temporary file per request to store streamed text.  
  - Append incoming chunks to the file atomically.  
  - Use Qt file locking or atomic rename operations to ensure safe writes.  
  - Update UI text widget by reading from the temp file periodically or on signal.

- **Atomic Save on Stream End:**  
  - When the full response is received, atomically replace the session markdown file or relevant prompt slice content with the temp file content.  
  - This ensures no partial data is lost or corrupted.

- **Cancellation:**  
  - Implement cancellation by aborting the network request and cleaning up temp files.  
  - Emit appropriate signals to update UI.

- **Backward Compatibility:**  
  - Since old classes are removed, update all code that used `OpenAIRequest` or `OpenAIMessage` to use `AIBackend` interface.  
  - Refactor `SessionTabWidget` and other UI components to handle partial responses and streaming updates.

- **Error Handling:**  
  - Provide clear error messages via signals.  
  - Handle network errors, API errors, and JSON parse errors gracefully.

- **Testing:**  
  - Write unit tests for `OpenAIBackend` network logic and streaming parsing.  
  - Test cancellation and error scenarios.  
  - Test integration with session file saving and UI updates.

---

## Developer Workflow

1. **Create a new Git branch** (e.g., `aibackend-refactor`).

2. **Remove `OpenAIMessage` and `OpenAIRequest` classes and usages.**

3. **Implement `AIBackend` abstract class as described.**

4. **Implement `OpenAIBackend` subclass with streaming support and config management.**

5. **Refactor UI components (`SessionTabWidget`, `MainWindow`) to use `AIBackend` interface.**

6. **Implement temporary file streaming and atomic save logic.**

7. **Extend `Project` class to include new API config parameters.**

8. **Test thoroughly and iterate.**

---

## Future Considerations

- Implement an `OllamaBackend` subclass using the same interface.  
- Add UI to select backend per project or session.  
- Add UI to tweak per-request parameters dynamically in the session tab.  
- Support advanced OpenAI features like function calls or embeddings in the backend interface.  
- Consider caching or rate-limiting strategies in backends.

---

## Summary

This refactor will modernize the AI integration in VibeKoder, improve maintainability, and enable new features like streaming and multi-backend support. The atomic save strategy ensures data integrity during streaming. The new `AIBackend` interface cleanly separates backend implementations from app logic.
