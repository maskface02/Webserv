#!/usr/bin/env python3
import sys
import os

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")

expected_vars = [
    'GATEWAY_INTERFACE',
    'SERVER_PROTOCOL',
    'SERVER_NAME',
    'SERVER_PORT',
    'REQUEST_METHOD',
    'SCRIPT_NAME',
    'QUERY_STRING',
    'REMOTE_ADDR',
    'CONTENT_TYPE',
    'CONTENT_LENGTH',
    'PATH_INFO',
    'SERVER_SOFTWARE'
]

sys.stdout.write("CGI Environment Variables Test\n")
sys.stdout.write("=" * 50 + "\n\n")

for var in expected_vars:
    value = os.environ.get(var, 'NOT SET')
    sys.stdout.write("%s: %s\n" % (var, value))

sys.stdout.write("\n" + "=" * 50 + "\n")
sys.stdout.write("Test completed\n")
sys.stdout.flush()
