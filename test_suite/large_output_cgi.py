#!/usr/bin/env python3
import sys

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")

for i in range(1000):
    sys.stdout.write("Line %d: " % i + "x" * 80 + "\n")

sys.stdout.flush()
