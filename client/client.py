#!/usr/bin/python2.7
"""Read the json output from the LDR2ethernet"""
__author__ = 'Jan KLopper (jan@underdark.nl)'
__version__ = 0.1

ALLOWEDVERSIONS = (1,)

#standard modules
import urllib
import simplejson

def fetchData(ip, port):
  """Parse the json, check the version and output the hitcount"""
  try:
    data = urllib.urlopen('http://%s:%d/json' % (ip, port)).read()
  except IOError:
    print 'Cannot connect to device, wrong IP?'
  json = simplejson.loads(data)
  if json['version'] not in ALLOWEDVERSIONS:
    print 'invalid protocol version'
    return False
  return int(json['hitcount'])

def main():
  """Processes commandline input to read from the ethernet module."""
  import optparse
  parser = optparse.OptionParser()
  parser.add_option('--host', default='192.168.178.210',
                    help='IP of the LDR2Ethernet device.')
  parser.add_option('-p', '--port', type='int', default=80,
                    help='Port of the LDR2Ethernet device.')
  options, arguments = parser.parse_args()
  print fetchData(options.host, options.port)
