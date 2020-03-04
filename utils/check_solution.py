#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2014  Mate Soos
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.


from __future__ import with_statement  # Required in 2.5
from __future__ import print_function
import sys
import random

def evaluate(monom, sol):
    # the constant monomial
    if monom == "1":
        return 1

    val = 1
    monom = monom.split("*")
    for var in monom:
        var = var.replace("x", "").replace("(", "").replace(")", "").strip()
        try:
            v = int(var)
        except ValueError:
            print("ERROR: Equation file contains non-integer monomial var '%s' in monomial '%s' " % (var, monom))
            exit(-1)

        if v not in sol:
            print("ERROR: variable %d in ANF but not in solution" % v)
            exit(-1)

        val *= sol[v]

    return val


if len(sys.argv) != 3:
    print("Usage: check_solution.py solution ANF")
    exit(-1)

solfile = sys.argv[1]
anffile = sys.argv[2]

sol = {}
unsat = None
with open(solfile, "r") as f:
    for line in f:
        line = line.strip()

        if line == "s ANF-SATISFIABLE":
            unsat = False

        if line == "s ANF-UNSATISFIABLE":
            unsat = True
            print("Unsatisfiable, cannot check solution")
            exit(0)

        if len(line) == 0:
            continue

        if line[0] != "v":
            continue

        line = line.split()
        for x in line:
            if x == "v":
                continue

            x = x.split("+")
            rhs = False
            if x[0] == "1":
                rhs = True
                x[0] = x[1]

            x = x[0].replace("x", "").replace("(", "").replace(")", "").strip()
            if len(x) == 0:
                continue
            try:
                var = abs(int(x))
            except ValueError:
                print("ERROR: solution file contains non-integer variable value: ", x)
                exit(-1)

            sol[var] = rhs

if unsat is None:
    print("ERROR: Solution file does not say ANF-SATISFIABLE or ANF-UNSATISFIABLE")
    print("ERROR: maybe it's corrupt?")
    exit(-1)
print("Solution:", sol)


with open(anffile, "r") as f:
    for line in f:
        line = line.strip()
        monoms = line.split("+")
        monoms = [m.strip() for m in monoms]
        val = 0
        for mon in monoms:
            val ^= evaluate(mon, sol)

        if val != 0:
            print("ERROR: Equation not satisfied: ", line)
            for mon in monoms:
                print("--> Monomial '%s' evaluates to: %s" % (mon, evaluate(mon, sol)))
            exit(-1)

print("OK, solution is good!")
