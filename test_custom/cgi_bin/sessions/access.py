#!/usr/bin/env python3
import os
import sys

cookie_header = os.environ.get('HTTP_COOKIE', '')
counter = os.environ.get('SESSION_COUNTER', '0')

if "session_id=" in cookie_header:
    body = f"""<!DOCTYPE html>
<html>
<head><title>Access Granted</title>
<style>body{{font-family:monospace;margin:20px;background:#1a1a2e;color:#eee}}h1{{color:#4CAF50}}.info{{background:#0f3460;padding:15px;border-radius:5px;margin:10px 0}}button{{background:#e94560;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px}}</style>
</head>
<body>
    <h1>Access Granted!</h1>
    <div class="info">
        <p><b>Session Counter:</b> {counter}</p>
        <p><b>Session ID:</b> {cookie_header.split('session_id=')[1].split(';')[0] if 'session_id=' in cookie_header else 'N/A'}</p>
    </div>
    <form action="/sessions/logout.py" method="POST">
        <button type="submit">Clear Session</button>
    </form>
    <p><a href="/">Back to Dashboard</a></p>
</body>
</html>"""
    status = "Status: 200 OK\r\n"
else:
    body = """<!DOCTYPE html>
<html>
<head><title>Access Denied</title>
<style>body{font-family:monospace;margin:20px;background:#1a1a2e;color:#eee}h1{color:#e94560}a{color:#e94560}</style>
</head>
<body>
    <h1>403 Forbidden - Access Denied</h1>
    <p>No active session found. Please visit the main page first to get a session cookie.</p>
    <p><a href="/">Go to Dashboard</a></p>
</body>
</html>"""
    status = "Status: 403 Forbidden\r\n"

sys.stdout.write(status)
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write(f"Content-Length: {len(body.encode('utf-8'))}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
