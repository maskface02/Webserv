#!/usr/bin/env python3
import sys

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("This CGI will exit with error\n")
sys.stdout.flush()

sys.exit(1)
