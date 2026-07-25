#!/usr/bin/env python3
import os

# 1. Read the HTTP_COOKIE environment variable passed by the C++ server
cookie_header = os.environ.get('HTTP_COOKIE', '')

# 2. Check if a session cookie actually exists in the request
if "session_id=" in cookie_header:
    # SUCCESS: Session found -> Expire it
    status = "Status: 200 OK\r\n"
    cookie_header_out = "Set-Cookie: session_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0; HttpOnly\r\n"
    body = """<!DOCTYPE html>
<html>
<body>
    <h1>Session Cleared</h1>
    <p>Your session cookie has been removed from your browser.</p>
</body>
</html>"""

else:
    # FAILURE: No session cookie present
    status = "Status: 400 Bad Request\r\n"
    cookie_header_out = ""  # No cookie to delete
    body = """<!DOCTYPE html>
<html>
<body>
    <h1>No Session Found</h1>
    <p>There is no active session stored to delete.</p>
</body>
</html>"""

# Convert body string to bytes for accurate Content-Length calculation
body_bytes = body.encode('utf-8')

# 3. Print CGI/1.1 Headers terminated by \r\n
print(status, end="")
if cookie_header_out:
    print(cookie_header_out, end="")
print("Content-Type: text/html; charset=utf-8\r\n", end="")
print(f"Content-Length: {len(body_bytes)}\r\n", end="")
print("\r\n", end="")  # Empty line (\r\n) signals end of HTTP headers

# 4. Print Response Body
print(body, end="")