#!/usr/bin/python

#-------------------------------------------------------------------------------------------------------------
# Copyright (c) 2016, Dolby Laboratories Inc.
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification, are permitted
# provided that the following conditions are met:

# 1. Redistributions of source code must retain the above copyright notice, this list of conditions
#    and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
#    and the following disclaimer in the documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
#    promote products derived from this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#-------------------------------------------------------------------------------------------------------------

import subprocess
import os
import glob
import string
import filecmp
import sys

# Test script for frame337 framer tool
#
# To run tests use no arguments i.e. python run_test.py
# To create the full set of sources that the test uses or generate the reference files from a reference executable
# use either 'sources' or 'references' as a single argument
# From a minimal set of test signals the 'sources' option is used first to complete creation of the source files
# Next the references have to be generated using the 'references' option
# No optional modeules are needed
# Note for either of these modes ref_frame337 must be set to a reference executable path
# Results are sent to standard output

# Generate additional test assets
tools_dir = '../Release/'
os_dir = subprocess.check_output('uname', shell=True)
ref_frame337 = 'ref_bin/' + os_dir.strip() + '/smpte.exe'
ref_frame337_ac4 =  'ref_bin/' + os_dir.strip() + '/smpte_app_lin64_ac4'
dut_frame337 = tools_dir + os_dir.strip() + '/frame337'

# Enums

class Op_modes():
		test = 1
		references = 2
		sources = 3

def process_files(arguments, input_dir, input_ext, tool_exec, output_dir, output_ext):

	for input_file in glob.glob(input_dir + '/*' + input_ext):
		file_stem = os.path.splitext(os.path.basename(input_file))[0]
		output_file_name = output_dir + '/' + file_stem + output_ext
		proc_file_output = subprocess.check_output( tool_exec + ' ' + arguments + ' -i' + input_file + ' -o' + output_file_name , stderr=subprocess.STDOUT,shell=True)
		print proc_file_output
		if (not(os.path.isfile(output_file_name)) and (os.path.getsize(output_file_name) > 0)):
			print "Creation of %s failed" % output_file_name
			return
		print "Created %s" % output_file_name
	

class Tester:
	'Base Testing Class'

	def __init__(self, init_op_mode):
		self.test_id = 1
		self.passed = 0
		self.failed = 0
		self.reference_mode = 0
		self.op_mode = init_op_mode
		if (self.op_mode == Op_modes.references):
			self.f = open('run_test_cases.txt', 'w')


	def __del__(self):
		if 	self.op_mode == Op_modes.references:
			self.f.close()

	def create_test_cases(self, arguments, input_file, output_ext):
		file_stem = os.path.splitext(os.path.basename(input_file))[0]
		ref_output_file_name = 'reference_output/tid' + (str(self.test_id)).zfill(3) + '_' + file_stem + output_ext
		if '.ac4' in output_ext or '.ac4' in input_file:
			cmd = ref_frame337_ac4 + ' ' + arguments + ' -i' + input_file + ' -o' + ref_output_file_name
		else:
			cmd = ref_frame337 + ' ' + arguments + ' -i' + input_file + ' -o' + ref_output_file_name
		print "Reference output cmd: " + cmd
		ref_test_output = subprocess.check_output(cmd , stderr=subprocess.STDOUT,shell=True)
		print ref_test_output
		self.f.write(input_file + ' ' + ref_output_file_name + ' ' + arguments + '\n')
		# ref AC-4 smpte tool does not support PCM input so create a test case using the PCM input signal
		# but the same reference output file as from the corresponding .wav file, since the results
		# should be identical
		if '.ac4' in output_ext and '.wav' in input_file:
			input_file_pcm = input_file.replace('ac4_wav', 'ac4_pcm')
			input_file_pcm = input_file_pcm.replace('.wav', '.pcm')
			self.f.write(input_file_pcm + ' ' + ref_output_file_name + ' ' + arguments + ' -b16' + '\n')
		self.test_id += 1

	def run_test_cases(self):
		with open('run_test_cases.txt', 'r') as f:
			for line in f:
				line_words = line.split()
				if (len(line_words) > 1):
					input_file = line_words[0]
					file_stem = os.path.splitext(os.path.basename(input_file))[0]
					ref_output_file_name = line_words[1]
					if len(line_words) > 2:
						arguments = " ".join(line_words[2:len(line_words)])
					else:
						arguments = ''
					dut_output_file_name = 'dut_output/tid' + (str(self.test_id)).zfill(3) + '_' + file_stem + os.path.splitext(ref_output_file_name)[1]
					cmd = dut_frame337 + ' ' + arguments + ' -i' + input_file + ' -o' + dut_output_file_name
					print "DUT cmd: " + cmd
					dut_test_output = subprocess.check_output(cmd , stderr=subprocess.STDOUT, shell=True)
					print dut_test_output
					if(filecmp.cmp(ref_output_file_name, dut_output_file_name)):
						print input_file + " -> " + dut_output_file_name + " Passed"
						self.passed += 1
					else:
						print input_file + " -> " + dut_output_file_name + " Failed"
						self.failed += 1
					self.test_id += 1
				else:
					print >> sys.stderr, 'Test cases file is truncated at line: ' + line

	def run_error_case(self, arguments, input_file, output_ext):
		file_stem = os.path.splitext(os.path.basename(input_file))[0]
		dut_output_file_name = 'dut_output/tid' + (str(self.test_id)).zfill(3) + '_' + file_stem + output_ext
		cmd = dut_frame337 + ' ' + arguments + ' -i' + input_file + ' -o' + dut_output_file_name
		print "DUT cmd: " + cmd
		return_code = subprocess.call(cmd , stderr=subprocess.STDOUT, shell=True)
		if ((return_code != 0 ) and (not(os.path.isfile(dut_output_file_name)) or (os.path.getsize(dut_output_file_name) == 0))):
			print "Error case " + input_file + " -> " + dut_output_file_name + " Passed"
			self.passed += 1
		else:
			print "Error case "+ input_file + " -> " + dut_output_file_name + " Failed"
			self.failed += 1
		self.test_id += 1

	def print_report(self):
		print "Number of tests completed: " + str(self.test_id - 1)
		print "Number of tests passed: " + str(self.passed)
		print "Number of tests failed: " + str(self.failed)

def main():
	# Resources
	os_dir = subprocess.check_output('uname', shell=True)
	dde_wav = 'sources/dde_wav'
	dde_es = 'sources/dde_es'
	dde_pcm = 'sources/dde_pcm'
	dde_pcm32 = 'sources/dde_pcm32'

	dd_wav = 'sources/dd_wav'
	dd_es = 'sources/dd_es'
	dd_pcm = 'sources/dd_pcm'

	ddplus_wav = 'sources/ddplus_wav'
	ddplus_es = 'sources/ddplus_es'
	ddplus_pcm = 'sources/ddplus_pcm'
    
	ac4_wav = 'sources/ac4_wav'
	ac4_es = 'sources/ac4_es'
	ac4_pcm = 'sources/ac4_pcm'
    
	error_es = 'sources/error_es'

	# Establish consistent mode of operation
	if ((len(sys.argv)) > 1):
		if (sys.argv[1] == 'references'):
			op_mode = Op_modes.references
		elif (sys.argv[1] == 'sources'):
			op_mode = Op_modes.sources
		else:
			op_mode = Op_modes.test
	else:
		op_mode = Op_modes.test

	Tester1 = Tester(op_mode)

	# For generating references
	# This section also generates the test case file
	if (op_mode == Op_modes.references):
		print "Generating References"
		os.system( 'rm -fR reference_output' )
		os.system( 'mkdir -p reference_output' )

		dde_es_files = glob.glob(dde_es + '/*.dde')
		dde_wav_files = glob.glob(dde_wav + '/*.wav')
		dde_pcm_files = glob.glob(dde_pcm + '/*.pcm')
		dde_pcm32_files = glob.glob(dde_pcm32 + '/*.pcm')
		dd_es_files = glob.glob(dd_es + '/*.ac3')
		dd_wav_files = glob.glob(dd_wav + '/*.wav')
		dd_pcm_files = glob.glob(dd_pcm + '/*.pcm')
		ddplus_es_files = glob.glob(ddplus_es + '/*.ec3')
		ddplus_wav_files = glob.glob(ddplus_wav + '/*.wav')
		ddplus_pcm_files = glob.glob(ddplus_pcm + '/*.pcm')
		ac4_es_files = glob.glob(ac4_es + '/*.ac4')
		ac4_wav_files = glob.glob(ac4_wav + '/*.wav')
		ac4_pcm_files = glob.glob(ac4_pcm + '/*.pcm')

		mss_ddplus_wav_files = '3_stream_640.wav', '6ch_main_1ch_ad.wav', '6ch_main_2ch_ad.wav', '6ch_substream.wav', '8ch_7.1_standard.wav', 'ChID_voices_71_448_ddp_joc.wav', 'ChID_voices_71_640_ddp_joc.wav'
		mss_ddplus_wav_files = [ddplus_wav + '/' + s for s in mss_ddplus_wav_files]

		# Create reference cases
		print "Basic Dolby E formatting"
		for input_file in dde_es_files:
			Tester1.create_test_cases('', input_file, '.wav')
		print "Basic Dolby E deformatting"
		for input_file in dde_wav_files:
			Tester1.create_test_cases('-d', input_file, '.dde')
		print "Basic DD formatting"
		for input_file in dd_es_files:
			Tester1.create_test_cases('', input_file, '.wav')	
		print "Basic DD deformatting"
		for input_file in dd_wav_files:
			Tester1.create_test_cases('-d', input_file, '.ac3')	
		print "Basic DD+ formatting"
		for input_file in ddplus_es_files:
			Tester1.create_test_cases('', input_file, '.wav')	
		print "Basic DD+ deformatting"
		for input_file in ddplus_wav_files:
			Tester1.create_test_cases('-d', input_file, '.ec3')
		print "DD alternate packing method"
		for input_file in dd_es_files:
			Tester1.create_test_cases('-a', input_file, '.wav')
		print "Deformatting from PCM (24bit or 32bit for Dolby E, 16bit for DD/DD+)"
		for input_file in dde_pcm_files:
			Tester1.create_test_cases('-d -b24', input_file, '.dde')
		for input_file in dde_pcm32_files:
			Tester1.create_test_cases('-d -b32', input_file, '.dde')
		for input_file in dd_pcm_files:
			Tester1.create_test_cases('-d -b16', input_file, '.ac3')
		for input_file in ddplus_pcm_files:
			Tester1.create_test_cases('-d -b16', input_file, '.ec3')
		print "Basic AC-4 formatting"
		for input_file in ac4_es_files:
			Tester1.create_test_cases('', input_file, '.wav')	
		print "Basic AC-4 deformatting"
		for input_file in ac4_wav_files:
			Tester1.create_test_cases('-d', input_file, '.ac4')
		return(0)

	# For generating sources
	if (op_mode == Op_modes.sources):
		print "Generating source files"
		os.system( 'rm -fR ' + dde_es + ' ' + dde_wav + ' ' + dd_es + ' ' + dd_wav + ' ' + ddplus_es + ' ' + ddplus_wav )
		os.system( 'mkdir -p ' + dde_es + ' ' + dde_wav + ' ' + dd_es + ' ' + dd_wav + ' ' + ddplus_es + ' ' + ddplus_wav )
		print "Generating source pcm files"
		process_files('-d -b24', dde_pcm, '.pcm', ref_frame337, dde_es, '.dde')
		process_files('-d -b16', dd_pcm, '.pcm', ref_frame337, dd_es, '.ac3')
		process_files('-d -b16', ddplus_pcm, '.pcm', ref_frame337, ddplus_es, '.ec3')

		print "Generating source wave files"
		process_files('', dde_es, '.dde', ref_frame337, dde_wav, '.wav')
		process_files('', dd_es, '.ac3', ref_frame337, dd_wav, '.wav')
		process_files('', ddplus_es, '.ec3', ref_frame337, ddplus_wav, '.wav')
		return(0)

	# If we got here we are running tests
	#assert this is the case and bail if not
	if (op_mode != Op_modes.test):
		return(0)

	# Remove device under test output files
	print "Clean dut output directory"
	os.system( 'rm -fR dut_output' )
	os.system( 'mkdir -p dut_output' )

	Tester1.run_test_cases();

	error_es_files = glob.glob(error_es + '/*.*')
	# Data rate too high error case
	# Unknown ES error case
	for input_file in error_es_files:
		Tester1.run_error_case('', input_file, '.wav')

	Tester1.print_report()
	return(0)

main()
