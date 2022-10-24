/* -*- c++ -*- */
/*
 * Copyright 2022 zjhao.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "CTSSender_impl.h"

namespace gr {
  namespace lora_rts_cts {

    CTSSender::sptr
    CTSSender::make()
    {
      return gnuradio::get_initial_sptr
        (new CTSSender_impl());
    }


    /*
     * The private constructor
     */
    CTSSender_impl::CTSSender_impl()
      : gr::block("CTSSender",
              gr::io_signature::make(1,1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0))
    {
       m_fft_size = 1024;
       m_fft = new fft::fft_complex(m_fft_size,true,1);
    }

    /*
     * Our virtual destructor.
     */
    CTSSender_impl::~CTSSender_impl()
    {
    }

    void
    CTSSender_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    CTSSender_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      unsigned int *out = (unsigned int *) output_items[0];

      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

