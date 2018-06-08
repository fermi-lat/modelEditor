# -*- python -*-
#
# $Id: SConscript,v 1.10 2013/12/21 00:06:37 giacomov Exp $
# Authors: Giacomo Vianello <giacomov@slac.stanford.edu>
# Version: pyBurstAnalysisGUI-01-04-03

Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()

progEnv.Tool('registerTargets', package = 'modelEditor', 
             data = (listFiles(['*.xsd'])),
             python = (listFiles(['*.py'])))
