/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Felix Brendel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include "macros.hpp"
#include "types.hpp"
#include "print.hpp"
#include "bucket_allocator.hpp"
#include "arraylist.hpp"
#include "stacktrace.hpp"


// NOTE(Felix): since we want to be able to interpolate any data in principle,
//   and the animation needs the start and end values, but the size of the stuff
//   we want to interpolate may vary, we just give it a small chunk of memory to
//   be stored
typedef s64 Data_Block[22];


typedef void (*Closure_F)(Data_Block);
typedef void (*Lambda_F)();

typedef f32  (*Shaper_F)(f32 t);
typedef void (*Interpolator_F)(void* from, f32 t, void* to, void* interpolant);


enum struct Interpolation_Type : u8 {
    Lerp = 1 << 7,
    Ease_In,
    Ease_Out,
    Ease_Middle,
    Ease_In_And_Out,
    Constant
};

enum struct Interpolant_Type : u8 {
    F32 = 1 << 7,
};


enum struct Function_Type {
    Closure,
    Lambda
};


struct Scheduled_Animation_Create_Info {
    float seconds_to_start;
    float seconds_to_end;

    void* interpolant;
    Interpolant_Type interpolant_type;

    void* from;
    void* to;

    Interpolation_Type interpolation_type = Interpolation_Type::Lerp;
    const char* name = "";
};


struct Action_Create_Info {
    Function_Type type;
    union {
        Lambda_F  lambda;
        Closure_F closure;
    };
    void* param      = nullptr;
    u32   param_size = 0;
    const char* name = "";
};

struct Scheduled_Action_Create_Info {
    float seconds_to_run;
    Action_Create_Info action;
};

struct Action {
    Function_Type type;
    union {
        Lambda_F  lambda;
        Closure_F closure;
    };
    Data_Block    param;
    Action*       next;
    u64           creation_stamp;
    bool          already_ran;
#ifdef FTB_INTERNAL_DEBUG
    const char* name;
#endif
};

struct Scheduled_Action {
    f32    seconds_to_run;
    Action action;
};

struct Scheduled_Animation {
    Interpolation_Type interpolation_type;
    Interpolant_Type   interpolant_type;

    f32 seconds_to_start;
    f32 seconds_to_end;

    void* interpolant;

    Data_Block from;
    Data_Block to;

    Action* next;
#ifdef FTB_INTERNAL_DEBUG
    const char* name;
#endif
};

enum struct Schedulee_Type : u8 {
    None = 0,
    Action,
    Animation,
};

struct Action_Or_Animation {
    Schedulee_Type type;
    union {
        Action* action;
        Scheduled_Animation* animation;
    };
};


struct Scheduler {
    // For user defined shapers t -> t
    Shaper_F       _shapers[127];
    // Interpolators for user types
    Interpolator_F _interpolators[127];
    size_t         _interpolatee_sizes[127];

    // Per reset cycle:
    // PING
    Bucket_Allocator<Scheduled_Animation> _active_animations_1;
    Bucket_Allocator<Scheduled_Action>    _scheduled_actions_1;
    Bucket_Allocator<Action>              _chained_actions_1;
    // PONG
    Bucket_Allocator<Scheduled_Animation> _active_animations_2;
    Bucket_Allocator<Scheduled_Action>    _scheduled_actions_2;
    Bucket_Allocator<Action>              _chained_actions_2;

    Bucket_Allocator<Scheduled_Animation>* active_animations;
    Bucket_Allocator<Scheduled_Action>*    scheduled_actions;
    Bucket_Allocator<Action>*              chained_actions;

    Bucket_Allocator<Scheduled_Animation>* future_active_animations;
    Bucket_Allocator<Scheduled_Action>*    future_scheduled_actions;
    Bucket_Allocator<Action>*              future_chained_actions;

    // Per Tick:
    // if something is deleted while iterating over them
    Array_List<Scheduled_Animation*> _animations_marked_for_deletion;
    Array_List<Scheduled_Action*>       _actions_marked_for_deletion;

    Array_List<Action*> _chained_actions_to_be_run_after_iteration;

    bool _is_iterating = false;
    bool _should_reset = false;
    bool _should_stop_iterating = false;
    u64 _action_stamp_counter = 0;

    auto init() -> void;
    auto deinit() -> void;
    auto reset() -> void;

    auto schedule_animation(Scheduled_Animation_Create_Info aci) -> Scheduled_Animation*;
    auto schedule_action(Scheduled_Action_Create_Info aci) -> Scheduled_Action*;
    auto chain_action(Scheduled_Animation* existing_animation, Action_Create_Info aci) -> Action*;
    auto chain_action(Action* existing_action, Action_Create_Info aci) -> Action*;
    auto chain_action(Action_Or_Animation aoa, Action_Create_Info aci) -> Action*;

    auto update_all(f32 delta_time) -> void;
    auto advance_single_animation(Scheduled_Animation* anim) -> void;

    auto register_shaper(Shaper_F f, Interpolation_Type t) -> void;
    auto register_interpolator(Interpolator_F f, Interpolant_Type t, size_t size_of_interpolant) -> void;

    auto execute_action(Action* action) -> void;
    auto handle_chained_action(Action* action) -> void;
    auto run_pending_actions_and_reset() -> void;

    auto _init_action(Action* action, Action_Create_Info aci) -> void;
    auto _actually_reset() -> void;
};

#ifdef FTB_SCHEDULER_IMPL

auto Scheduler::init () -> void {
    _active_animations_1.init();
    _scheduled_actions_1.init();
    _chained_actions_1.init();

    _active_animations_2.init();
    _scheduled_actions_2.init();
    _chained_actions_2.init();

    active_animations = &_active_animations_1;
    scheduled_actions = &_scheduled_actions_1;
    chained_actions   = &_chained_actions_1;

    future_active_animations = &_active_animations_2;
    future_scheduled_actions = &_scheduled_actions_2;
    future_chained_actions   = &_chained_actions_2;


    _animations_marked_for_deletion.init();
    _actions_marked_for_deletion.init();
    _chained_actions_to_be_run_after_iteration.init(20);
}

auto Scheduler::deinit() -> void {
    _active_animations_1.deinit();
    _scheduled_actions_1.deinit();
    _chained_actions_1.deinit();

    _active_animations_2.deinit();
    _scheduled_actions_2.deinit();
    _chained_actions_2.deinit();

    _animations_marked_for_deletion.deinit();
    _actions_marked_for_deletion.deinit();
    _chained_actions_to_be_run_after_iteration.deinit();
}

auto Scheduler::register_shaper(Shaper_F f, Interpolation_Type t) -> void {
    _shapers[(u8)t] = f;
}

auto Scheduler::register_interpolator(Interpolator_F f, Interpolant_Type t,
                                      size_t size_of_interpolant)
    -> void
{
    _interpolatee_sizes[(u8)t] = size_of_interpolant;
    _interpolators[(u8)t] = f;
}

auto Scheduler::schedule_animation(Scheduled_Animation_Create_Info aci)
    -> Scheduled_Animation*
{
    Scheduled_Animation* anim;
    if (_should_reset) {
        anim = future_active_animations->allocate();
    } else {
        anim = active_animations->allocate();
    }

    anim->next = nullptr;

#ifdef FTB_INTERNAL_DEBUG
    anim->name = aci.name;
#endif

    anim->interpolation_type = aci.interpolation_type;
    anim->interpolant_type   = aci.interpolant_type;

    anim->seconds_to_start = aci.seconds_to_start;
    anim->seconds_to_end   = aci.seconds_to_end;

    anim->interpolant = aci.interpolant;

    size_t data_size;
    switch(anim->interpolant_type) {
        case Interpolant_Type::F32:  data_size = sizeof(f32); break;
        default: {
            data_size = _interpolatee_sizes[(u8)(anim->interpolant_type)];
            panic_if(data_size == 0, "Interpolatee (%d) size was 0 (was it initialized correctly)?",
                     anim->interpolant_type);
        }
    }

    memcpy(&anim->from, aci.from, data_size);
    memcpy(&anim->to,   aci.to,   data_size);

    return anim;
}

auto Scheduler::advance_single_animation(Scheduled_Animation* anim) -> void {
    // if not yet started, skip
    if (anim->seconds_to_start > 0)
        return;

    // otherwise run it:
    f32 perf_diff = anim->seconds_to_end - anim->seconds_to_start;
    f32 perf_inside = -anim->seconds_to_start;
    f32 t = 1.0f * perf_inside / perf_diff;

    switch (anim->interpolation_type) {
        case Interpolation_Type::Lerp: break;
        case Interpolation_Type::Ease_In: {
            t *= t;
        } break;
        case Interpolation_Type::Ease_Out: {
            t = -(t * (t - 2));
        } break;
        case Interpolation_Type::Ease_In_And_Out: {
            // NOTE(Felix): yes -- Mon Dec  7 11:47:34 2020
            t = (t < 0.5) ?
                +2 * t * t :
                -2 * (t * (t - 2)) - 1;
        } break;
        case Interpolation_Type::Ease_Middle: {
            // t = (t < 0.5) ?
            //     -2 * t * (t - 1) :
            //     +2 * t * (t - 1) + 1;
            t = 4.0f / 5.0f * t*t*t - 6.0f/5.0f *t*t + 7.0f/5.0f * t;
        } break;
        case Interpolation_Type::Constant: {
            t = (t < 0.5f) ?
                0.0f :
                1.0f;
        } break;
        default: {
            // try user supported shaper
            Shaper_F shaper = _shapers[(u8)anim->interpolant_type];

            panic_if(shaper == nullptr,
                     "Shaper function for type (%d) was nullptr (was it initialized correctly)?",
                     anim->interpolation_type);

            t = shaper(t);
        }
    }

    switch(anim->interpolant_type) {
        case Interpolant_Type::F32: {
            f32 from = *((f32*)&anim->from);
            f32 to   = *((f32*)&anim->to);
            *((f32*)anim->interpolant) = from + (to - from) * t;
        } break;
        default: {
            // try user supported interpolator
            Interpolator_F interpolator = _interpolators[(u8)anim->interpolant_type];

            panic_if(interpolator == nullptr,
                     "Interpolator function for type (%d) was nullptr (was it initialized correctly)?",
                     anim->interpolant_type);

            interpolator(&anim->from, t, &anim->to, anim->interpolant);
        }
    }
}

auto Scheduler::_init_action(Action* action, Action_Create_Info aci) -> void {
    action->next = nullptr;
#ifdef FTB_INTERNAL_DEBUG
    action->name = aci.name;
#endif
    action->creation_stamp = _action_stamp_counter;

    ++_action_stamp_counter;
    action->already_ran = false;

    if (aci.param) {
        if(aci.param_size > sizeof(Data_Block)) {
            panic("%llu (sizeof(Data_Block)) < %u (aci.param_size)",
                  sizeof(Data_Block), aci.param_size);
        }
        action->type = Function_Type::Closure;
        action->closure = aci.closure;
        memcpy(&action->param, aci.param, aci.param_size);
    } else {
        action->type = Function_Type::Lambda;
        action->lambda = aci.lambda;
        memset(&action->param, 0, sizeof(Data_Block));
    }

    if      (aci.type == Function_Type::Lambda)  action->lambda  = aci.lambda;
    else if (aci.type == Function_Type::Closure) action->closure = aci.closure;

}


auto Scheduler::schedule_action(Scheduled_Action_Create_Info aci) -> Scheduled_Action* {
    Scheduled_Action* scheduled_action;
    if (_should_reset) {
        scheduled_action = future_scheduled_actions->allocate();
    } else {
        scheduled_action = scheduled_actions->allocate();
    }

    scheduled_action->seconds_to_run = aci.seconds_to_run;
    _init_action(&scheduled_action->action, aci.action);
    debug_log("scheduling action %s %d", aci.action.name, scheduled_action->action.creation_stamp);
    return scheduled_action;
}

auto Scheduler::_actually_reset() -> void {
    debug_log("HARD RESET SCHEDULER");
    _should_reset = false;
    active_animations->clear();
    scheduled_actions->clear();
    chained_actions->clear();
    _animations_marked_for_deletion.clear();
    _actions_marked_for_deletion.clear();

    // swap ping pong
    Bucket_Allocator<Scheduled_Animation>* t_active_animations = active_animations;
    Bucket_Allocator<Scheduled_Action>*    t_scheduled_actions = scheduled_actions;
    Bucket_Allocator<Action>*              t_chained_actions   = chained_actions;

    active_animations = future_active_animations;
    scheduled_actions = future_scheduled_actions;
    chained_actions   = future_chained_actions;

    future_active_animations = t_active_animations;
    future_scheduled_actions = t_scheduled_actions;
    future_chained_actions   = t_chained_actions;
}

auto Scheduler::reset() -> void {
    if (_is_iterating) {
        debug_log("SOFT RESET Scheduler");
        _should_reset = true;
        future_active_animations->clear();
        future_scheduled_actions->clear();
        future_chained_actions->clear();
    }
    else
        _actually_reset();
}

auto Scheduler::chain_action(Scheduled_Animation* existing_animation,
                             Action_Create_Info aci)
    -> Action*
{
    Action* chain_action;
    if (_should_reset) {
        chain_action = future_chained_actions->allocate();
    } else {
        chain_action = chained_actions->allocate();
    }

    _init_action(chain_action, aci);

    existing_animation->next = chain_action;
    return chain_action;
}

auto Scheduler::chain_action(Action* existing_action, Action_Create_Info aci) -> Action* {
    Action* chain_action;
    if (_should_reset) {
        chain_action = future_chained_actions->allocate();
    } else {
        chain_action = chained_actions->allocate();
    }

    _init_action(chain_action, aci);

    existing_action->next = chain_action;
    return chain_action;
}

inline auto Scheduler::chain_action(Action_Or_Animation aoa, Action_Create_Info aci)
    -> Action*
{
    if (aoa.type == Schedulee_Type::Action)
        return chain_action(aoa.action, aci);
    else if (aoa.type == Schedulee_Type::Animation)
        return chain_action(aoa.animation, aci);
    else if (aoa.type == Schedulee_Type::None) {
        Scheduled_Action* sa = schedule_action({
                .seconds_to_run = 0.0f,
                .action { aci }
            });
        return &sa->action;
    } else {
        panic("unknown Schedulee type");
        return nullptr;
    }
}

inline auto Scheduler::execute_action(Action* action) -> void {
    debug_log("running action %s %d", action->name, action->creation_stamp);

    action->already_ran = true;
    if (action->type == Function_Type::Closure) {
        action->closure(action->param);
    } else {
        action->lambda();
    }

    debug_log("finished running action %s %d", action->name, action->creation_stamp);
}

auto Scheduler::handle_chained_action(Action* action) -> void {
    debug_log("Handling chained actions");
    while (action) {
        debug_log("RUNNING CHAINED %s %d", action->name, action->creation_stamp);
        execute_action(action);

        debug_log("DELETING action %s %d", action->name, action->creation_stamp);
        chained_actions->free_object(action);
        action = action->next;

        if (_should_stop_iterating) break;
    }
    debug_log("Chained actions done");
}

auto Scheduler::run_pending_actions_and_reset() -> void {
    Auto_Array_List<Action*> pending_actions(16);

    debug_log("%{color<}Running pending actions:", console_cyan);
    scheduled_actions->for_each([&](Scheduled_Action* s_action) {
        if (!s_action->action.already_ran) {
            pending_actions.append(&s_action->action);
        }
    });

    active_animations->for_each([&](Scheduled_Animation* s_animation) {
        if (s_animation->next && !s_animation->next->already_ran) {
            pending_actions.append(s_animation->next);
        }
    });

    qsort(pending_actions.data,
          pending_actions.count,
          sizeof(pending_actions.data[0]),
          [] (const void* a1, const void* a2) -> int {
              return (int)((*((Action**)(a1)))->creation_stamp -
                           (*((Action**)(a2)))->creation_stamp);
          });

    for (auto action : pending_actions) {
        execute_action(action);
    }

    debug_log("pending_actions done%{>color}");
    _should_stop_iterating = true;
    reset();
}

auto Scheduler::update_all(f32 delta_time) -> void {

    // advance animations and collect due actions
    {
        active_animations->for_each([=](Scheduled_Animation* anim) {
            anim->seconds_to_start -= delta_time;
            anim->seconds_to_end   -= delta_time;

            advance_single_animation(anim);

            if (anim->seconds_to_end <= 0) {
                // NOTE(Felix): if already finished snap the value to its target
                //   and delete it
                switch (anim->interpolant_type) {
                    case Interpolant_Type::F32:        *(f32*)anim->interpolant =       *(f32*)anim->to; break;
                    default: {
                        size_t interpolantee_size = _interpolatee_sizes[(u8)anim->interpolant_type];
                        panic_if(interpolantee_size == 0,
                                 "Interpolatee (%d) size was 0 (was it initialized correctly)?",
                                 anim->interpolant_type);

                        memcpy(anim->interpolant, anim->to, interpolantee_size);
                    }
                }

                _animations_marked_for_deletion.append(anim);
                // NOTE(Felix): Don't actually run the next here, queue it
                //   up for after the iteration, see note below.
                if (anim->next) {
                    _chained_actions_to_be_run_after_iteration.append(anim->next);
                }
            }
        });
        scheduled_actions->for_each([=](Scheduled_Action* action) {
            action->seconds_to_run -= delta_time;
            if (action->seconds_to_run <= 0) {
                _chained_actions_to_be_run_after_iteration.append(&action->action);
                _actions_marked_for_deletion.append(action);
            }
        });
    }


    // NOTE(Felix): The reason why we iterate over the chained actions like
    //   this: Imagine the following scenario: We schedule two animations
    //   that take the same time to run should run in parallel. So what you
    //   do for the first animation is put their parenting action (not the
    //   unparenting action) as the last action in the current game state
    //   and schedule their animation and then chain the unparenting action
    //   to the animation. The gamestate however only knows about the
    //   parenting action, so that the next thing that is scheduled runs
    //   parallel to it. For the second animation, the parenting action is
    //   scheduled onto the parenting action of the first animation, the
    //   animation is scheduled, and then also the unparenting action is
    //   chained to the animation. This time we set the Game_State
    //   last_scheduled to the unparenting action, so that no further
    //   animation will happen in parallel. Now imagine we chain some
    //   animation to the game state, that involves the animation of the
    //   first object. It can happen, when iterating through the animations,
    //   that we iterate first over the second animation, would then run
    //   their chained actions, in which we would animate the first object.
    //   This is bad, as they are not finished yet, their unparenting action
    //   did not run yet. Now we "solve" this by iterating breadth first
    //   over the actions.
    if (_chained_actions_to_be_run_after_iteration.count > 1) {
        qsort(_chained_actions_to_be_run_after_iteration.data,
              _chained_actions_to_be_run_after_iteration.count,
              sizeof(_chained_actions_to_be_run_after_iteration.data[0]),
              [] (const void* a1, const void* a2) -> int {
                  return (int)((*((Action**)(a1)))->creation_stamp -
                               (*((Action**)(a2)))->creation_stamp);
              });
    }
    // excute all due actions:
    {
        _is_iterating = true;
        _should_stop_iterating = false;
        for (Action* action : _chained_actions_to_be_run_after_iteration) {
            if (action->already_ran) continue;

            execute_action(action);

            if (_should_stop_iterating) break;

            handle_chained_action(action->next);

            if (_should_stop_iterating) break;
        }
        _is_iterating = false;
        _chained_actions_to_be_run_after_iteration.clear();
    }


    // NOTE(Felix): if Scheduler::reset was called in an action (happens
    //   when scheduling a level switch), then don't actually reset
    //   everyting, but only set _should_reset. Because if we would actually
    //   clear all the bucket allocators, we would then anyway call
    //   free_object on them down when we process the _marked_for_deletion
    //   things, so we would have stuff in the free list that is not
    //   actually allocated
    if (_should_reset) {
        _actually_reset();
    } else {
        // NOTE(Felix): Don't free stuff from the bucket_allocators while
        //   iterating over them, as this appends the pointers to the free_list,
        //   which then is no longer sorted, which destroys the mechanism to
        //   check if a bucket is active or freed, so for_each will be called
        //   for buckets that are no longer alive. -Felix Dec 18 2020
        for (auto anim : _animations_marked_for_deletion) {
            active_animations->free_object(anim);
        }
        for (auto action : _actions_marked_for_deletion) {
            scheduled_actions->free_object(action);
        }
        _animations_marked_for_deletion.clear();
        _actions_marked_for_deletion.clear();
    }
}

#endif // FTB_SCHEDULER_IMPL
