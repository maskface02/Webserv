#!/usr/bin/env python3
import sys

sys.stdout.write("Status: 404 Not Found\r\n")
sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.write("<h1>Resource Created via CGI</h1>")

































# #!/usr/bin/env python3

# # INVALID: Division by zero causes Python to output a Traceback, which is not valid CGI
# print("Status: 200 OK")
# x = 1 / 0  # ZeroDivisionError occurs before Content-Type or header delimiter
# print("Content-Type: text/html\n")


# what sould happen ? => 502 invalid script output| 500 crach but is not server problem it is from the script