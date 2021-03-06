/****
Copyright (c) 2016, Benjamin Buchfink
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
****/

#include "seq_file_format.h"

struct Raw_text {};
struct Sequence_data {};

template<typename _t>
inline char convert_char(char a)
{
	return a;
}

template<>
inline char convert_char<Sequence_data>(char a)
{
	return input_value_traits.from_char(a);
}

template<typename _t, typename _what>
void copy_line(Compressed_istream & s, vector<_t>& v, _what)
{
	char a = 0;
	while (s.read(&a, 1) == 1 && a != '\n' && a != '\r')
		v.push_back(convert_char<_what>(a));
	if (a == '\r')
		if (s.read(&a, 1) != 1 || a != '\n')
			throw file_format_exception();
}

void skip_line(Compressed_istream &s)
{
	char a = 0;
	while (s.read(&a, 1) == 1 && a != '\n' && a != '\r');
	if (a == '\r')
		if (s.read(&a, 1) != 1 || a != '\n')
			throw file_format_exception();
}

void copy_until(Compressed_istream &s, int delimiter, vector<Letter> &v)
{
	char a = 0;
	size_t col = 0;
	while (s.read(&a, 1) == 1 && a != delimiter) {
		switch (a) {
		case '\n':
			col = 0;
		case '\r':
			break;
		default:
			v.push_back(input_value_traits.from_char(a));
			++col;
		}
	}
	if (a == delimiter) {
		if (col > 0)
			throw file_format_exception();
		else
			s.putback(a);
	}
}

void skip_char(Compressed_istream &s, char c)
{
	char a;
	if (s.read(&a, 1) != 1 || a != c)
		throw file_format_exception();
}

bool have_char(Compressed_istream &s, char c)
{
	char a;
	if (s.read(&a, 1) == 0)
		return false;
	else if (a != c)
		throw file_format_exception();
	else
		return true;
}

bool FASTA_format::get_seq(vector<char>& id, vector<Letter>& seq, Compressed_istream & s) const
{
	if (!have_char(s, '>'))
		return false;
	id.clear();
	seq.clear();
	copy_line(s, id, Raw_text());
	copy_until(s, '>', seq);
	return true;
}

bool FASTQ_format::get_seq(vector<char>& id, vector<Letter>& seq, Compressed_istream & s) const
{
	if (!have_char(s, '@'))
		return false;
	id.clear();
	seq.clear();
	copy_line(s, id, Raw_text());
	copy_line(s, seq, Sequence_data());
	skip_char(s, '+');
	skip_line(s);
	skip_line(s);
	return true;
}

const Sequence_file_format * guess_format(const string & file)
{
	static const FASTA_format fasta;
	static const FASTQ_format fastq;

	Compressed_istream f(file);
	char c;
	if (f.read(&c, 1) != 1)
		throw file_format_exception();
	switch (c) {
	case '>': return &fasta;
	case '@': return &fastq;
	default: throw file_format_exception();
	}
	return 0;
}
