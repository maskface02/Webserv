#!/usr/bin/env python3
import os
import sys

sys.stdout.write("Content-Type: text/plain\r\n\r\n")
for key in sorted(os.environ.keys()):
    sys.stdout.write(f"{key}={os.environ[key]}\n")
sys.stdout.flush()
