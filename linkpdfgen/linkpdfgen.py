#! /usr/bin/python
# at Wed May 22 12:34:59 CEST 2013

"""Generate a small PDF file containing a single hyperlink."""

__author__ = 'pts@fazekas.hu (Peter Szabo)'

import re
import sys

def gen_pdf(url):
  url_escaped = re.sub(r'[\\()]', lambda match: '\\' + match.group(), url)
  return """%%PDF-1.4
1 0 obj <</Length 58>>
stream
q 0 .5 0 rg 1 0 0 1 50 50 cm 0 0 m 200 100 l 0 200 l h f Q
endstream
endobj
2 0 obj <</Type/Page/Contents 1 0 R/MediaBox[0 0 300 300]/Resources
4 0 R/Parent 6 0 R/Annots[3 0 R]>> endobj
4 0 obj <</ProcSet[/PDF]>> endobj
6 0 obj <</Type/Pages/Count 1/Kids[2 0 R]>> endobj
5 0 obj <</Type/Catalog/Pages 6 0 R>> endobj
3 0 obj <</Type/Annot/Border[0 0 0]/H/I/C[0 1 1]/Rect[50 50 250 250]
/Subtype/Link/A<</Type/Action/S/URI/URI
(%s)>>>> endobj
xref
0 7
0000000000 65535 f 
0000000009 00000 n 
0000000115 00000 n 
0000000355 00000 n 
0000000225 00000 n 
0000000310 00000 n 
0000000259 00000 n 
trailer
<</Size 7/Root 5 0 R>>
startxref
%d
%%%%EOF\n""" % (url_escaped, 478 + len(url_escaped))


def main(argv):
  if len(argv) not in (2, 3):
    print >>sys.stderr, 'Usage: %s <url> <output.pdf>' % argv[0]
    return 1
  pdf_data = gen_pdf(argv[1])
  if len(argv) == 2:
    sys.stdout.write(pdf_data)
  else:
    f = open(argv[2], 'wb')
    try:  # Compatible with Python 2.4
      f.write(pdf_data)
    finally:
      f.close()


if __name__ == '__main__':
  sys.exit(main(sys.argv))
