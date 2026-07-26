#!/usr/bin/env python3
import os

# 1. Read environment variables set by your C++ server
cookie_header = os.environ.get('HTTP_COOKIE', '')
counter = os.environ.get('SESSION_COUNTER', '0')

# 2. Check if "session_id=" is present in the cookie string
if "session_id=" in cookie_header:
    # ACCESS GRANTED
    body = f"""<!DOCTYPE html>
<html>
<body>
    <h1>Access Granted!</h1>
    <p>Session Counter: {counter}</p>

    <!-- Logout Button -->
    <form action="/logout" method="POST">
        <button type="submit">Clear Session</button>
    </form>
</body>
</html>"""
    
    status = "Status: 200 \r\n"

else:
    # ACCESS DENIED
    body = """<!DOCTYPE html>
<html>
<body>
    <h1>403 Forbidden - Access Denied</h1>
</body>
</html>"""
    
    status = "Status: 403 Forbidden\r\n"

# 3. Print CGI Headers (separated from body by \r\n\r\n)
print(status, end="")
print("Content-Type: text/html\r\n", end="")
print(f"Content-Length: {len(body.encode('utf-8'))}\r\n", end="")
print("\r\n", end="")  # Empty line signals end of headers

# 4. Print Response Body
print(body, end="")