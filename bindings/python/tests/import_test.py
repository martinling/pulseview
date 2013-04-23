#!/usr/bin/env python
# -*- coding: utf-8 -*-
 
'''Test cases for pulseview bindings module.'''
 
import unittest
 
class ImportTest(unittest.TestCase):
 
    def testImport(self):
        import pulseview
 
if __name__ == '__main__':
    unittest.main()
