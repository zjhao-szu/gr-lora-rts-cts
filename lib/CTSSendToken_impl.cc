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
#include "CTSSendToken_impl.h"

namespace gr {
  namespace lora_rts_cts {

    CTSSendToken::sptr
    CTSSendToken::make(uint8_t sf, uint32_t samp_rate, uint32_t bw, std::vector<uint16_t> sync_words)
    {
      return gnuradio::get_initial_sptr
        (new CTSSendToken_impl(sf,samp_rate,bw,sync_words));
    }


    /*
     * The private constructor
     */
    CTSSendToken_impl::CTSSendToken_impl(uint8_t sf, uint32_t samp_rate, uint32_t bw, std::vector<uint16_t> sync_words)
      : gr::block("CTSSendToken",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(gr_complex))),
        m_sf(sf),
        m_bw(bw),
        m_samp_rate(samp_rate),
        m_sync_words(sync_words)
    {
      pmt::pmt_t m_receiveCTSTokenPort = pmt::mp("CTSToken");
      message_port_register_in(m_receiveCTSTokenPort);
      set_msg_handler(m_receiveCTSTokenPort, boost::bind(&CTSSendToken_impl::receiveCTSToken, this, _1));

      m_number_of_bins = (uint32_t)(1u << m_sf);
      m_oversample_rate = m_samp_rate / m_bw;
      m_samples_per_symbol = (uint32_t) (m_number_of_bins * m_oversample_rate);

      m_downchirp.resize(m_samples_per_symbol);
      m_upchirp.resize(m_samples_per_symbol);

      build_ref_chirps(&m_upchirp[0],&m_downchirp[0],m_sf,m_oversample_rate);

      if(m_sync_words.size() == 1){
        uint16_t tmp = m_sync_words[0];
        m_sync_words.resize(2,0);
        m_sync_words[0] = ((tmp&0xF0)>>4)<<3;
        m_sync_words[0] = ((tmp&0x0F))<<3;
      }

      m_up = 8;
      preamb_symb_cnt = 0;
      m_CTSToken = 0;
      m_state = STATE_RESET;
    }

    /*
     * Our virtual destructor.
     */
    CTSSendToken_impl::~CTSSendToken_impl()
    {
      
    }
    void
    CTSSendToken_impl::receiveCTSToken(pmt::pmt_t msg){
      m_CTSToken = pmt::to_uint64(msg);
      m_state = STATE_SEND_TOKEN;
    }
    void
    CTSSendToken_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    CTSSendToken_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex * in = (const gr_complex  *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];
      if(m_state == STATE_RESET){
        consume_each (0);
        return 0;
      }else{
        m_up = 8;
        preamb_symb_cnt = 0;
      }
      int output_offset = 0;
      preamb_symb_cnt = 0;
      for (int i = 0; i < noutput_items / m_samples_per_symbol; i++)
      {
          if (preamb_symb_cnt < n_up + 5) //should output preamble part
          {
              if (preamb_symb_cnt < n_up)
              { //upchirps
                  memcpy(&out[output_offset], &m_upchirp[0], m_samples_per_symbol * sizeof(gr_complex));
              }
              else if (preamb_symb_cnt == n_up) //sync words
                  build_upchirp(&out[output_offset], m_sync_words[0], m_sf,m_oversample_rate);
              else if (preamb_symb_cnt == n_up + 1)
                  build_upchirp(&out[output_offset], m_sync_words[1], m_sf,m_oversample_rate);

              else if (preamb_symb_cnt < n_up + 4) //2.25 downchirps
                  memcpy(&out[output_offset], &m_downchirp[0], m_samples_per_symbol * sizeof(gr_complex));
              else if (preamb_symb_cnt == n_up + 4)
              {
                  memcpy(&out[output_offset], &m_downchirp[0], m_samples_per_symbol / 4 * sizeof(gr_complex));
                  //correct offset dur to quarter of downchirp
                  output_offset -= 3 * m_samples_per_symbol / 4;
                  symb_cnt = 0;
                  
              }
              output_offset += m_samples_per_symbol;
              preamb_symb_cnt++;
          }
      }
      if(output_offset > noutput_items){
        std::cout<<"添加的一次性内容超过了gnuradio最多项数"<<std::endl;
      }else{
        build_downchirp(&out[output_offset], m_CTSToken, m_sf,m_oversample_rate);
        output_offset += m_samples_per_symbol; 
      }


      // Do <+signal processing+>
      // Tell runtime system how many input items we consumed on
      
      
      
      // each input stream.
      consume_each (0);

      // Tell runtime system how many output items we produced.
      return output_offset;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

