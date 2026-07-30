#!/usr/bin/env python3
import os
import sys
import html
from urllib.parse import parse_qs

qs = os.environ.get("QUERY_STRING", "")
parsed = parse_qs(qs)

sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<html><head><title>Query String Test</title>\r\n")
sys.stdout.write("<style>body{font-family:monospace;margin:20px}table{border-collapse:collapse}th,td{border:1px solid #ddd;padding:8px}th{background:#FF9800;color:white}</style>\r\n")
sys.stdout.write("</head><body>\r\n")
sys.stdout.write("<h1>Query String Parameters</h1>\r\n")
sys.stdout.write(f"<p><b>QUERY_STRING:</b> {html.escape(qs)}</p>\r\n")
sys.stdout.write("<table><tr><th>Parameter</th><th>Value</th></tr>\r\n")
if parsed:
    for k, v in parsed.items():
        sys.stdout.write(f"<tr><td>{html.escape(k)}</td><td>{html.escape(v[0])}</td></tr>\r\n")
else:
    sys.stdout.write("<tr><td colspan='2'>No query parameters</td></tr>\r\n")
sys.stdout.write("</table>\r\n")
sys.stdout.write("<p>Try: <a href='/cgi/query_test.py?name=webserv&lang=C++&port=8080'>/cgi/query_test.py?name=webserv&lang=C++&port=8080</a></p>\r\n")
sys.stdout.write("</body></html>\r\n")
