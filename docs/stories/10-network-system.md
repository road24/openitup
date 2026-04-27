# Network System User Stories

**Status**: FUTURE  
**Epic Scope**: Subsystem 10 (Network) — Phases 8-9  
**Roadmap Context**: Enables online score submission and leaderboards after arcade I/O support is complete. All network features are additive and optional; the engine remains fully functional offline.

---

## Architecture Foundation

### Story ID: US-NET-001 — HTTP Client Integration

**Story Card:**
> **As a** Developer  
> **I want** a thread-safe HTTP client library integrated into the build system  
> **So that** the engine can communicate with remote servers without blocking gameplay

#### 📝 Description
Integrate libcurl or cpp-httplib via CMake FetchContent. The client must support HTTPS, custom headers, JSON request/response bodies, and configurable timeouts. This is the foundation for all network operations.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Successful GET request**
    *   **Given** a test HTTP server is running on localhost:8080
    *   **When** the client makes a GET request to http://localhost:8080/health
    *   **Then** the response status code is 200 and the response body is returned as a string

*   **Scenario 2: Request timeout**
    *   **Given** the timeout is configured to 5 seconds
    *   **When** the client makes a request to a server that does not respond within 5 seconds
    *   **Then** the request fails with a timeout error and does not block the calling thread beyond the timeout duration

*   **Scenario 3: HTTPS certificate validation**
    *   **Given** a remote HTTPS endpoint with a valid certificate
    *   **When** the client makes a request to that endpoint
    *   **Then** the request succeeds and certificate validation passes

*   **Scenario 4: Connection failure handling**
    *   **Given** no server is listening at the target address
    *   **When** the client attempts a request
    *   **Then** the request fails with a connection error and does not crash the engine

#### 📊 Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: REQ-ENG-002 (engine initialization)
*   **Phase**: 8

---

### Story ID: US-NET-002 — Background Thread Request Executor

**Story Card:**
> **As a** Developer  
> **I want** network requests executed on a dedicated background thread with main-thread callbacks  
> **So that** HTTP operations never block rendering or input processing

#### 📝 Description
Create a `NetworkService` class that owns a background thread and a request queue. Requests are enqueued from the main thread, processed on the background thread, and results are delivered via callback on the main thread during the next frame.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Non-blocking request submission**
    *   **Given** a network request is submitted during a frame
    *   **When** the frame render time is measured
    *   **Then** the request submission adds no more than 0.1 milliseconds to the frame time

*   **Scenario 2: Callback execution on main thread**
    *   **Given** a request completes on the background thread
    *   **When** the next `Engine::update()` call occurs
    *   **Then** the completion callback is invoked on the main thread before the frame ends

*   **Scenario 3: Multiple concurrent requests**
    *   **Given** 10 requests are submitted within the same frame
    *   **When** all requests complete
    *   **Then** each callback is invoked exactly once with the correct response data

*   **Scenario 4: Request cancellation**
    *   **Given** a request is queued but not yet started
    *   **When** the caller cancels the request
    *   **Then** the callback is never invoked and the request is removed from the queue

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-001
*   **Phase**: 8

---

### Story ID: US-NET-003 — Default Request Timeout

**Story Card:**
> **As a** Player  
> **I want** network requests that fail to complete within 30 seconds to time out automatically  
> **So that** slow or dead connections do not leave the game hanging indefinitely

#### 📝 Description
All network requests must have a 30-second default timeout. Requests that exceed this duration fail with a timeout error and invoke the error callback. The timeout value is configurable via settings.json but defaults to 30000 milliseconds.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Default timeout enforced**
    *   **Given** no custom timeout is specified for a request
    *   **When** the request takes 31 seconds to complete
    *   **Then** the request fails with a timeout error after 30 seconds

*   **Scenario 2: Custom timeout respected**
    *   **Given** a request is submitted with a 10-second timeout override
    *   **When** the request takes 11 seconds
    *   **Then** the request fails after 10 seconds

*   **Scenario 3: Timeout notification**
    *   **Given** a request times out
    *   **When** the error callback is invoked
    *   **Then** the error message includes "timeout" and the duration that was waited

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-002
*   **Phase**: 8

---

## Authentication and Accounts

### Story ID: US-NET-010 — Account Registration

**Story Card:**
> **As a** Player  
> **I want** account registration with username, email, and password  
> **So that** I can create an identity for submitting scores

#### 📝 Description
Provide a registration form in the UI (Lua-driven screen in Phase 9) that collects username, email, and password, then posts to `/api/auth/register`. The server returns a session token on success. Validation errors are displayed to the player. Password policy: minimum 8 characters, at least one number.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Successful registration**
    *   **Given** the username "player123", email "player@example.com", and password "SecurePass1!" are entered
    *   **When** the registration request is submitted
    *   **Then** the server responds with HTTP 201, a session token, and the player's profile is created locally with the token stored

*   **Scenario 2: Username already taken**
    *   **Given** the username "player123" already exists on the server
    *   **When** a registration request with that username is submitted
    *   **Then** the server responds with HTTP 409 and an error message "Username already exists" is displayed

*   **Scenario 3: Invalid email format**
    *   **Given** the email "notanemail" is entered
    *   **When** the registration form is submitted
    *   **Then** a validation error "Invalid email format" is shown before the request is sent

*   **Scenario 4: Weak password rejected**
    *   **Given** the password "short" is entered (fewer than 8 characters)
    *   **When** the registration form is submitted
    *   **Then** a validation error "Password must be at least 8 characters and contain at least one number" is shown

*   **Scenario 5: Password missing number**
    *   **Given** the password "ValidPassword" is entered (8+ chars but no number)
    *   **When** the registration form is submitted
    *   **Then** a validation error "Password must contain at least one number" is shown

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-002, REQ-DAT-001 (profile system)
*   **Phase**: 9

---

### Story ID: US-NET-011 — Account Login

**Story Card:**
> **As a** Player  
> **I want** login with username and password returning a session token  
> **So that** I can authenticate my score submissions

#### 📝 Description
Provide a login form that posts username and password to `/api/auth/login`. On success, the session token is stored in the active player profile. On failure, an error message is displayed. The token is included in all authenticated requests via an `Authorization: Bearer <token>` header.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Successful login**
    *   **Given** a registered account with username "player123" and password "SecurePass1!"
    *   **When** those credentials are submitted to the login form
    *   **Then** the server responds with HTTP 200, a session token, and the token is stored in the profile

*   **Scenario 2: Incorrect password**
    *   **Given** the username "player123" exists but the password "WrongPass" is entered
    *   **When** the login request is submitted
    *   **Then** the server responds with HTTP 401 and "Invalid credentials" is displayed

*   **Scenario 3: Non-existent username**
    *   **Given** the username "nonexistent" does not exist
    *   **When** the login request is submitted
    *   **Then** the server responds with HTTP 401 and "Invalid credentials" is displayed

*   **Scenario 4: Token persistence**
    *   **Given** a player logs in successfully
    *   **When** the game is closed and reopened
    *   **Then** the session token is still present in the profile and authenticated requests work without re-login

#### 📊 Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-NET-010, REQ-DAT-001 (profile system)
*   **Phase**: 9

---

### Story ID: US-NET-012 — Token Refresh Mechanism

**Story Card:**
> **As a** Player  
> **I want** expired session tokens to refresh automatically during long play sessions  
> **So that** I do not need to re-login repeatedly

#### 📝 Description
Session tokens expire after 7 days. If a request fails with HTTP 401 and the response indicates token expiration, the client automatically attempts to refresh the token using a `/api/auth/refresh` endpoint. If refresh succeeds, the original request is retried. If refresh fails, the player is logged out.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Automatic refresh on token expiration**
    *   **Given** a session token has expired
    *   **When** a score submission request is made
    *   **Then** the request fails with 401, the client automatically calls `/api/auth/refresh`, receives a new token, and retries the score submission

*   **Scenario 2: Refresh failure leads to logout**
    *   **Given** a refresh request fails with HTTP 401 (refresh token also expired)
    *   **When** the failure is detected
    *   **Then** the player's session token is cleared from the profile and a "Session expired, please log in again" message is displayed

*   **Scenario 3: No unnecessary refresh**
    *   **Given** a session token is still valid for 5 more days
    *   **When** an authenticated request is made
    *   **Then** no refresh attempt occurs and the request proceeds normally

*   **Scenario 4: Client-side expiry detection**
    *   **Given** the client detects the stored token is expired before attempting a request
    *   **When** the user tries to submit a score
    *   **Then** the client immediately attempts a token refresh without sending the score request, and if refresh fails, displays "Session expired, please log in again"

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-011
*   **Phase**: 9

---

## Score Submission

### Story ID: US-NET-020 — Post Score After Gameplay

**Story Card:**
> **As a** Player  
> **I want** my score automatically submitted to the server after completing a song  
> **So that** it appears on leaderboards without manual action

#### 📝 Description
When `GameplayScene` transitions to `ResultScene`, if the player is logged in and has a network connection, the score is posted to `/api/scores`. The request includes score, grade, max combo, judgment counts (Perfect/Great/Good/Bad/Miss), chart hash, judge profile identifier, and replay hash.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Successful submission**
    *   **Given** a song is completed with a score of 850000, grade A, max combo 300, and specific judgment counts
    *   **When** the result screen is displayed and the player is logged in
    *   **Then** the score is posted to `/api/scores` within 2 seconds and the server responds with HTTP 201

*   **Scenario 2: Submission failure is silent**
    *   **Given** the score submission request fails with HTTP 500
    *   **When** the failure is detected
    *   **Then** the result screen is not interrupted, the score is saved locally, and a background notification appears saying "Score submission failed, will retry later"

*   **Scenario 3: Offline mode skips submission**
    *   **Given** the player has no internet connection
    *   **When** a song is completed
    *   **Then** no network request is attempted and the score is saved to the local queue

*   **Scenario 4: Unauthenticated player skips submission**
    *   **Given** the player is not logged in
    *   **When** a song is completed
    *   **Then** no network request is made and no error is shown

#### 📊 Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-NET-011, REQ-CHT-010 (chart hash), REQ-JDG-001 (judge profile ID)
*   **Phase**: 8

---

### Story ID: US-NET-021 — Score Submission Payload

**Story Card:**
> **As a** Developer  
> **I want** score submissions to include chart hash, judge profile version, and replay hash  
> **So that** the server can validate score authenticity and prevent cheating

#### 📝 Description
The score submission JSON payload includes: `score` (int), `grade` (string), `max_combo` (int), `perfect_count` (int), `great_count` (int), `good_count` (int), `bad_count` (int), `miss_count` (int), `chart_hash` (string, SHA-256 hex), `judge_profile` (string, e.g., "exceed_v1"), `replay_hash` (string, SHA-256 hex), `timestamp` (ISO 8601).

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Payload structure validation**
    *   **Given** a score submission is prepared
    *   **When** the JSON is serialized
    *   **Then** all required fields are present, chart_hash is 64 hex characters, replay_hash is 64 hex characters, and timestamp is valid ISO 8601

*   **Scenario 2: Chart hash matches chart content**
    *   **Given** two identical charts loaded from different file formats (one KSF, one SSC)
    *   **When** scores are submitted for each
    *   **Then** both submissions have the same `chart_hash` value

*   **Scenario 3: Judge profile identifier correct**
    *   **Given** the player is using the "Exceed v1" judge profile
    *   **When** a score is submitted
    *   **Then** the `judge_profile` field is "exceed_v1"

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-020, REQ-CHT-010 (chart hash), REQ-NET-008 (replay hash)
*   **Phase**: 8

---

### Story ID: US-NET-022 — Failed Submission Queue

**Story Card:**
> **As a** Player  
> **I want** failed score submissions saved locally and retried automatically  
> **So that** I do not lose scores due to temporary network issues

#### 📝 Description
Failed submissions are written to `~/.local/share/openitup/profiles/<profile>/submission_queue.json`. On the next successful network operation (any request that returns HTTP 2xx), the queue is processed and submissions are retried. The queue is limited to 100 entries (oldest are dropped).

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Queue persistence**
    *   **Given** a score submission fails with a network error
    *   **When** the failure is detected
    *   **Then** the submission payload is appended to `submission_queue.json`

*   **Scenario 2: Automatic retry on reconnection**
    *   **Given** 3 failed submissions are in the queue
    *   **When** the player logs in successfully (network is now available)
    *   **Then** all 3 submissions are retried within 5 seconds

*   **Scenario 3: Duplicate prevention**
    *   **Given** a submission has a unique request ID
    *   **When** the same submission is attempted twice (e.g., via manual retry and auto-retry)
    *   **Then** the server accepts only the first submission and returns HTTP 409 for the duplicate

*   **Scenario 4: Queue size limit**
    *   **Given** the queue already contains 100 entries
    *   **When** a new submission fails
    *   **Then** the oldest entry is removed and the new entry is added

*   **Scenario 5: Player notification**
    *   **Given** 5 submissions are pending in the queue
    *   **When** the player views their profile or result screen
    *   **Then** a message "5 scores pending upload" is displayed

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-020
*   **Phase**: 8

---

### Story ID: US-NET-023 — Capture RNG Seed with Score

**Story Card:**
> **As a** Developer  
> **I want** the random number generator seed captured with each score submission  
> **So that** replays can be reproduced with identical visual effects and animations

#### 📝 Description
The RNG seed used for non-gameplay-affecting randomness (e.g., background animations, particle effects) is recorded at the start of gameplay. This seed is included in the score submission payload as `rng_seed` (64-bit integer). During replay playback, the same seed is used to ensure visual determinism.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: RNG seed recorded**
    *   **Given** a gameplay session starts with RNG seed 123456789
    *   **When** the session completes
    *   **Then** the score submission includes `"rng_seed": 123456789`

*   **Scenario 2: Replay playback uses recorded seed**
    *   **Given** a replay is loaded with RNG seed 987654321
    *   **When** the replay is played back
    *   **Then** the RNG is initialized with seed 987654321 and background animations match the original session

*   **Scenario 3: Seed is deterministic per session**
    *   **Given** the same chart is played twice
    *   **When** both sessions complete
    *   **Then** each submission has a different RNG seed (seeds are unique per session start)

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-021 (score payload), REQ-NET-008 (RNG seed requirement)
*   **Phase**: 8

---

## Leaderboards

### Story ID: US-NET-030 — Query Leaderboard by Chart Hash

**Story Card:**
> **As a** Player  
> **I want** leaderboard retrieval for a specific chart  
> **So that** I can see how my score ranks against others

#### 📝 Description
Make a GET request to `/api/leaderboards/{chart_hash}` with optional query parameters `judge_profile` (filter by judge version) and `limit` (default 10, max 100). The response is a JSON array of entries, each containing `rank`, `player_name`, `score`, `grade`, and `date` (ISO 8601).

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Retrieve top 10 scores**
    *   **Given** a chart hash "abc123..." exists with 50 scores
    *   **When** the leaderboard is queried with no parameters
    *   **Then** the server returns the top 10 scores sorted by score descending, each with rank, player name, score, grade, and date

*   **Scenario 2: Filter by judge profile**
    *   **Given** a chart has scores submitted with both "exceed_v1" and "nx_v1" judge profiles
    *   **When** the leaderboard is queried with `?judge_profile=exceed_v1`
    *   **Then** only scores submitted using "exceed_v1" are returned

*   **Scenario 3: Custom limit**
    *   **Given** a chart has 200 scores
    *   **When** the leaderboard is queried with `?limit=50`
    *   **Then** the top 50 scores are returned

*   **Scenario 4: No scores available**
    *   **Given** a chart hash has zero submitted scores
    *   **When** the leaderboard is queried
    *   **Then** the server returns HTTP 200 with an empty array

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-001, US-NET-020 (chart hash)
*   **Phase**: 9

---

### Story ID: US-NET-031 — Display Leaderboard in Song Select

**Story Card:**
> **As a** Player  
> **I want** the leaderboard visible on the song select screen  
> **So that** I can see top scores before deciding to play a chart

#### 📝 Description
When a chart is highlighted in the song select screen, the leaderboard for that chart is fetched asynchronously and displayed in a sidebar. The display shows rank, player name, and score for the top 10 entries. If the fetch fails or times out, "Leaderboard unavailable" is shown.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Leaderboard loads successfully**
    *   **Given** a chart is selected in the song select screen
    *   **When** the leaderboard request completes within 2 seconds
    *   **Then** the top 10 scores are displayed in a ranked list with player names and scores

*   **Scenario 2: Loading indicator during fetch**
    *   **Given** a leaderboard request is in progress
    *   **When** less than 2 seconds have elapsed
    *   **Then** a "Loading leaderboard..." message or spinner is shown

*   **Scenario 3: Fetch failure fallback**
    *   **Given** the leaderboard request fails with a timeout or HTTP 500
    *   **When** the failure is detected
    *   **Then** "Leaderboard unavailable" is displayed and no error modal appears

*   **Scenario 4: Offline mode shows local scores**
    *   **Given** the player has no internet connection
    *   **When** a chart is selected
    *   **Then** local high scores for that chart are displayed instead of the online leaderboard

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-030, REQ-SCR-004 (song select screen)
*   **Phase**: 9

---

### Story ID: US-NET-032 — Leaderboard Pagination

**Story Card:**
> **As a** Player  
> **I want** pagination controls for viewing scores beyond the top 10  
> **So that** I can browse deeper into the leaderboard

#### 📝 Description
The leaderboard display includes "Next" and "Previous" buttons. Clicking "Next" fetches the next page of 10 scores using an `offset` query parameter. The server supports `?offset=N&limit=10`.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Fetch next page**
    *   **Given** the top 10 scores are displayed
    *   **When** the "Next" button is clicked
    *   **Then** ranks 11–20 are fetched and displayed

*   **Scenario 2: Fetch previous page**
    *   **Given** ranks 11–20 are displayed
    *   **When** the "Previous" button is clicked
    *   **Then** ranks 1–10 are displayed again

*   **Scenario 3: Last page disables Next button**
    *   **Given** a chart has 25 scores and ranks 21–25 are displayed
    *   **When** the page is rendered
    *   **Then** the "Next" button is disabled or hidden

*   **Scenario 4: First page disables Previous button**
    *   **Given** ranks 1–10 are displayed
    *   **When** the page is rendered
    *   **Then** the "Previous" button is disabled or hidden

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-031
*   **Phase**: 9

---

## Replay System

### Story ID: US-NET-040 — Capture Input Snapshot Each Tick

**Story Card:**
> **As a** Developer  
> **I want** input state recorded every tick during gameplay  
> **So that** replays can be reconstructed for anti-cheat verification

#### 📝 Description
During gameplay, each `InputSnapshot` (60 per second) is appended to a replay buffer. The buffer stores tick number and the bitmask of held panels. At the end of the song, the buffer is serialized to a compact binary format and hashed with SHA-256.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Record all input ticks**
    *   **Given** a song lasts 120 seconds
    *   **When** gameplay completes
    *   **Then** the replay buffer contains 7200 input snapshots (120 * 60)

*   **Scenario 2: Correct tick timestamps**
    *   **Given** an input is pressed at tick 1800
    *   **When** the replay is serialized
    *   **Then** the replay data includes tick 1800 with the correct panel bitmask

*   **Scenario 3: Replay hash is deterministic**
    *   **Given** the same song is played twice with identical inputs
    *   **When** both replay hashes are computed
    *   **Then** the hashes are identical

*   **Scenario 4: Memory efficiency**
    *   **Given** a 3-minute song (10800 ticks)
    *   **When** the replay buffer is serialized
    *   **Then** the binary size is no more than 50 KB

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: REQ-INP-002 (input system)
*   **Phase**: 9

---

### Story ID: US-NET-041 — Include Replay Hash in Score Submission

**Story Card:**
> **As a** Developer  
> **I want** replay hashes included in score submissions  
> **So that** the server can verify score authenticity

#### 📝 Description
The `replay_hash` field in the score submission payload is the SHA-256 hash of the binary replay data. The full replay data is not uploaded automatically — only the hash. The full replay binary is saved locally for potential upload if the server flags the score as suspicious.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Hash included in submission**
    *   **Given** a gameplay session completes with a recorded replay
    *   **When** the score is submitted
    *   **Then** the `replay_hash` field contains a 64-character SHA-256 hex string

*   **Scenario 2: Hash matches replay content**
    *   **Given** a replay is recorded and hashed
    *   **When** the replay binary is re-hashed
    *   **Then** the hash matches the value submitted with the score

*   **Scenario 3: Full replay stored locally**
    *   **Given** a score is submitted with a replay hash
    *   **When** the submission completes
    *   **Then** the full replay binary is saved to `~/.local/share/openitup/replays/<replay_hash>.bin`

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-040, US-NET-021 (score payload)
*   **Phase**: 9

---

### Story ID: US-NET-043 — Replay Playback for Verification

**Story Card:**
> **As a** Developer  
> **I want** replay playback functionality in the engine  
> **So that** I can verify replay data produces the claimed score

#### 📝 Description
The engine can load a replay binary, initialize `GameplayScene` with the replay as the input source instead of live input, and play through the song. The resulting score and judgment counts are compared against the submitted values. This is used for manual verification and anti-cheat testing.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Replay produces matching score**
    *   **Given** a replay file is loaded for a score submission of 850000 points
    *   **When** the replay is played back
    *   **Then** the final score is 850000 and all judgment counts match the submission

*   **Scenario 2: Replay divergence detection**
    *   **Given** a replay file has been tampered with
    *   **When** the replay is played back
    *   **Then** the final score does not match the submission and a "Replay verification failed" error is shown

*   **Scenario 3: Replay UI mode**
    *   **Given** a replay is loaded
    *   **When** playback begins
    *   **Then** a "REPLAY MODE" indicator is visible on screen and no live input is accepted

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-040, REQ-JDG-001 (judge logic)
*   **Phase**: 9

---

**Note**: Conditional replay upload (full replay uploaded only when server requests it) deferred pending PO validation. If validated, this will become a separate story covering bandwidth optimization for flagged scores.

---

## Server Implementation

### Story ID: US-NET-050 — API Documentation for Client Validation

**Story Card:**
> **As a** Developer  
> **I want** API documentation that client code can validate against  
> **So that** the client and server implementations remain in sync

#### 📝 Description
Provide an OpenAPI/Swagger specification documenting all server endpoints (`POST /api/auth/register`, `POST /api/auth/login`, `POST /api/auth/refresh`, `POST /api/scores`, `GET /api/leaderboards/{chart_hash}`, `POST /api/replays/{replay_hash}`, `GET /api/replays/{replay_hash}`) with request/response schemas, authentication requirements, and error codes. The client build process validates against this spec to catch breaking changes early.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Specification completeness**
    *   **Given** the OpenAPI spec is generated
    *   **When** each endpoint is checked
    *   **Then** all required endpoints have documented request bodies, response schemas, and error codes

*   **Scenario 2: Authentication requirements documented**
    *   **Given** an endpoint requires authentication
    *   **When** the spec is read
    *   **Then** the `Authorization: Bearer <token>` header requirement is documented

*   **Scenario 3: Error code documentation**
    *   **Given** an endpoint can return 400, 401, 409, and 500
    *   **When** the spec is viewed
    *   **Then** each status code has a description and example response

*   **Scenario 4: Client validation integration**
    *   **Given** the client code makes a request with a missing required field
    *   **When** the validation step runs during build
    *   **Then** the build fails with a clear error pointing to the API spec mismatch

#### 📊 Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: None (can be written before implementation)
*   **Phase**: 8

---

### Story ID: US-NET-051 — Persistent Score Storage

**Story Card:**
> **As a** Developer  
> **I want** persistent score storage so scores survive server restarts  
> **So that** player data is not lost during maintenance or crashes

#### 📝 Description
Implement database persistence with tables: `users` (id, username, email, password_hash, created_at), `sessions` (id, user_id, token_hash, expires_at), `scores` (id, user_id, chart_hash, score, grade, max_combo, judgment counts, judge_profile, replay_hash, timestamp), `replays` (replay_hash as PK, binary_data, uploaded_at). All score submissions and user data must persist to disk and remain available after server restart.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: User creation**
    *   **Given** a registration request is received
    *   **When** the user is created in the database
    *   **Then** a row exists in the `users` table with a bcrypt-hashed password and unique username

*   **Scenario 2: Score insertion**
    *   **Given** a score submission is validated
    *   **When** the score is saved
    *   **Then** a row exists in the `scores` table with all required fields and a foreign key to the `users` table

*   **Scenario 3: Session token storage**
    *   **Given** a login succeeds
    *   **When** a session token is generated
    *   **Then** a row exists in the `sessions` table with the token hash and expiration timestamp 7 days in the future

*   **Scenario 4: Replay storage**
    *   **Given** a replay binary is uploaded
    *   **When** the upload completes
    *   **Then** the binary is stored in the `replays` table keyed by its SHA-256 hash

*   **Scenario 5: Data survives restart**
    *   **Given** 100 scores are submitted and the server is stopped
    *   **When** the server is restarted
    *   **Then** all 100 scores are queryable via the leaderboard API

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-050 (API spec informs schema)
*   **Phase**: 8

---

### Story ID: US-NET-052 — Rate Limiting per Account and IP

**Story Card:**
> **As a** Developer  
> **I want** rate limiting on all server endpoints  
> **So that** abusive clients cannot overwhelm the server

#### 📝 Description
Apply rate limits to prevent abuse and ensure fair resource distribution: 10 requests/minute per IP for unauthenticated endpoints (register, login), 100 requests/minute per account for authenticated endpoints (score submission, leaderboard query). Exceeded requests return HTTP 429 with a `Retry-After` header. Rate limits exist to prevent both accidental abuse (buggy client retry loops) and intentional attacks (brute-force login attempts, score spam).

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Unauthenticated rate limit**
    *   **Given** an IP makes 11 registration requests within 1 minute
    *   **When** the 11th request is received
    *   **Then** the server responds with HTTP 429 and `Retry-After: 60`

*   **Scenario 2: Authenticated rate limit**
    *   **Given** a user submits 101 scores within 1 minute
    *   **When** the 101st request is received
    *   **Then** the server responds with HTTP 429

*   **Scenario 3: Rate limit reset**
    *   **Given** a user hits the rate limit
    *   **When** 60 seconds elapse
    *   **Then** the next request succeeds

*   **Scenario 4: Burst pattern handling**
    *   **Given** a user submits 10 scores in 5 seconds, waits 55 seconds, then submits 90 more in 5 seconds
    *   **When** the 101st score is submitted within the 60-second window
    *   **Then** the server responds with HTTP 429

#### 📊 Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-NET-051 (database for tracking)
*   **Phase**: 9

---

### Story ID: US-NET-053 — Score Validation for Physical Possibility

**Story Card:**
> **As a** Developer  
> **I want** server-side validation that submitted scores are physically possible  
> **So that** blatantly impossible scores are rejected automatically

#### 📝 Description
Validate that max combo does not exceed total note count, judgment counts sum correctly, and score is within the theoretical maximum for the chart and judge profile. Impossible scores return HTTP 400 with an error message. This validation requires the server to have access to chart metadata (note count, theoretical max score) for each judge profile.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Max combo exceeds note count**
    *   **Given** a chart has 400 notes
    *   **When** a score submission claims max combo 500
    *   **Then** the server responds with HTTP 400 and "Max combo exceeds note count"

*   **Scenario 2: Judgment counts do not sum to note count**
    *   **Given** a chart has 400 notes
    *   **When** a submission has judgment counts summing to 380
    *   **Then** the server responds with HTTP 400 and "Judgment counts do not match note count"

*   **Scenario 3: Score exceeds theoretical maximum**
    *   **Given** the theoretical max score for a chart is 1000000 under "exceed_v1"
    *   **When** a submission claims 1100000
    *   **Then** the server responds with HTTP 400 and "Score exceeds theoretical maximum"

*   **Scenario 4: Valid score accepted**
    *   **Given** all validations pass
    *   **When** a score is submitted
    *   **Then** the server responds with HTTP 201 and stores the score

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-051, REQ-CHT-010 (chart hash implies note count), REQ-JDG-001 (scoring formula), server must have judge profile metadata available
*   **Phase**: 9

---

### Story ID: US-NET-054 — Suspicious Score Flagging with Review Mechanism

**Story Card:**
> **As a** Developer  
> **I want** scores with statistically anomalous patterns flagged for manual review  
> **So that** sophisticated cheating attempts can be caught

#### 📝 Description
Scores are flagged if they exhibit statistically anomalous patterns: timing standard deviation impossibly low (< 5ms across all hits), player's skill rating jumped dramatically (e.g., 280000 point improvement in one session), or judgment distribution is highly unusual for the chart difficulty. Flagged scores are stored but marked as "under review" and excluded from leaderboards until verified. Players can appeal flags and admins can manually review and approve/reject.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Impossible timing consistency**
    *   **Given** a submission includes timing error data with standard deviation 2ms
    *   **When** the score is processed
    *   **Then** it is flagged with reason "Impossible timing consistency" and `replay_required: true` is returned to the client

*   **Scenario 2: Large skill jump**
    *   **Given** a player's previous best score was 700000 and they submit 980000
    *   **When** the score is processed
    *   **Then** it is flagged with reason "Unusual skill improvement" and queued for manual review

*   **Scenario 3: Normal score not flagged**
    *   **Given** a submission has typical judgment distribution and timing variation
    *   **When** the score is processed
    *   **Then** it is not flagged and appears on the leaderboard immediately

*   **Scenario 4: Player appeal mechanism**
    *   **Given** a player's score is flagged as "under review"
    *   **When** the player views their submission status
    *   **Then** an "Appeal this flag" option is available that notifies admins for manual review

*   **Scenario 5: Admin review and approval**
    *   **Given** a flagged score is under manual review
    *   **When** an admin approves the score after reviewing the replay
    *   **Then** the flag is removed and the score appears on the leaderboard

#### 📊 Technical Notes & Constraints
*   **Story Points**: 8
*   **Dependencies**: US-NET-053 (score validation)
*   **Phase**: 9

---

## Non-Functional Requirements

### Story ID: US-NET-100 — Offline Mode is Fully Functional

**Story Card:**
> **As a** Player  
> **I want** all core gameplay features available without an internet connection  
> **So that** I can play anytime without network dependency

#### 📝 Description
The engine must detect network unavailability gracefully. All gameplay, local high scores, settings, and profiles work offline. Network features (leaderboards, score submission) degrade silently with no error popups or gameplay interruption. An "Offline" indicator is shown in the UI.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Boot offline**
    *   **Given** the device has no internet connection
    *   **When** the game is launched
    *   **Then** the game boots normally, shows an "Offline" indicator, and all screens are accessible

*   **Scenario 2: Gameplay works offline**
    *   **Given** the game is offline
    *   **When** a song is selected and played
    *   **Then** the song plays, notes are judged, and the score is saved locally

*   **Scenario 3: Local high scores offline**
    *   **Given** the game is offline
    *   **When** the song select screen displays scores
    *   **Then** local high scores are shown (not online leaderboards)

*   **Scenario 4: No network error interruptions**
    *   **Given** the game is offline
    *   **When** gameplay or navigation occurs
    *   **Then** no modal errors or popups related to network issues appear

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-002 (async network service)
*   **Phase**: 8

---

### Story ID: US-NET-101 — Network Operations Never Block Rendering

**Story Card:**
> **As a** Player  
> **I want** network operations to run in the background  
> **So that** slow connections do not cause frame drops or stuttering

#### 📝 Description
All network requests execute on a background thread. The main thread never waits on a network call. Frame time measurements during network activity must not exceed the target frame budget (16.67ms for 60 FPS).

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Frame rate maintained during score submission**
    *   **Given** a score submission is in progress
    *   **When** 100 frames are rendered
    *   **Then** no frame takes longer than 17ms

*   **Scenario 2: Frame rate maintained during leaderboard fetch**
    *   **Given** a leaderboard request is pending
    *   **When** the song select screen is rendered
    *   **Then** the frame rate remains above 59 FPS

*   **Scenario 3: Input responsiveness during network activity**
    *   **Given** multiple network requests are queued
    *   **When** the player presses a key
    *   **Then** the input is registered within 1 frame (16.67ms)

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-002
*   **Phase**: 8

---

### Story ID: US-NET-102 — Progress Indication for Long Operations

**Story Card:**
> **As a** Player  
> **I want** visual feedback when network operations are in progress  
> **So that** I know the game is working and not frozen

#### 📝 Description
Operations expected to take more than 1 second (e.g., initial login, leaderboard fetch, queue retry) display a progress indicator (spinner or progress bar). The indicator disappears when the operation completes or times out.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Login progress indicator**
    *   **Given** a login request is submitted
    *   **When** the request takes 3 seconds
    *   **Then** a spinner is visible next to the "Log In" button while the request is pending

*   **Scenario 2: Leaderboard fetch progress**
    *   **Given** a leaderboard request is made
    *   **When** the request takes 2 seconds
    *   **Then** "Loading leaderboard..." with a spinner is displayed

*   **Scenario 3: Progress removed on completion**
    *   **Given** a progress indicator is showing
    *   **When** the request completes
    *   **Then** the indicator is removed within 1 frame

*   **Scenario 4: Progress removed on timeout**
    *   **Given** a request times out after 30 seconds
    *   **When** the timeout occurs
    *   **Then** the progress indicator is removed and an error message is briefly displayed

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-002
*   **Phase**: 9

---

### Story ID: US-NET-103 — SQL Injection and XSS Protection

**Story Card:**
> **As a** Developer  
> **I want** all database queries parameterized and all user input sanitized  
> **So that** the server is protected against SQL injection and XSS attacks

#### 📝 Description
Use parameterized queries for all database operations. Escape HTML special characters in all user-provided strings (username, player names) before rendering in the web portal. Input validation rejects strings containing SQL keywords or script tags.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: SQL injection attempt blocked**
    *   **Given** a registration request with username "admin'; DROP TABLE users;--"
    *   **When** the registration is processed
    *   **Then** the username is stored as a literal string and no SQL is executed

*   **Scenario 2: XSS attempt blocked**
    *   **Given** a player name contains "<script>alert('xss')</script>"
    *   **When** the profile page is rendered
    *   **Then** the script tag is escaped and displayed as text, not executed

*   **Scenario 3: Input validation rejects dangerous strings**
    *   **Given** a username contains "SELECT * FROM"
    *   **When** the registration form is submitted
    *   **Then** a validation error "Invalid username format" is returned

#### 📊 Technical Notes & Constraints
*   **Story Points**: 2
*   **Dependencies**: US-NET-051 (database), US-NET-060 (web UI)
*   **Phase**: 9

---

## Web Portal (Separate Backlog)

**Note**: The following stories represent a separate web portal application with a different technology stack (likely React/Vue/Angular frontend + REST API backend). These are out-of-scope for the core engine backlog and should be managed as a separate project. They are listed here for reference and to document cross-system dependencies.

---

### Story ID: US-NET-060 — Web Leaderboard Browser

**Story Card:**
> **As a** Player  
> **I want** a web page showing leaderboards for all charts  
> **So that** I can view rankings without launching the game

#### 📝 Description
Create a responsive web page at `/leaderboards` that lists all charts with submitted scores. Clicking a chart shows its leaderboard. The page uses the same `GET /api/leaderboards/{chart_hash}` endpoint as the game client.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Chart list displayed**
    *   **Given** the server has scores for 50 charts
    *   **When** the leaderboards page is loaded
    *   **Then** all 50 charts are listed with title, artist, and submission count

*   **Scenario 2: Chart leaderboard view**
    *   **Given** a chart is selected
    *   **When** the chart detail page loads
    *   **Then** the top 10 scores are displayed with rank, player name, score, grade, and date

*   **Scenario 3: Mobile responsive**
    *   **Given** the page is viewed on a mobile device
    *   **When** the leaderboard is displayed
    *   **Then** the layout adapts and remains readable without horizontal scrolling

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-030 (leaderboard API)
*   **Phase**: 9
*   **Tech Stack**: Out-of-scope for C++ engine, requires separate web frontend project

---

### Story ID: US-NET-061 — Player Profile Page

**Story Card:**
> **As a** Player  
> **I want** a profile page showing my statistics and score history  
> **So that** I can track my progress over time

#### 📝 Description
Create a profile page at `/players/{username}` showing total plays, average grade, top scores, and recent activity. Data is fetched from a new `GET /api/players/{username}/stats` endpoint.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Profile statistics displayed**
    *   **Given** a player has submitted 120 scores
    *   **When** their profile page is loaded
    *   **Then** the page shows total plays: 120, average grade, and grade distribution

*   **Scenario 2: Top scores listed**
    *   **Given** a player has scores across 30 charts
    *   **When** the profile page is loaded
    *   **Then** the player's top 10 scores are listed with chart title, score, grade, and date

*   **Scenario 3: Recent activity**
    *   **Given** a player submitted 5 scores today
    *   **When** the profile page is loaded
    *   **Then** the 5 most recent scores are displayed with timestamps

*   **Scenario 4: Player not found**
    *   **Given** the username "nonexistent" does not exist
    *   **When** `/players/nonexistent` is accessed
    *   **Then** a 404 page with "Player not found" is displayed

#### 📊 Technical Notes & Constraints
*   **Story Points**: 5
*   **Dependencies**: US-NET-060 (web infrastructure)
*   **Phase**: 9
*   **Tech Stack**: Out-of-scope for C++ engine, requires separate web frontend project

---

### Story ID: US-NET-062 — Player Search

**Story Card:**
> **As a** Player  
> **I want** search functionality to find other players by name  
> **So that** I can view their profiles and compare scores

#### 📝 Description
Add a search bar to the web portal that queries `GET /api/players/search?q={query}`. Results are displayed as a list of player names linking to their profile pages.

#### ✅ Acceptance Criteria (Confirmation)

*   **Scenario 1: Partial name match**
    *   **Given** a player named "player123" exists
    *   **When** the search query "play" is submitted
    *   **Then** "player123" appears in the search results

*   **Scenario 2: Case insensitive**
    *   **Given** a player named "Player123" exists
    *   **When** the search query "player123" is submitted
    *   **Then** "Player123" appears in the results

*   **Scenario 3: No results**
    *   **Given** no players match the query "zzzzz"
    *   **When** the search is submitted
    *   **Then** "No players found" is displayed

*   **Scenario 4: Result limit**
    *   **Given** 200 players match the query "player"
    *   **When** the search is submitted
    *   **Then** the first 20 results are displayed with a "Show more" button

#### 📊 Technical Notes & Constraints
*   **Story Points**: 3
*   **Dependencies**: US-NET-061 (profile pages)
*   **Phase**: 9
*   **Tech Stack**: Out-of-scope for C++ engine, requires separate web frontend project

---

## Summary

**Total Stories**: 30 (27 core engine + 3 web portal)  
**By Phase**:
- Phase 8 (Score Submission, Server Prototype): 10 stories
- Phase 9 (Leaderboards, Accounts, Replay System, NFRs): 20 stories

**By Status**:
- PLANNED: 0
- FUTURE: 30
- DONE: 0

**Total Story Points**: 115 (excluding web portal)
- Phase 8: 39 points
- Phase 9: 76 points

**Key Dependencies**:
- All network stories depend on US-NET-001 (HTTP client) and US-NET-002 (async executor)
- Score submission (US-NET-020–023) enables leaderboards (US-NET-030–032)
- Replay system (US-NET-040, 041, 043) supports anti-cheat (US-NET-054)
- Server stories (US-NET-050–054) run in parallel with client-side development
- Web portal stories (US-NET-060–062) are out-of-scope for engine backlog

**Cross-System Dependencies**:
- REQ-DAT-001 (Profile System) — stories US-NET-010, US-NET-011, US-NET-022
- REQ-CHT-010 (Chart Hash) — stories US-NET-020, US-NET-021, US-NET-030, US-NET-053
- REQ-JDG-001 (Judge) — stories US-NET-020, US-NET-021, US-NET-043, US-NET-053
- REQ-INP-002 (Input System) — story US-NET-040
- REQ-NET-008 (RNG Seed & Replay Hash) — stories US-NET-021, US-NET-023

**Removed Stories**:
- US-NET-042 (conditional replay upload) — deferred pending PO validation
