/****
Copyright (c) 2014-2016, University of Tuebingen, Benjamin Buchfink
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

#ifndef ALIGN_RANGE_H_
#define ALIGN_RANGE_H_

#include "../basic/statistics.h"
#include "../data/sorted_list.h"
#include "../search/trace_pt_buffer.h"
#include "sse_dist.h"

struct Stage1_hit
{
	Stage1_hit(unsigned q_ref, unsigned q_offset, unsigned s_ref, unsigned s_offset) :
		q(q_ref + q_offset),
		s(s_ref + s_offset)
	{}
	bool operator<(const Stage1_hit &rhs) const
	{
		return q < rhs.q;
	}
	struct Query
	{
		unsigned operator()(const Stage1_hit &x) const
		{
			return x.q;
		}
	};
	unsigned q, s;
};

struct Range_ref
{
	Range_ref(vector<Finger_print>::const_iterator q_begin, vector<Finger_print>::const_iterator s_begin) :
		q_begin(q_begin),
		s_begin(s_begin)
	{}
	const vector<Finger_print>::const_iterator q_begin, s_begin;
};

struct Seed_filter
{
	Seed_filter(Statistics &stats, Trace_pt_buffer::Iterator &out, const unsigned sid) :
		vq(TLS::get(vq_ptr)),
		vs(TLS::get(vs_ptr)),
		hits(TLS::get(hits_ptr)),
		stats(stats),
		out(out),
		sid(sid)
	{}
	void run(const sorted_list::const_iterator &q, const sorted_list::const_iterator &s);
	void tiled_search(vector<Finger_print>::const_iterator q,
		vector<Finger_print>::const_iterator q_end,
		vector<Finger_print>::const_iterator s,
		vector<Finger_print>::const_iterator s_end,
		const Range_ref &ref,
		unsigned level);

	static TLS_PTR vector<Finger_print> *vq_ptr, *vs_ptr;
	static TLS_PTR vector<Stage1_hit> *hits_ptr;
	vector<Finger_print> &vq, &vs;
	vector<Stage1_hit> &hits;
	Statistics &stats;
	Trace_pt_buffer::Iterator &out;
	const unsigned sid;
};

void stage2_search(const sorted_list::const_iterator &q,
	const sorted_list::const_iterator &s,
	const vector<Stage1_hit> &hits,
	Statistics &stats,
	Trace_pt_buffer::Iterator &out,
	const unsigned sid);

inline void align_partition(unsigned hp,
		Statistics &stats,
		unsigned sid,
		sorted_list::const_iterator i,
		sorted_list::const_iterator j,
		unsigned thread_id)
{
#ifndef SIMPLE_SEARCH
	if (hp > 0)
		return;
#endif
	Trace_pt_buffer::Iterator* out = new Trace_pt_buffer::Iterator (*Trace_pt_buffer::instance, thread_id);
	Seed_filter seed_filter(stats, *out, sid);
	while(!i.at_end() && !j.at_end()) {
		if(i.key() < j.key()) {
			++i;
		} else if(j.key() < i.key()) {
			++j;
		} else {
			if(i[0] != 0)
				seed_filter.run(j, i);
			++i;
			++j;
		}
	}
	delete out;
}

#endif /* ALIGN_RANGE_H_ */
