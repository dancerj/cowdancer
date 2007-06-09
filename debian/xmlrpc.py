#!/usr/bin/python
import xmlrpclib
import sys
srcwiki = xmlrpclib.ServerProxy("http://wiki.debian.org/?action=xmlrpc2")
print srcwiki.getPage(sys.argv[1])
