#!/usr/bin/env python3
import os
import sys

cookie_header = os.environ.get('HTTP_COOKIE', '')

if "session_id=" in cookie_header:
    status = "Status: 200 OK\r\n"
    cookie_out = "Set-Cookie: session_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0; HttpOnly\r\n"
    body = """<!DOCTYPE html>
<html>
<head><title>Session Cleared</title>
<style>body{font-family:monospace;margin:20px;background:#1a1a2e;color:#eee}h1{color:#4CAF50}a{color:#4CAF50}</style>
</head>
<body>
    <h1>Session Cleared</h1>
    <p>Your session cookie has been removed.</p>
    <p><a href="/">Back to Dashboard</a></p>
</body>
</html>"""
else:
    status = "Status: 400 Bad Request\r\n"
    cookie_out = ""
    body = """<!DOCTYPE html>
<html>
<head><title>No Session</title>
<style>body{font-family:monospace;margin:20px;background:#1a1a2e;color:#eee}h1{color:#e94560}a{color:#e94560}</style>
</head>
<body>
    <h1>No Session Found</h1>
    <p>There is no active session to delete.</p>
    <p><a href="/">Back to Dashboard</a></p>
</body>
</html>"""

body_bytes = body.encode('utf-8')

sys.stdout.write(status)
if cookie_out:
    sys.stdout.write(cookie_out)
sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n")
sys.stdout.write(f"Content-Length: {len(body_bytes)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
