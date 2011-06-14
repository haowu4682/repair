#! /bin/sh
# http_load-12mar2006/http_load -verbose -timeout 40 -parallel 10 -seconds 5 http_load-12mar2006/url.1k | tee results.log

http_load-12mar2006/http_load -verbose -timeout 40 -parallel 10 -seconds 60 http_load-12mar2006/url.1k | tee results.log
