#!/usr/bin/env python3
import sys
import time

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("Starting slow CGI...\n")
sys.stdout.flush()

for i in range(3):
    time.sleep(1)
    sys.stdout.write("Processing step %d/3...\n" % (i + 1))
    sys.stdout.flush()

sys.stdout.write("Slow CGI completed successfully\n")
sys.stdout.flush()
