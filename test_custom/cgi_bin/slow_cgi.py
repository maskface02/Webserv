#!/usr/bin/env python3
import time
import sys

sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<html><body>\r\n")
sys.stdout.write("<h1>Slow CGI Script</h1>\r\n")
sys.stdout.write("<p>Starting 30 second sleep...</p>\r\n")
sys.stdout.flush()
time.sleep(30)
sys.stdout.write("<p>Done sleeping!</p>\r\n")
sys.stdout.write("</body></html>\r\n")
