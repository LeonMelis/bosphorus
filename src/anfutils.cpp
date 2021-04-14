/*****************************************************************************
Copyright (C) 2018  Davin Choo, Kian Ming A. Chai, DSO National Laboratories

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
***********************************************/

#include <iostream>
#include <unordered_set>

#include "anfutils.hpp"

using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using std::swap;
using std::unordered_set;
using std::vector;

USING_NAMESPACE_PBORI
using namespace BLib;

pair<bool, double> BLib::if_sample_and_clone(const vector<BoolePolynomial>& eqs,
                                       double log2size)
{
    const polybori::BoolePolyRing& ring(eqs.front().ring());
    const size_t log2fullsz =
        log2(eqs.size()) +
        2 * log2(ring.nVariables()); // assume quadratic equations only
    return make_pair(log2fullsz > log2size, log2fullsz);
}

double BLib::sample_and_clone(const uint32_t verbosity,
                        const vector<BoolePolynomial>& eqs,
                        vector<BoolePolynomial>& equations, double log2size)
{
    auto ret = if_sample_and_clone(eqs, log2size);
    if (!ret.first) {
        // Small system, so clone the entire system
        equations = eqs;
        return ret.second;
    } else {
        return do_sample_and_clone(verbosity, eqs, equations, log2size);
    }
}

double BLib::do_sample_and_clone(const uint32_t verbosity,
                           const vector<BoolePolynomial>& eqs,
                           vector<BoolePolynomial>& equations, double log2size)
{
    assert(equations.empty());
    // fill an indexing vector with identity
    vector<size_t> idx(eqs.size());
    for (size_t i = 0; i < eqs.size(); ++i)
        idx[i] = i;

    // randomly select equations until a limit
    unordered_set<BooleMonomial::hash_type> unique;
    double log2uniquesz = 0;
    size_t sampled = 1, reject = 0;
    double rej_rate = 0;
    do {
        rej_rate = static_cast<double>(reject) / sampled;
        size_t sel =
            std::floor(static_cast<double>(rand()) / RAND_MAX * idx.size());
        const BoolePolynomial& poly(eqs[idx[sel]]);
        ++sampled;
        if (!unique.empty() && rej_rate < 0.8) {
            // accept with probability of not increasing then number of monomials
            size_t out = 0;
            for (const BooleMonomial& mono : poly)
                if (unique.find(mono.hash()) == unique.end())
                    ++out;
            if (static_cast<double>(rand()) / RAND_MAX <
                static_cast<double>(out) / poly.length()) {
                ++reject;
                continue; // reject and continue with do-while loop
            }
        }
        equations.push_back(poly);
        swap(idx.back(), idx[sel]);
        idx.pop_back();
        for (const BooleMonomial& mono : equations.back())
            unique.insert(mono.hash());
        log2uniquesz = log2(unique.size());
    } while ((log2(equations.size()) + log2uniquesz < log2size) &&
             (idx.size() > 0));
    if (verbosity >= 3)
        cout << "c  Selected " << equations.size() << '[' << unique.size()
             << "] equations with rejection rate " << rej_rate << endl;

    return log2uniquesz;
}

void BLib::subsitute(const BooleVariable& from_var, const BoolePolynomial& to_poly,
               BoolePolynomial& poly)
{
    BoolePolynomial quotient = poly / from_var;

    /*if( quotient.isZero() ) // don't need this check
    return;
  else*/
    if (poly.isSingleton()) // for some reason, we need this special case to prevent segfault in some cases
        poly = quotient * to_poly;
    else if (quotient.isOne()) { // ditto
        poly -= from_var;
        poly += to_poly;
    } else {
        // construct reminder manually to prevent segfault
        quotient *= to_poly;
        for (const BooleMonomial& mono : poly) {
            if (!mono.reducibleBy(from_var)) {
                quotient += mono;
            }
        }
        swap(quotient, poly); // because we are returning poly
    }
}

/**
 * Substitute variable 'from_var' in 'target' with 'to_poly'
 *
 * @param from_var the variable to replace
 * @param to_poly the target to replace with
 * @param target pointer to the target to perform the replacement on (like 'this' in our Python ZDD implementation)
 */
BoolePolynomial BLib::substitute(const BooleVariable& from_var,
                                 const BoolePolynomial& to_poly, BoolePolynomial& target)
{
    if (target
            .isSingleton()) // for some reason, we need this special case to prevent segfault in some cases
        return target;

    const BooleVariable var = target.usedVariables().firstVariable();

    if (var < from_var) {
        BoolePolynomial lo = BoolePolynomial(target.navigation().elseBranch(), target.ring());
        BoolePolynomial hi = BoolePolynomial(target.navigation().thenBranch(), target.ring());

        lo = substitute(from_var, to_poly, lo);
        hi = substitute(from_var, to_poly, hi);

        return lo + (var * hi);
    } else if (var > from_var) {
        return target;
    } else if (var == from_var) {
        BoolePolynomial lo = BoolePolynomial(target.navigation().elseBranch(), target.ring());
        BoolePolynomial hi = BoolePolynomial(target.navigation().thenBranch(), target.ring());

        return lo + (hi * to_poly);
    }

    return target;
}


void BLib::substitute_in_place(const BooleVariable& from_var,
                                 const BoolePolynomial& to_poly, BoolePolynomial& target)
{
    if (target
            .isSingleton()) // for some reason, we need this special case to prevent segfault in some cases
        return;

    const BooleVariable var = target.usedVariables().firstVariable();

    if (var < from_var) {
        BoolePolynomial lo = BoolePolynomial(target.navigation().elseBranch(), target.ring());
        BoolePolynomial hi = BoolePolynomial(target.navigation().thenBranch(), target.ring());

        substitute(from_var, to_poly, lo);
        substitute(from_var, to_poly, hi);

        target = lo + (var * hi);
    } else if (var > from_var) {
        return;
    } else if (var == from_var) {
        BoolePolynomial lo = BoolePolynomial(target.navigation().elseBranch(), target.ring());
        BoolePolynomial hi = BoolePolynomial(target.navigation().thenBranch(), target.ring());

        target = lo + (hi * to_poly);
    }
}