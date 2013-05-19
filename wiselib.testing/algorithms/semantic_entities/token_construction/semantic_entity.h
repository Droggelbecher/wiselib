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

#ifndef SEMANTIC_ENTITY_H
#define SEMANTIC_ENTITY_H

#include <limits>

#include <util/pstl/vector_static.h>
#include <util/pstl/map_static_vector.h>
#include <util/pstl/algorithm.h>
#include <util/serialization/serialization.h>

#include "regular_event.h"
#include "semantic_entity_id.h"

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
		typename Timer_P = typename OsModel_P::Timer,
		int MAX_NEIGHBORS_P = 8
	>
	class SemanticEntity {
		public:
			typedef SemanticEntity<OsModel_P, Radio_P, Clock_P, Timer_P, MAX_NEIGHBORS_P> self_type;
				
			typedef OsModel_P OsModel;
			typedef typename OsModel::block_data_t block_data_t;
			typedef typename OsModel::size_t size_type;
			typedef ::uint8_t token_count_t;
			typedef ::uint8_t distance_t;
			typedef Radio_P Radio;
			typedef typename Radio::node_id_t node_id_t;
			typedef Clock_P Clock;
			typedef typename Clock::millis_t millis_t;
			typedef typename Clock::time_t time_t;
			typedef Timer_P Timer;
			typedef RegularEvent<OsModel, Radio, Clock, Timer> RegularEventT;
			
			class TreeState;
			class TokenState;
			
			enum Restrictions { MAX_NEIGHBORS = MAX_NEIGHBORS_P };
			enum SpecialValues {
				npos = (size_type)(-1),
				NULL_NODE_ID = Radio::NULL_NODE_ID
			};
			
			typedef vector_static<OsModel, node_id_t, MAX_NEIGHBORS> Childs;
			typedef MapStaticVector<OsModel, node_id_t, RegularEventT, MAX_NEIGHBORS> TokenForwards;
			
			/**
			 */
			class TreeState {
				// {{{
				public:
					TreeState() : parent_(0), root_(std::numeric_limits<node_id_t>::max()), distance_(-1) {
					}
					
					TreeState(const TreeState& other) { *this = other; }
					
					node_id_t parent() const { return parent_; }
					void set_parent(node_id_t p) { parent_ = p; }
					
					node_id_t root() const { return root_; }
					void set_root(node_id_t r) { root_ = r; }
					
					distance_t distance() const { return distance_; }
					void set_distance(distance_t s) { distance_ = s; }
					
				private:
					node_id_t parent_;
					node_id_t root_;
					::uint32_t distance_;
				// }}}
			};
			typedef MapStaticVector<OsModel, node_id_t, TreeState, MAX_NEIGHBORS> TreeStates;
			
			/**
			 */
			class TokenState {
				// {{{
				public:
					TokenState() : token_count_(0) {
					}
					
					token_count_t count() const { return token_count_; }
					void set_count(token_count_t c) { token_count_ = c; }
					
					void increment_count() {
						token_count_++;
					}
					
				private:
					token_count_t token_count_;
				// }}}
			};
			
			/**
			 */
			class State {
				// {{{
				public:
					typedef SemanticEntity::TokenState TokenState;
					typedef SemanticEntity::TreeState TreeState;
					
					State() : dirty_(true) {
					}
					
					State(const SemanticEntityId& id) : id_(id), dirty_(true) {
					}
					
					SemanticEntityId& id() { return id_; }
					
					//TreeState& tree() { return tree_state_; }
					//TokenState& token() { return token_state_; }
					
					token_count_t count() const { return token_state_.count(); }
					node_id_t parent() const { return tree_state_.parent(); }
					node_id_t root() const { return tree_state_.root(); }
					distance_t distance() const { return tree_state_.distance(); }
					
					void set_count(token_count_t c) {
						if(c != token_state_.count()) {
							token_state_.set_count(c);
							dirty_ = true;
						}
					}
					
					void increment_count() {
						token_state_.increment_count();
						dirty_ = true;
					}
					
					/**
					 * @return true if value was actually changed.
					 */
					bool set_parent(node_id_t r) {
						if(r != tree_state_.parent()) {
							DBG("// treestatechange parent %d -> %d", tree_state_.parent(), r);
							tree_state_.set_parent(r);
							dirty_ = true;
							return true;
						}
						return false;
					}
					
					/**
					 * @return true if value was actually changed.
					 */
					bool set_root(node_id_t r) {
						if(r != tree_state_.root()) {
							DBG("// treestatechange root %d -> %d", tree_state_.root(), r);
							tree_state_.set_root(r);
							dirty_ = true;
							return true;
						}
						return false;
					}
					
					/**
					 * @return true if value was actually changed.
					 */
					bool set_distance(distance_t d) {
						if(d != tree_state_.distance()) {
							DBG("// treestatechange distance %d -> %d", tree_state_.distance(), d);
							tree_state_.set_distance(d);
							dirty_ = true;
							return true;
						}
						return false;
					}
					
					bool dirty() const { return dirty_; }
					void set_clean() { dirty_ = false; }
					
					const TreeState& tree() { return tree_state_; }
					const TokenState& token() { return token_state_; }
					
				private:
					TreeState tree_state_;
					TokenState token_state_;
					SemanticEntityId id_;
					bool dirty_;
					
				template<typename OsModel_, typename Radio_, typename Clock_, typename Timer_, int MAX_NEIGHBORS_>
				friend class SemanticEntity;
				
				template<typename OsModel_, Endianness Endianness_, typename BlockData_, typename T_>
				friend class Serialization;
				
				// }}}
			};
			typedef MapStaticVector<OsModel, node_id_t, State, MAX_NEIGHBORS> States;
			
			/// Ctors.
			
			SemanticEntity() : activity_phase_(false) {
			}
			
			SemanticEntity(const SemanticEntityId& id) : state_(id), activity_phase_(false) {
			}
			
			SemanticEntity(const SemanticEntity& other) {
				*this = other;
			}
			
			/**
			 * @return true iff the entity is currently in an activity phase.
			 * That is, when is_active() is true, the token construction will
			 * sooner or later start an activity phase (involving setting up
			 * timers). This manages a bool variable to track if that has
			 * already hpappened or not.
			 */
			bool in_activity_phase() { return activity_phase_; }
			void begin_activity_phase() { activity_phase_ = true; }
			void end_activity_phase() { activity_phase_ = false; }
			
			
			/**
			 * Being awake (in contrast to active) means that currently a unit
			 * of caffeine is allocated.
			 * Note that being awake is independent from being active:
			 * A node can be only awake (waiting for a token),
			 * only active (got the token before awake phase when it was awake
			 * for another reason), neither (just sleeping) or both (active in
			 * the duty cycling sense).
			 */
			bool is_awake() {
				if(activating_token_.waiting()) {
					return true;
				}
				for(typename TokenForwards::iterator it = token_forwards_.begin(); it != token_forwards_.end(); ++it) {
					if(it->second.waiting()) { return true; }
				}
				return false;
			}
			
			/**
			 * Recalculate current internal tree state.
			 * @param mynodeid id of this node.
			 * @return true iff the state actually changed.
			 */
			bool update_state(node_id_t mynodeid) {
				// {{{
				
				// Fill child list from neighbors and sort
				// 
				if(neighbor_states_.size() == 0) {
					DBG("// node %d has no neighbors!", mynodeid);
				}
				
				Childs oldchilds = childs_;
				
				childs_.clear();
				for(typename TreeStates::iterator iter = neighbor_states_.begin(); iter != neighbor_states_.end(); ++iter) {
					DBG("node %d SE %d.%d neighbor %d neighbor_parent %d neighbor_root %d neighbor_distance %d",
							mynodeid, id().rule(), id().value(), iter->first, iter->second.parent(), iter->second.root(), iter->second.distance());
					if(iter->second.parent() == mynodeid) {
						DBG("// %d found child %d", mynodeid, iter->first)
						if(childs_.find(iter->first) == childs_.end()) {
							childs_.push_back(iter->first);
						}
					}
				}
				sort(childs_.begin(), childs_.end());
				
				
				// did we loose children?
				
				DBG("node %d // SE %d.%d child list begin old=%d new=%d this=%p", mynodeid, id().rule(), id().value(), oldchilds.size(), childs_.size(), this);
				typename Childs::iterator i_new = childs_.begin();
				for(typename Childs::iterator i_old = oldchilds.begin(); i_old != oldchilds.end(); ++i_old) {
					DBG("node %d // SE %d.%d old child %d this=%p", mynodeid, id().rule(), id().value(), *i_old, this);
					if(i_new != childs_.end()) {
						DBG("node %d // SE %d.%d new child %d", mynodeid, id().rule(), id().value(), *i_new);
					}
						
					while(i_new != childs_.end() && *i_new < *i_old) {
						i_new++;
						if(i_new != childs_.end()) {
							DBG("node %d // SE %d.%d new child %d", mynodeid, id().rule(), id().value(), *i_new);
						}
					}
					if(i_new == childs_.end() || *i_new != *i_old) {
						DBG("node %d // SE %d.%d LOST CHILD %d", mynodeid, id().rule(), id().value(), *i_old);
						// lost child *i_old
						cancel_timers(*i_old);
					}
				}
				if(i_new != childs_.end()) { ++i_new; }
				for(; i_new != childs_.end(); ++i_new) {
					DBG("node %d // SE %d.%d new child %d", mynodeid, id().rule(), id().value(), *i_new);
				}
				DBG("node %d // SE %d.%d child list end this=%p", mynodeid, id().rule(), id().value(), this);
				
				
				// Update tree state
				// 
				::uint8_t distance = -1;
				node_id_t parent = mynodeid;
				node_id_t root = mynodeid;
				for(typename TreeStates::iterator iter = neighbor_states_.begin(); iter != neighbor_states_.end(); ++iter) {
					if(iter->second.parent() == mynodeid) {
						continue;
					}
					
					if(iter->second.root() < root) {
						parent = iter->first;
						root = iter->second.root();
						distance = iter->second.distance() + 1;
					}
					else if(iter->second.root() == root && (iter->second.distance() + 1) < distance) {
						parent = iter->first;
						distance = iter->second.distance() + 1;
					}
				}
				
				if(root == mynodeid) {
					distance = 0;
					parent = mynodeid;
				}
				
				bool changed = false;
				
				// don't use something like "changed = changed || state().set_xxx()" here!
				// short circuit evaluation will kill you!
				bool c_a = state().set_distance(distance);
				bool c_b = state().set_parent(parent);
				bool c_c = state().set_root(root);
				changed = changed || c_a || c_b || c_c;
				
				DBG("node %d SE %d.%d distance %d parent %d root %d",
						mynodeid, id().rule(), id().value(), state().distance(), state().parent(), state().root());
				
				return changed;
				
				// }}}
			}
			
			/**
			 * @return true iff the token state defines this node as active.
			 * Note that this is not always the same as the entity being
			 * awake.
			 * Activity says that the node *should* be awake, awakeness that
			 * it actually is. This distinction is important so we can avoid
			 * multiple unecessary parallel running awakeness timers.
			 */
			bool is_active(node_id_t mynodeid) {
				token_count_t l = prev_token_state_.count();
				if(is_root(mynodeid)) {
					return l == token().count();
				}
				else {
					return l > token().count();
				}
			}
			
			/**
			 * Update the internal token state with the previously received
			 * one such that this node will not be considered active anymore
			 * if it was before.
			 */
			void update_token_state(node_id_t mynodeid) {
				token_count_t l = prev_token_state_.count();
				if(is_root(mynodeid)) {
					if(l == token().count()) {
						state().increment_count();
					}
				}
				else {
					state().set_count(l);
				}
			}
			
			/**
			 * @return true iff this node is currently root of the SE tree.
			 */
			bool is_root(node_id_t mynodeid) {
				return tree().root() == mynodeid;
			}
			
			void set_count(token_count_t c) {
				state().set_count(c);
			}
			
			token_count_t prev_token_count() {
				return prev_token_state_.count();
			}
			
			
			/**
			 * Where should the token information be sent to after
			 * we processed it?
			 * Note this is different from forwarding a token we received but
			 * dont process!
			 */
			node_id_t next_token_node() {
				if(childs() > 0) {
					return childs_[0];
				}
				return tree().parent();
			}
			
			void set_prev_token_count(token_count_t ptc) {
				prev_token_state_.set_count(ptc);
			}
			
			void set_clean() {
				state_.set_clean();
				// TODO: mark states as clean
			}
			
			SemanticEntityId& id() { return state_.id(); }
			node_id_t parent() { return state_.tree().parent(); }
			node_id_t root() { return state_.tree().root(); }
			token_count_t count() { return state_.token().count(); }
			
			TreeState& neighbor_state(node_id_t id) {
				return neighbor_states_[id];
			}
			
			/**
			 * Return length of ordered list of childs.
			 */
			size_type childs() {
				return childs_.size();
			}
			
			/**
			 * Return position of child in ordered list of childs.
			 */
			size_type find_child(node_id_t id) {
				return find_child(id, childs_);
			}
			
			static size_type find_child(node_id_t id, Childs& childs) {
				// TODO
				typename Childs::iterator it = childs.find(id);
				if(it == childs.end()) { return npos; }
				return it - childs.begin();
			}
			
			node_id_t child_address(size_type idx) {
				if(idx >= childs_.size()) {
					return NULL_NODE_ID;
				}
				return childs_[idx];
			}
			
			State& state() { return state_; }
			const TreeState& tree() { return state_.tree(); }
			const TokenState& token() { return state_.token(); }
			
			bool dirty() { return state_.dirty(); }
			
			bool operator==(SemanticEntity& other) { return id() == other.id(); }
			bool operator<(SemanticEntity& other) { return id() < other.id(); }
			
			void erase_neighbor(node_id_t neighbor) {
				neighbor_states_.erase(neighbor);
				cancel_timers(neighbor);
			}
			
			/// Timing.
			
			void learn_activating_token(typename Clock::self_pointer_t clock, node_id_t mynodeid, time_t hit) {
				activating_token_.hit(hit, clock, mynodeid);
			}
			
			template<typename T, void (T::*BeginWaiting)(void*), void (T::*EndWaiting)(void*)>
			void schedule_activating_token(
					typename Clock::self_pointer_t clock,
					typename Timer::self_pointer_t timer,
					T* obj, void* userdata = 0
			) {
				activating_token_.template start_waiting_timer<T, BeginWaiting, EndWaiting>(clock, timer, obj, userdata);
			}
			
			void end_wait_for_activating_token() {
				activating_token_.end_waiting();
			}
			
			
			void learn_token_forward(typename Clock::self_pointer_t clock, node_id_t mynodeid, node_id_t from, time_t hit) {
				token_forwards_[from].hit(hit, clock, mynodeid);
			}
			
			template<typename T, void (T::*BeginWaiting)(void*), void (T::*EndWaiting)(void*)>
			void schedule_token_forward(
					typename Clock::self_pointer_t clock,
					typename Timer::self_pointer_t timer,
					T* obj, node_id_t from, void* userdata = 0
			) {
				DBG("// scheduling token_forward from %d SE %d.%d", from, id().rule(), id().value());
				token_forwards_[from].template start_waiting_timer<T, BeginWaiting, EndWaiting>(clock, timer, obj, userdata);
			}
			
			void end_wait_for_token_forward(node_id_t from) {
				token_forwards_[from].end_waiting();
			}
			
			
			/// Debugging.
			
			void print_state(node_id_t mynodeid, unsigned t, const char* comment) {
				DBG("node %d SE %d.%d active=%d awake=%d count=%d t=%d parent=%d root=%d distance=%d // %s", mynodeid, id().rule(), id().value(), is_active(mynodeid),
						is_awake(), count(), t,
						tree().parent(), tree().root(), tree().distance(),
						comment);
				//DBG(" parent=%d root=%d distance=%d", tree().parent(), tree().root(), tree().distance());
				//DBG(" count=%d active=%d awake=%d", token().count(), is_active(mynodeid), is_awake());
			}
			
			
		private:
			
			void cancel_timers(node_id_t n) {
				if(token_forwards_.contains(n)) {
					token_forwards_[n].cancel();
				}
			}
			
			/// Timing.
			
			TokenForwards token_forwards_;
			RegularEventT activating_token_;
			
			State state_;
			
			// Cached states from other nodes
			TokenState prev_token_state_; // just the token value of previous
			TreeStates neighbor_states_; // node_id => TreeState
			
			// vector of pairs: (time-offs from token recv, sender of forward
			// token)
			//vector_static<OsModel, pair< millis_t, node_id_t >, MAX_NEIGHBORS > token_forwards_;
			Childs childs_;
			bool activity_phase_;
	}; // SemanticEntity
	
	
	
	template<
		typename OsModel_P,
		Endianness Endianness_P,
		typename BlockData_P
	>
	struct Serialization<OsModel_P, Endianness_P, BlockData_P, typename SemanticEntity<OsModel_P>::State > {
		typedef OsModel_P OsModel;
		typedef BlockData_P block_data_t;
		typedef SemanticEntity<OsModel_P> SemanticEntityT;
		typedef typename SemanticEntityT::State State;
		typedef typename SemanticEntityT::TreeState TreeState;
		typedef typename SemanticEntityT::TokenState TokenState;
		typedef typename OsModel::size_t size_type;
		
		enum { SERIALIZATION_SIZE = sizeof(SemanticEntityId) + sizeof(TreeState) + sizeof(TokenState) };
		
		static size_type write(block_data_t *data, State& value) {
			wiselib::write<OsModel>(data, value.id()); data += sizeof(SemanticEntityId);
			
			wiselib::write<OsModel>(data, value.tree_state_); data += sizeof(TreeState);
			//DBG("wrote tree state: %d %d %d -> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
					//value.tree_state_.parent(), value.tree_state_.root(), value.tree_state_.distance(),
					//data[-12], data[-11], data[-10], data[-9],
					//data[-8], data[-7], data[-6], data[-5],
					//data[-4], data[-3], data[-2], data[-1]
			//);
					
			wiselib::write<OsModel>(data, value.token_state_); data += sizeof(TokenState);
			return SERIALIZATION_SIZE; // sizeof(SemanticEntityId) + sizeof(TreeState) + sizeof(TokenState);
		}
		
		static State read(block_data_t *data) {
			//DBG("-------- READING state");
			State value;
			wiselib::read<OsModel>(data, value.id()); data += sizeof(SemanticEntityId);
			wiselib::read<OsModel>(data, value.tree_state_); data += sizeof(TreeState);
			wiselib::read<OsModel>(data, value.token_state_); data += sizeof(TokenState);
			return value;
		}
		
		/*
		template<
			typename _OsModel_P,
			typename _Radio_P,
			typename _Clock_P
		>
		friend class SemanticEntity;
		*/
	};
	
}

#endif // SEMANTIC_ENTITY_H

