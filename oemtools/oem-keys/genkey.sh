#!/bin/sh

openssl genrsa -F4 -out oem.key 2048 2>/dev/null

openssl req -batch -new -x509 -key oem.key -out oem.crt

