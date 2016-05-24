#! /usr/bin/python2 -u
# -*- coding: utf-8 -*-
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Simple tokenizer for Burmese sentences.
"""

from __future__ import unicode_literals

import codecs
import re
import sys

STDIN = codecs.getreader('utf-8')(sys.stdin)
STDOUT = codecs.getwriter('utf-8')(sys.stdout)
STDERR = codecs.getwriter('utf-8')(sys.stderr)

TOKEN = re.compile(r'''
  (?P<native>[\u1000-\u103F\u104C-\u104F\u1050-\u1059\u200C]+)
| (?P<latin>[A-Za-z]+)
| (?P<number>[\u101D\u1040-\u1049]*[\u1040-\u1049]+[\u101D\u1040-\u1049]*)
| (?P<punct>[:\u104A\u104B\u2018\u2019\u2013\u2014]+)
| (?P<space>[\u0020\u00A0\u200B]+)
| (?P<symbol>[$]+)
''', re.VERBOSE)


def GetToken(match):
  items = [(k, v) for (k, v) in match.groupdict().items() if v is not None]
  assert len(items) == 1
  return items[0]


def main(unused_argv):
  for line in STDIN:
    STDOUT.write('\n%s' % line)
    line = line.rstrip('\n')
    end = 0
    for match in TOKEN.finditer(line):
      if match.start() != end:
        unmatched = line[end:match.start()]
        STDOUT.write('Unmatched: %s\n' % repr(unmatched))
      kind_token = GetToken(match)
      STDOUT.write('token %6s: %s\n' % kind_token)
      end = match.end()
    if end < len(line):
      STDOUT.write('Unmatched: %s\n' % repr(line[end:]))
  return


if __name__ == '__main__':
  main(sys.argv)
