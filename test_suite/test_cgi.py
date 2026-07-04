#!/usr/bin/env python3
import os
import sys

sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")

sys.stdout.write("<html><body>\r\n")
sys.stdout.write("<h1>CGI Test Script</h1>\r\n")
sys.stdout.write("<p>Method: " + os.environ.get('REQUEST_METHOD', 'UNKNOWN') + "</p>\r\n")
sys.stdout.write("<p>Query String: " + os.environ.get('QUERY_STRING', '') + "</p>\r\n")
sys.stdout.write("<p>Script Name: " + os.environ.get('SCRIPT_NAME', '') + "</p>\r\n")
sys.stdout.write("<p>Server Name: " + os.environ.get('SERVER_NAME', '') + "</p>\r\n")
sys.stdout.write("<p>Server Port: " + os.environ.get('SERVER_PORT', '') + "</p>\r\n")

if os.environ.get('REQUEST_METHOD') == 'POST':
    sys.stdout.write("<h2>POST Data:</h2>\r\n")
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        sys.stdout.write("<pre>" + post_data + "</pre>\r\n")

sys.stdout.write("</body></html>\r\n")
sys.stdout.flush()
