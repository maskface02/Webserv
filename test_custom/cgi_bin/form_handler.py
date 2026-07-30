#!/usr/bin/env python3
import os
import sys
import html
from urllib.parse import parse_qs

method = os.environ.get("REQUEST_METHOD", "GET")
content_type = os.environ.get("CONTENT_TYPE", "")

body = ""
if method == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    raw = sys.stdin.read(content_length)
    if "application/x-www-form-urlencoded" in content_type:
        parsed = parse_qs(raw)
        parts = []
        for k, v in parsed.items():
            parts.append(f"<tr><td>{html.escape(k)}</td><td>{html.escape(v[0])}</td></tr>")
        body = "".join(parts)
    else:
        body = f"<tr><td colspan='2'><pre>{html.escape(raw)}</pre></td></tr>"
else:
    qs = os.environ.get("QUERY_STRING", "")
    parsed = parse_qs(qs)
    parts = []
    for k, v in parsed.items():
        parts.append(f"<tr><td>{html.escape(k)}</td><td>{html.escape(v[0])}</td></tr>")
    body = "".join(parts) if parts else "<tr><td colspan='2'>No data</td></tr>"

sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<html><head><title>Form Handler</title>\r\n")
sys.stdout.write("<style>body{font-family:monospace;margin:20px}table{border-collapse:collapse}th,td{border:1px solid #ddd;padding:8px}th{background:#2196F3;color:white}</style>\r\n")
sys.stdout.write("</head><body>\r\n")
sys.stdout.write(f"<h1>Form Data (Method: {html.escape(method)})</h1>\r\n")
sys.stdout.write("<table><tr><th>Field</th><th>Value</th></tr>\r\n")
sys.stdout.write(body + "\r\n")
sys.stdout.write("</table>\r\n")
sys.stdout.write("<p><a href='/cgi/form_handler.py'>Back</a></p>\r\n")
sys.stdout.write("</body></html>\r\n")
