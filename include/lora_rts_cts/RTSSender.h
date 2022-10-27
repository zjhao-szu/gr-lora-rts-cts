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

#ifndef INCLUDED_LORA_RTS_CTS_RTSSENDER_H
#define INCLUDED_LORA_RTS_CTS_RTSSENDER_H

#include <lora_rts_cts/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace lora_rts_cts {
	enum RTS_Sender_State{
		S_RTS_RESET,
		S_RTS_RECEIVE_DATA,
		S_RTS_CAD,
		S_RTS_WAIT_DATA,
		S_RTS_WAIT_CTS,
		S_RTS_send_RTS,
		S_RTS_SLEEP,
    S_RTS_TO_RECEIVE1,
		S_RTS_RECEIVE1,
    S_RTS_TO_RECEIVE2,
		S_RTS_RECEIVE2,
		S_RTS_Send_Data,
    S_RTS_RECEIVE_Slot,
    S_RTS_RECEIVE_CLass_C,
		S_RTS_BEACON
	};
    /*!
     * \brief <+description of block+>
     * \ingroup lora_rts_cts
     *
     */
    class LORA_RTS_CTS_API RTSSender : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<RTSSender> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of lora_rts_cts::RTSSender.
       *
       * To avoid accidental use of raw pointers, lora_rts_cts::RTSSender's
       * constructor is in a private implementation
       * class. lora_rts_cts::RTSSender::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace lora_rts_cts
} // namespace gr

#endif /* INCLUDED_LORA_RTS_CTS_RTSSENDER_H */

