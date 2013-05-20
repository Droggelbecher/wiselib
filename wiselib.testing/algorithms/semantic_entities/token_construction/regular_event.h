/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

#ifndef REGULAR_EVENT_H
#define REGULAR_EVENT_H

#include <external_interface/external_interface.h>
#include <util/delegates/delegate.hpp>

namespace wiselib {
	
	/**
	 * @brief
	 * 
	 * @ingroup
	 * 
	 * @tparam 
	 */
	template<
		typename OsModel_P,
		typename Radio_P = typename OsModel_P::Radio,
		typename Clock_P = typename OsModel_P::Clock,
		typename Timer_P = typename OsModel_P::Timer
	>
	class RegularEvent {
		// {{{
		public:
			typedef OsModel_P OsModel;
			typedef typename OsModel::size_t size_type;
			typedef Radio_P Radio;
			typedef typename Radio::node_id_t node_id_t;
			typedef Clock_P Clock;
			typedef typename Clock::time_t time_t;
			typedef ::uint32_t abs_millis_t;
			typedef Timer_P Timer;
			typedef delegate1<void, void*> begin_waiting_callback_t;
			typedef delegate1<void, void*> end_waiting_callback_t;
			
			enum { TIMESCALE = 10 };
			
			/// Some fractions (in percent).
			enum TimingFractions {
				/**
				 * 1/x window size in which a hit is considered close.
				 * E.g. CLOSE_HIT_WINDOW = 4 --> hit in 1/4 of current window
				 * size is considered close.
				 */ 
				CLOSE_HIT_WINDOW = 25,
				/// Analogue to @a CLOSE_HIT_WINDOW.
				STABLE_HIT_WINDOW = 75,
				
				/// How much a close hit influences expected timing.
				ALPHA_CLOSE = 50,
				/// How much a stable hit influences expected timing
				ALPHA_STABLE = 25,
				/// How much a far hit influences expected timing
				ALPHA_FAR = 25
			};
			
			enum HitType { HIT_CLOSE, HIT_STABLE, HIT_FAR };
			enum Restrictions { MIN_WINDOW_SIZE = 100 * TIMESCALE };
			
			RegularEvent() : last_encounter_(0), interval_(1000 * TIMESCALE), window_(1000 * TIMESCALE),
				hits_(0), waiting_(false), waiting_timer_set_(false), cancel_(false) {
			}
			
			void hit(time_t t, typename Clock::self_pointer_t clock, node_id_t mynodeid) {
				time_t new_interval = t - last_encounter_;
				
				//DBG("------- HIT id %u current window size: %llu hit at %llu expected: %llu", mynodeid, window_, t, expected());
				
				switch(hit_type(t, clock)) {
					case HIT_CLOSE:
						//DBG("------ Close hit, halving window");
						window_ /= 2;
						if(window_ < MIN_WINDOW_SIZE) { window_ = MIN_WINDOW_SIZE; }
						update_interval(new_interval, ALPHA_CLOSE);
						break;
					case HIT_STABLE:
						//DBG("------ stable hit");
						update_interval(new_interval, ALPHA_STABLE);
						break;
					case HIT_FAR:
						//DBG("------ far hit, doubling window");
						//DBG("------- HIT window before double %llu", window_);
						window_ *= 2;
						//DBG("------- HIT window after double %llu", window_);
						if(window_ > interval_) { window_ = interval_; }
						update_interval(new_interval, ALPHA_FAR);
						break;
				}
				
				hits_++;
				//DBG("------- HIT id %u current window new: %llu hits=%d", mynodeid, window_, hits_);
				last_encounter_ = t;
			}
			
			time_t last_encounter() { return last_encounter_; }
			time_t window() { return window_; }
			time_t interval() { return interval_; }
			time_t expected() { return last_encounter_ + interval_; }
			
			time_t next_expected(time_t t) {
				time_t r = last_encounter_;
				while(r <= t) { r += interval_; }
				return r;
			}
			
			/**
			 * @return true iff this event is still in an early state.
			 * 
			 * That is, it can not be assumed to yield useful
			 * next_expected() values yet.
			 */
			bool early() { return hits_ <= 2; }
			
			/**
			 * Set waiting timer.
			 * If no waiting timer is set, set a timer to start
			 * waiting for the event according to current estimations.
			 * 
			 * @return true if a wakeup was scheduled, false else.
			 * The false-case will occur when the wakeup would be scheduled
			 * with 0ms delay in which case the caller should do whatever he
			 * wants done himself directly.
			 */
			template<typename T, void (T::*BeginWaiting)(void*), void (T::*EndWaiting)(void*)>
			bool start_waiting_timer(
					typename Clock::self_pointer_t clock,
					typename Timer::self_pointer_t timer,
					T* obj, void* userdata = 0
			) {
				if(waiting_timer_set_) {
					//DBG("t=%d // timer already set!", absolute_millis(clock, clock->time()));
					return true;
				}
				
				begin_waiting_callback_ = begin_waiting_callback_t::template from_method<T, BeginWaiting>(obj);
				end_waiting_callback_ = end_waiting_callback_t::template from_method<T, EndWaiting>(obj);
				userdata_ = userdata;
				
				if(early()) {
					DBG("t=%d // EARLY! hits=%d userdata=%p", absolute_millis(clock, clock->time()), hits_, userdata);
					waiting_ = true;
					begin_waiting_callback_(userdata_);
					
				}
				else {
					waiting_timer_set_ = true;
					
					abs_millis_t delta;
					delta = absolute_millis(clock, next_expected(clock->time()) - clock->time() - window_);
					//DBG("t=%d // begin_waiting in %dms", absolute_millis(clock, clock->time()), delta);
					timer->template set_timer<RegularEvent, &RegularEvent::begin_waiting>( delta, this, 0);
				}
				return true;
			}
			
			/**
			 * This on the other hand please call manually
			 * if you got your event!
			 * Also remember to call start_waiting_timer to wait for
			 * the next event!
			 */
			bool end_waiting() {
				if(waiting_) {
					waiting_ = false;
					end_waiting_callback_(userdata_);
					return true;
				}
				return false;
			}
			
			void cancel() {
				if(!end_waiting()) {
					//DBG("// cancel without end_waiting_callback");
					cancel_ = true;
				}
				//DBG("// cancelled without end_waiting_callback");
			}
			
			bool waiting() { return waiting_; }
			
		private:
			/**
			 * x.
			 * Do not call this directly, it will be called by the
			 * timer installed by start_waiting_timer!
			 */
			void begin_waiting(void*) {
				waiting_timer_set_ = false;
				if(!waiting_) {
					if(cancel_) {
						//DBG("// begin_waiting: net executing because cancel");
						waiting_ = false;
						cancel_ = false;
					}
					else {
						waiting_ = true;
						begin_waiting_callback_(userdata_);
					}
				}
			}
			
			static abs_millis_t absolute_millis(typename Clock::self_pointer_t clock, const time_t& t) {
				return clock->seconds(t) * 1000 + clock->milliseconds(t);
			}
	
			HitType hit_type(time_t t, typename Clock::self_pointer_t clock_) {
				time_t diff_t = (t > expected()) ? (t - expected()) : (expected() - t);
				//millis_t diff_m  = clock_->milliseconds(diff_t);
				
				if(diff_t < window_ * CLOSE_HIT_WINDOW / 100) { return HIT_CLOSE; }
				else if(diff_t < window_ * STABLE_HIT_WINDOW / 100) { return HIT_STABLE; }
				else { return HIT_FAR; }
			}
			
			void update_interval(time_t new_interval, ::uint8_t alpha) {
				//assert(new_interval > time_t(0));
				if(new_interval < window_) { new_interval = window_; }
				
				if(early()) {
					interval_ = new_interval;
				}
				else {
					interval_ = (interval_ * (100 - alpha) + new_interval * alpha) / 100;
				}
				
				check_invariant();
			}
			
			void check_invariant() {
				assert(window_ > time_t(0));
				assert(interval_ >= window_);
			}
	
			begin_waiting_callback_t begin_waiting_callback_;
			end_waiting_callback_t end_waiting_callback_;
			
			time_t last_encounter_;
			time_t interval_;
			time_t window_;
			size_type hits_;
			void *userdata_;
			
			/// true iff there is currently a timer waiting for this
			bool waiting_;
			bool waiting_timer_set_;
			bool cancel_;
		// }}}
	};
}

#endif // REGULAR_EVENT_H

