# Network System Requirements

## REQ-NET-001: Client-Server Architecture
**Status**: [FUTURE]  
**Priority**: Could Have

The network system must follow a client-server architecture where the server is a separate project and the engine includes a thin HTTP client layer.

**Acceptance Criteria**:
- Client makes HTTP/HTTPS requests to server
- Server is separate codebase (Rust or Go)
- Client and server versioned independently
- Client works offline (network features optional)

**Dependencies**: REQ-ENG-002  
**Source**: Roadmap subsystem 10, Phase 8-9, architecture decisions

---

## REQ-NET-002: Score Submission
**Status**: [FUTURE]  
**Priority**: Could Have

After completing a song, the client must post score, chart hash, judge profile version, and replay hash to the server.

**Acceptance Criteria**:
- POST request with score data JSON
- Includes: score, grade, max combo, judgment counts
- Includes: chart hash, judge profile identifier
- Includes: replay hash for anti-cheat verification
- Authentication token included in request

**Dependencies**: REQ-NET-001, REQ-DAT-005, REQ-CHT-010  
**Source**: Roadmap subsystem 10, Phase 8

---

## REQ-NET-003: Leaderboard Query
**Status**: [FUTURE]  
**Priority**: Could Have

The client must query leaderboards per chart hash, retrieving top scores with player names and dates.

**Acceptance Criteria**:
- GET request with chart hash parameter
- Returns top N scores (10-100 configurable)
- Each entry: player name, score, grade, date
- Supports filtering by judge profile
- Pagination for large leaderboards

**Dependencies**: REQ-NET-001, REQ-CHT-010  
**Source**: Roadmap subsystem 10, Phase 9

---

## REQ-NET-004: Account Authentication
**Status**: [FUTURE]  
**Priority**: Could Have

The network system must support username/password authentication with token-based sessions.

**Acceptance Criteria**:
- Register account: username, email, password
- Login: username/password returns session token
- Token stored in player profile
- Token included in authenticated requests
- Token refresh mechanism for long sessions

**Dependencies**: REQ-NET-001, REQ-DAT-001  
**Source**: Roadmap subsystem 10, Phase 9

---

## REQ-NET-005: Asynchronous Network Operations
**Status**: [FUTURE]  
**Priority**: Could Have

All network operations must run asynchronously on background threads with callbacks, never blocking the game.

**Acceptance Criteria**:
- Network requests do not block rendering or input
- Callbacks invoked on main thread for UI updates
- Timeout after 30 seconds for requests
- User can cancel in-progress requests
- Progress indication for long operations

**Dependencies**: REQ-NET-001  
**Source**: Roadmap subsystem 10

---

## REQ-NET-006: Failed Submission Queue
**Status**: [FUTURE]  
**Priority**: Could Have

Failed score submissions must be queued locally and retried on next successful connection.

**Acceptance Criteria**:
- Failed submissions saved to disk queue
- Queue retried on next network availability
- Queue limited to reasonable size (100 entries)
- User notified of pending submissions
- Duplicate submissions prevented via request ID

**Dependencies**: REQ-NET-002, REQ-DAT-001  
**Source**: Roadmap subsystem 10

---

## REQ-NET-007: Fully Functional Offline Mode
**Status**: [FUTURE]  
**Priority**: Must Have

The game must be fully functional offline with no network dependency for core gameplay.

**Acceptance Criteria**:
- All gameplay works without internet connection
- Local high scores work offline
- Network features degrade gracefully when offline
- No network-related errors interrupt gameplay
- Offline mode clearly indicated in UI

**Dependencies**: REQ-NET-001  
**Source**: Roadmap subsystem 10, architecture decisions

---

## REQ-NET-008: Replay Data Capture
**Status**: [FUTURE]  
**Priority**: Could Have

The engine must capture replay data during gameplay for upload with scores for anti-cheat verification.

**Acceptance Criteria**:
- Records input snapshot each tick
- Records RNG seeds if any randomness used
- Compact binary format for efficient storage/transmission
- Replay hash (SHA-256) included with score submission
- Replay playback functionality for verification

**Dependencies**: REQ-INP-002, REQ-JDG-001  
**Source**: Roadmap subsystem 10, Phase 9

---

## REQ-NET-009: Server REST API
**Status**: [FUTURE]  
**Priority**: Could Have

The server must provide REST API endpoints for score submission, leaderboard query, account management, and replay verification.

**Acceptance Criteria**:
- POST /api/scores - submit score
- GET /api/leaderboards/{chart_hash} - get leaderboard
- POST /api/auth/register - register account
- POST /api/auth/login - login
- GET /api/replays/{replay_hash} - download replay
- OpenAPI/Swagger documentation

**Dependencies**: REQ-NET-001  
**Source**: Roadmap subsystem 10, Phase 8-9

---

## REQ-NET-010: Server Security and Anti-Cheat
**Status**: [FUTURE]  
**Priority**: Could Have

The server must implement rate limiting, input validation, and anti-cheat verification.

**Acceptance Criteria**:
- Rate limiting per account and IP
- Score submissions validated for physical possibility
- Replay verification against submitted scores
- Suspicious scores flagged for manual review
- SQL injection and XSS protection

**Dependencies**: REQ-NET-009  
**Source**: Roadmap subsystem 10, Phase 9

---

## REQ-NET-011: Web Portal for Leaderboards
**Status**: [FUTURE]  
**Priority**: Could Have

A web portal must allow browsing leaderboards and profiles outside the game.

**Acceptance Criteria**:
- Browse leaderboards by song/chart
- Search for players by name
- View player profiles and statistics
- View replay playback (if implemented)
- Responsive design for mobile

**Dependencies**: REQ-NET-009  
**Source**: Roadmap subsystem 10, Phase 9
