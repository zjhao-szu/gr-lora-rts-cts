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
#include "cadDetect_impl.h"

#define REQUIRE_PREAMBLE_NUMBER 8
#define DetectMethod CADDetectMinBin
#define CADDetectCorrelation  0
#define CADDetectMinBin  1

namespace gr {
  namespace lora_rts_cts {

    cadDetect::sptr
    cadDetect::make(uint8_t sf,uint32_t sampRate,uint32_t bw)
    {
      return gnuradio::get_initial_sptr
        (new cadDetect_impl(sf,sampRate,bw));
    }


    /*
     * The private constructor
     */
    cadDetect_impl::cadDetect_impl(uint8_t sf,uint32_t sampRate,uint32_t bw)
      : gr::block("cadDetect",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0)),
        m_sf(sf),
        m_samp_rate(sampRate),
        m_bw(bw)
    {
        m_cad_port = pmt::mp("CADSDR");
        message_port_register_out(m_cad_port);

        m_number_of_bins = (uint32_t)(1u << m_sf);
        m_oversample_rate = m_samp_rate / m_bw;

        m_samples_per_symbol = (uint32_t)(m_number_of_bins * m_oversample_rate);
        m_fft_size = m_samples_per_symbol;
        m_state = S_CAD_RESET;
        m_cad_result = false;
        m_preamble_drift_max = 1;

        m_fft = new fft::fft_complex(m_fft_size,true,1);

        m_upchirp.resize(m_samples_per_symbol);
        m_downchirp.resize(m_samples_per_symbol);
        for(int i = 0; i < m_samples_per_symbol;i++){
            double phase = M_PI / m_oversample_rate * (i - i*i / (float) m_samples_per_symbol);
            m_downchirp[i] = gr_complex(std::polar(1.0,phase));
            m_upchirp[i] = gr_complex(std::polar(1.0,-1*phase));
        }
    }

    /*
     * Our virtual destructor.
     */
    cadDetect_impl::~cadDetect_impl()
    {
    }



    void
    cadDetect_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
      ninput_items_required[0] = noutput_items * m_samples_per_symbol;
    }

        /*
    =*******************************************************************************
    */
    bool 
    cadDetect_impl::CADDetect_Correlation(std::vector<gr_complex> inputs){

    };

    unsigned int 
    cadDetect_impl::pmod(int x,unsigned int n){
        return ((x % n) + n) % n;
    }

    uint32_t 
    cadDetect_impl::argmax(float * fft_result,float * max_value){
        float mag = abs(fft_result[0]);
        float max_val = mag;
        uint32_t max_index = 0;
        for(int i = 0; i < m_number_of_bins;i++){
            mag = abs(fft_result[i]);
            if(mag > max_val){
                max_index = i;
                max_val = mag;
            }
        }
        *max_value = max_val;
        return max_index;
    }

    uint32_t 
    cadDetect_impl::searchFFTPeek(const gr_complex *fft_result,float * max_value,float * fft_res_mag){
        volk_32fc_magnitude_32f(fft_res_mag, fft_result, m_fft_size);
        uint32_t max_idx = argmax(fft_res_mag, max_value);
        return max_idx;
    }

    bool 
    cadDetect_impl::CADDetect_MinBin(){
        float m_preamble_idx = m_argmaxHistory[0];
        if(m_preamble_idx == 0) return false;
        // Check for discontinuities that exceed some tolerance
        bool preamble_found = true;
        int count = 4;
        for (int i = 1; i < m_argmaxHistory.size(); i++)
        {  
        //   std::cout<<"firstIdx"<<m_preamble_idx<<" "<<"historyIdx"<<m_argmaxHistory[i]<<std::endl;
          unsigned int dis = pmod(int(m_preamble_idx) - int(m_argmaxHistory[i]), m_fft_size);
          if (dis <= m_preamble_drift_max)
          {
            count--;
          }
        }
        return count > 0 ? false : true;
    };


    int
    cadDetect_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        unsigned int *out = (uint32_t *) output_items[0];
        unsigned int num_consumed = m_samples_per_symbol;
        uint32_t maxIndex = 0;
        float maxValue = 0;

        m_cad_result = false;
    
        
        gr_complex * signalBlockBuffer =  (gr_complex *) volk_malloc(m_fft_size * sizeof(gr_complex),volk_get_alignment());
        gr_complex * outbuf =  (gr_complex *) volk_malloc(m_fft_size * sizeof(gr_complex),volk_get_alignment());
        float * fft_res_mag = (float*)volk_malloc(m_fft_size*sizeof(float), volk_get_alignment());

        volk_32fc_x2_multiply_32fc(signalBlockBuffer, in, &m_downchirp[0], m_samples_per_symbol);
        
        memset(m_fft->get_inbuf(), 0,m_fft_size*sizeof(gr_complex));
        memcpy(m_fft->get_inbuf(),&signalBlockBuffer[0],m_samples_per_symbol*sizeof(gr_complex));
        m_fft->execute();
        
        maxIndex = searchFFTPeek(m_fft->get_outbuf(),&maxValue,fft_res_mag);
        if(maxValue >= 0.1){
            // std::cout<<"max value is"<<maxValue<<std::endl;
            m_argmaxHistory.insert(m_argmaxHistory.begin(),maxIndex);
        }else{
            // std::cout<<"while noise is"<<maxValue<<std::endl;
        }
       
        if(m_argmaxHistory.size() > REQUIRE_PREAMBLE_NUMBER){
            m_argmaxHistory.pop_back();
        }


        switch (m_state)
        {
            case S_CAD_RESET:
            {
                /* code */
                m_argmaxHistory.clear();
                m_state = S_CAD_DETECT;
                break;
            }
            case S_CAD_DETECT:
            {
                #if DetectMethod == CADDetectMinBin
                    if(m_argmaxHistory.size() >= REQUIRE_PREAMBLE_NUMBER){
                        m_cad_result = CADDetect_MinBin();
                    }
                #else        
                    m_cad_result = CADDetect_Correlation();
                #endif
               
                if(m_cad_result){
                    m_state = S_CAD_OUT;
                    std::cout<<"Detecting ....: result"<< (m_cad_result ? "true" : "false") << std::endl;
                }
                break;
            }
               
            case S_CAD_OUT:{
                std::cout<<"Detecting result is true!! starting send message to next block!!!!"<<std::endl;
                pmt::pmt_t dict = pmt::make_dict();
                dict = pmt::dict_add(dict,pmt::intern("CAD_DETECT_RESULT"),pmt::PMT_T);
                pmt::pmt_t msg_pair = pmt::cons(dict,pmt::make_u8vector(10,0));
                m_state = S_CAD_RESET;
                message_port_pub(m_cad_port, msg_pair);
                std::cout<<"sending over!!!!"<<std::endl;
                break;
            }
               
            default:
                std::cout<<"default error"<<std::endl;
                exit(0);
                break;
        }
      
      
      
      
        // Do <+signal processing+>
        // Tell runtime system how many input items we consumed on
        // each input stream.
        consume_each (num_consumed);
        volk_free(outbuf);
        volk_free(fft_res_mag);
        volk_free(signalBlockBuffer);
        
        

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace lora_rts_cts */
} /* namespace gr */

