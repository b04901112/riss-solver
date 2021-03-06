/* Part of the generic incremental SAT API called 'ipasir'.
 *
 * This LICENSE applies to all software included in the IPASIR distribution,
 * except for those parts in sub-directories or in included software
 * distribution packages, such as tar and zip files, which have their own
 * license restrictions.  Those license restrictions are usually listed in the
 * corresponding LICENSE or COPYING files, either in the sub-directory or in
 * the included software distribution package (the tar or zip file).  Please
 * refer to those licenses for rights to use that software.
 *
 * Copyright (c) 2014, Tomas Balyo, Karlsruhe Institute of Technology.
 * Copyright (c) 2014, Armin Biere, Johannes Kepler University.
 * Copyright (c) 2015, Norbert Manthey
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include "ipasir.h"
#include "riss/librissc.h" // include actual C-interface of Riss
#include <stdlib.h>        // getenv

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return the name and the version of the incremental SAT
 * solving library.
 */
const char * ipasir_signature()
{
    return riss_signature();
}

/**
 * Construct a new solver and return a pointer to it.
 * Use the returned pointer as the first parameter in each
 * of the following functions.
 *
 * Required state: N/A
 * State after: INPUT
 */
void * ipasir_init()
{
    char* env_config = getenv("RISSCONFIG");
    const char* rissconfig = env_config == 0 ? 0 : env_config; // old default config, possible buggy: INCSOLVE:-init-act=3:-actStart=2048:INCSIMP
    return riss_init_configured(rissconfig); // use riss with the configuration for incremental solving, and incremental simplification (after 50000 conflicts)
}

/**
 * Release the solver, i.e., all its resoruces and
 * allocated memory (destructor). The solver pointer
 * cannot be used for any purposes after this call.
 *
 * Required state: INPUT or SAT or UNSAT
 * State after: undefined
 */
void ipasir_release(void * solver)
{
    riss_destroy(&solver);
}

/**
 * Add the given literal into the currently added clause
 * or finalize the clause with a 0.  Clauses added this way
 * cannot be removed. The addition of removable clauses
 * can be simulated using activation literals and assumptions.
 *
 * Required state: INPUT or SAT or UNSAT
 * State after: INPUT
 *
 * Literals are encoded as (non-zero) integers as in the
 * DIMACS formats.  They have to be smaller or equal to
 * INT_MAX and strictly larger than INT_MIN (to avoid
 * negation overflow).  This applies to all the literal
 * arguments in API functions.
 */
void ipasir_add(void * solver, int lit_or_zero)
{
    riss_add(solver, lit_or_zero);
}

/**
 * Add an assumption for the next SAT search (the next call
 * of ipasir_solve). After calling ipasir_solve all the
 * previously added assumptions are cleared.
 *
 * Required state: INPUT or SAT or UNSAT
 * State after: INPUT
 */
void ipasir_assume(void * solver, int lit)
{
    riss_assume(solver, lit);
}

/**
 * Solve the formula with specified clauses under the specified assumptions.
 * If the formula is satisfiable the function returns 10 and the state of the solver is changed to SAT.
 * If the formula is unsatisfiable the function returns 20 and the state of the solver is changed to UNSAT.
 * If the search is interrupted (see ipasir_set_terminate) the function returns 0 and the state of the solver remains INPUT.
 * This function can be called in any defined state of the solver.
 *
 * Required state: INPUT or SAT or UNSAT
 * State after: INPUT or SAT or UNSAT
 */
int ipasir_solve(void * solver)
{
    return riss_sat(solver);
}

int ipasir_solve_limited(void * solver, const int64_t nOfConflicts)
{
    return riss_sat_limited(solver, nOfConflicts);
}

/**
 * Get the truth value of the given literal in the found satisfying
 * assignment. Return 'lit' if True, '-lit' if False, and 0 if not important.
 * This function can only be used if ipasir_solve has returned 10
 * and no 'ipasir_add' nor 'ipasir_assume' has been called
 * since then, i.e., the state of the solver is SAT.
 *
 * Required state: SAT
 * State after: SAT
 */
int ipasir_val(void * solver, int lit)
{
    return riss_deref(solver, lit) > 0 ? lit : -lit;
}

/**
 * Check if the given assumption literal was used to prove the
 * unsatisfiability of the formula under the assumptions
 * used for the last SAT search. Return 1 if so, 0 otherwise.
 * This function can only be used if ipasir_solve has returned 20 and
 * no ipasir_add or ipasir_assume has been called since then, i.e.,
 * the state of the solver is UNSAT.
 *
 * Required state: UNSAT
 * State after: UNSAT
 */
int ipasir_failed(void * solver, int lit)
{
// Note: Riss will convert the literal into a variable, hence the check works only for variables, not for literals (if the complement of an assumption is tested)
    return riss_assumption_failed(solver, lit);
}

/**
 * Set a callback function used to indicate a termination requirement to the
 * solver. The solver will periodically call this function and check its return
 * value during the search. The ipasir_set_terminate function can be called in any
 * state of the solver, the state remains unchanged after the call.
 * The callback function is of the form "int terminate(void * state)"
 *   - it returns a non-zero value if the solver should terminate.
 *   - the solver calls the callback function with the parameter "state"
 *     having the value passed in the ipasir_set_terminate function (2nd parameter).
 *
 * Required state: INPUT or SAT or UNSAT
 * State after: INPUT or SAT or UNSAT
 */
void ipasir_set_terminate(void * solver, void * state, int (*terminate)(void * state))
{
    riss_set_termination_callback(solver, state, terminate);
}

/**
 * Set a callback function used to extract learned clauses up to a given lenght from the
 * solver. The solver will call this function for each learned clause that satisfies
 * the maximum length (literal count) condition. The ipasir_set_learn function can be called in any
 * state of the solver, the state remains unchanged after the call.
 * The callback function is of the form "void learn(void * state, int * clause)"
 *   - the solver calls the callback function with the parameter "state"
 *     having the value passed in the ipasir_set_terminate function (2nd parameter).
 *   - the argument "clause" is a pointer to a null terminated integer array containing the learned clause.
 *     the solver can change the data at the memory location that "clause" points to after the function call.
 *
 * Required state: INPUT or SAT or UNSAT
 * State after: INPUT or SAT or UNSAT
 */
void ipasir_set_learn(void * solver, void * state, int max_length, void (*learn)(void * state, int * clause))
{
    riss_set_learn_callback(solver, state, max_length, learn);
}

/** apply unit propagation (find units, not shrink clauses) and remove satisfied (learned) clauses from solver
 * @return 1, if simplification did not reveal an empty clause, 0 if an empty clause was found (or inconsistency by unit propagation)
 */
int ipasir_simplify(void * solver)
{
    return riss_simplify(solver);
}

int ipasir_get_nvar(void * solver)
{
    return riss_get_nvar(solver);
}

void ipasir_set_nvar(void * solver, int n)
{
    riss_set_nvar(solver, n);
}

#ifdef __cplusplus
}
#endif
