#!/usr/bin/env python3
import sys

sys.stdout.write("Status: 500 Internal Server Error\r\n")
sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<html><body><h1>500 - CGI Error</h1><p>This CGI script intentionally returns a 500 error.</p></body></html>\r\n")
