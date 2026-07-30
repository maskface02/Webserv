#!/usr/bin/env python3
import os
import sys
import html

sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<html><head><title>CGI Environment Variables</title>\r\n")
sys.stdout.write("<style>body{font-family:monospace;margin:20px}table{border-collapse:collapse;width:100%}th,td{border:1px solid #ddd;padding:8px;text-align:left}th{background:#4CAF50;color:white}tr:nth-child(even){background:#f2f2f2}</style>\r\n")
sys.stdout.write("</head><body>\r\n")
sys.stdout.write("<h1>CGI Environment Variables</h1>\r\n")
sys.stdout.write("<table><tr><th>Variable</th><th>Value</th></tr>\r\n")
for key in sorted(os.environ.keys()):
    sys.stdout.write(f"<tr><td>{html.escape(key)}</td><td>{html.escape(os.environ[key])}</td></tr>\r\n")
sys.stdout.write("</table></body></html>\r\n")
