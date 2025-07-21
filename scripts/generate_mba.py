"""
MBA Linear Expression Generator

This script generates equal linear Boolean expressions using
basic expressions (such as `x&y`, `x|~y`, etc.). These expressions are
useful in reverse engineering, code obfuscation,
and symbolic analysis.

The core of the algorithm is based on solving linear equations 
in ℤ using Boolean algebra. The expressions generated are
obfuscations of real simple expressions, preserving their behaviour
for any possible input.

Dependencies:
    - SymPy (symbolic computation, nullspace)
    - Z3 (not actively used here, but intended for SMT verification)
    - pandas, tqdm, argparse

Usage:
    python script.py --numOfVars 2 --numOfTerms 5

Output:
    CSV file with two columns: [Obfuscated, Original]
"""

import os
import argparse
import pandas as pd

from z3 import *
from sympy import Matrix, zeros, lcm
from sympy.logic.boolalg import Or, And, Not, BooleanTrue, BooleanFalse
from sympy.abc import x, y, z
from itertools import product, combinations_with_replacement
from tqdm import tqdm
from datetime import datetime

class ExpressionGenerator:
    """
    ExpressionGenerator generates valid linear MBA (Mixed Boolean-Arithmetic) expressions.

    An MBA expression is an equality of the form:
        Σ ci * Ei(x, y, ...) = Σ cj * Ej(x, y, ...)
    where Ei and Ej are Boolean expressions over n variables, and ci, cj are integers.

    This class takes a set of reduced Boolean expressions, selects combinations of them
    of fixed size (nTerms), and uses linear algebra to find a combination of coefficients
    such that the weighted sum of the expressions is identical on both sides (equality of truth vectors).

    Attributes :
        - nVars (int): Number of Boolean variables (e.g., 2 for x and y).
        - nTerms (int): Number of terms to combine in each MBA.
        - exprs (dict): Dictionary of Boolean expressions (in string form) and their truth table.
        - indexes (list): List of expression index combinations to be tested.
        - result (list): List of pairs (obfuscated_expr, original_expr) generated.
    """

    def __init__(self, nVars: int, nTerms: int):
        self.nVars = nVars
        self.nTerms = nTerms
        self.indexes = []
        self.result = []

        # A dictionnary of boolean expressions and their corresponding truth tables.
        # A bitwise expression En with n variables has 2^(2^n) different reduced Boolean expression
        self.exprs = {
            "x": [0, 0, 1, 1], "y": [0, 1, 0, 1], "-1": [1, 1, 1, 1], "~x": [1, 1, 0, 0], "~y": [1, 0, 1, 0],
            "x&y": [0, 0, 0, 1], "~x&y": [0, 1, 0, 0], "x&~y": [0, 0, 1, 0], "~(x&y)": [1, 1, 1, 0],
            "x|y": [0, 1, 1, 1], "~x|y": [1, 1, 0, 1], "x|~y": [1, 0, 1, 1], "~(x|y)": [1, 0, 0, 0],
            "x^y": [0, 1, 1, 0], "~x^y": [1, 0, 0, 1], "x^~y": [1, 0, 0, 1], "~(x^y)": [1, 0, 0, 1]
        }
        # self.exprs = self.generate_all_exprs()

    # CA MARCHE PAS
    def generate_all_exprs(self):
        """
        Automatically generate all simplified boolean expressions with their truth tables
        for the given number of variables (e.g., x, y, z).
        """
        # Define variables dynamically
        var_symbols = [x, y, z][:self.nVars]

        # Generate all possible input combinations (truth assignments)
        input_combinations = list(product([False, True], repeat=self.nVars))

        # Generate candidate expressions
        basic_ops = [lambda a: a, Not]
        binary_ops = [And, Or, Xor]

        # Set to store unique truth tables to avoid duplicates
        unique_tables = {}
        
        # Generate all combinations of operations (up to 2-variable expressions)
        for a, b in combinations_with_replacement(var_symbols, 2):
            for bin_op in binary_ops:
                for una_a in basic_ops:
                    for una_b in basic_ops:
                        try:
                            expr = bin_op(una_a(a), una_b(b))
                            truth = tuple(bool(expr.subs(dict(zip(var_symbols, inputs)))) for inputs in input_combinations)
                            if truth not in unique_tables.values():
                                unique_tables[str(expr)] = truth
                        except Exception:
                            continue

        # Add constants manually
        unique_tables[str(BooleanTrue())] = tuple([True] * len(input_combinations))
        unique_tables[str(BooleanFalse())] = tuple([False] * len(input_combinations))

        # Replace original dictionary
        self.exprs = {k: [int(v) for v in table] for k, table in unique_tables.items()}

    def index_combine(self, start: int, tmp: list):
        """
        Recursively generate all non-repeating combinations of `nTerms` expression indexes.

        Args:
            start (int): Current starting index.
            tmp (list): Temporary list of indexes being built.

        Implementation detail:
            - To reduce search space, we step by 10^(nVars - 2).
            This skips similar patterns to avoid redundant MBA generation,
            acting as a crude form of pruning.
        """
       
        if len(tmp) == self.nTerms:
            self.indexes.append(list(tmp))
            return

        i = start
        while i < len(self.exprs):
            tmp.append(i)
            self.index_combine(i + 1, tmp)
            tmp.pop()
            i += 10 ** (self.nVars - 2)

    def expression_generate(self, indexOfExprs: list, v: list):
        """
        Converts a set of expression indexes (in exprs) and their associated coefficients
        into a character string representing a valid arithmetic equality between two MBA expressions.

        The distribution of terms between the left and right sides depends on their order to simulate an asymmetric form
        while retaining the mathematical truth: same result on both sides.

        This function also ensures that the variables `x`, `y`, and `z` appear in the expression
        to avoid constant, trivial or non-variable equalities.
        """

        def construct_expression(coeff, sign, left, right):
            if left:
                left += "+" + coeff if sign else "-" + coeff
            else:
                left = coeff if sign else "-" + coeff
            return left, right

        def ensure_variable_inclusion(left, right, var):
            if (var not in left) and (var not in right):
                left += "+" + var
                right += "+" + var
            return left, right

        left, right = "", ""
        keys = list(self.exprs.keys())

        for i, j in enumerate(indexOfExprs):
            coeff = keys[j] if j < len(keys) else "-1"
            sign = (v[i] >= 0)

            if len(coeff) > 1:
                coeff = f"({coeff})"

            if coeff == "-1":
                coeff = str(abs(v[i]))
            elif abs(v[i]) > 1:
                coeff = f"{abs(v[i])}*{coeff}"

            if j < 2 or not left:
                left, right = construct_expression(coeff, sign, left, right)
            else:
                right, left = construct_expression(coeff, not sign, right, left)

        # Ensure that x and y variables are included in both sides of the expression
        left, right = ensure_variable_inclusion(left, right, "x")
        left, right = ensure_variable_inclusion(left, right, "y")
        if self.nVars > 2:
            left, right = ensure_variable_inclusion(left, right, "z")

        # Clean up unnecessary parentheses
        # Even if we can keep them to make the MBA more hard to read
        if ("+" not in left) and ("-" not in left) and ("*" not in left) and left.startswith("(") and left.endswith(")"):
            left = left[1:-1]

        self.result.append([left, right])

    def first_zero_index(self, v):
        """
        Find the index of the first zero in a vector
        """
        for i in range(len(v)):
            if v[i] == 0:
                return i
        return -1

    def compute_nullspace_vector(self, F: Matrix):
        """
        Computes a vector v ≠ 0 such that F * v = 0, i.e. a vector in the kernel of F.
        
        This vector v is used to combine the columns (i.e., truth vectors of expressions) 
        linearly to obtain a zero sum - meaning that the left 
        and right sides of a future equality will give the same result for all combinations 
        of Boolean inputs.

        The aim is to generate non-trivial integer coefficients to construct an MBA.

        Algorithmically:
            - Get the basis of the kernel of F via `nullspace()`.
            - Attempt to construct a combination of these vectors that contains no zeros (avoids null terms).
            - If this is impossible, we return a null vector, which means "no usable solution".
        """
        
        solutions = F.nullspace()
        v = zeros(self.nTerms, 1)

        for s in solutions:
            v += s

        # Find the index of the first zero-value in vector where v is a solution of Fv=0
        idx = self.first_zero_index(v)

        # Generate non-zero matrix v, if there is no 0 in v
        # v is the final solution we have to return, else we change
        # v through the linear combination of s in solutions.
        while idx != -1:
            if len(solutions) < 2:
                return zeros(v.shape[0], v.shape[1])

            has_non_zero_null_space = any(s[idx] != 0 for s in solutions)
            if not has_non_zero_null_space:
                return zeros(v.shape[0], v.shape[1])

            # Add non-zero value of nullspace to v
            for s in solutions:
                if s[idx] != 0:
                    v += s
                    break
            
            # Try to eliminate the next 0 in v
            idx = self.first_zero_index(v)

        return v

    def generate_expressions(self):
        """
        Generates all valid MBA expressions by :
        - Iterating over combinations of Boolean expressions (`nTerms` among `exprs`)
        - Constructing a matrix F of their truth vectors
        - Solving F * v = 0 to find a valid linear combination
        - Generating symbolic expressions via `expression_generate`
        """

        self.index_combine(0, [])

        for index in tqdm(self.indexes, desc="Generating expressions"):
            F = Matrix()

            # Construct the matrix F based on the selected expressions
            for j in index:
                key = list(self.exprs.keys())[j]
                F = F.col_insert(F.shape[1], Matrix(self.exprs[key]))

            v = self.compute_nullspace_vector(F)
            if v == zeros(self.nTerms, 1):
                continue # Skip if no valid solution is found !

            # Scale the nullspace vector to have integer coefficients
            scale = lcm([val.q for val in v if val.q != 0])
            v = scale * v

            # Flatten the solution vector and generate the corresponding expression
            flat_v = [int(v[i]) for i in range(v.rows)]
            self.expression_generate(index, flat_v)

# References:
#      - https://aclanthology.org/2020.findings-emnlp.56.pdf
#      - https://link.springer.com/chapter/10.1007/978-3-540-77535-5_5
#
# This script is a fork of the NeoReduce paper
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate MBA dataset")
    parser.add_argument("--numOfTerms", type=int, default=5, help="Number of terms")
    parser.add_argument("--numOfVars", type=int, default=2, help="Number of variables")

    args = parser.parse_args()
    
    if args.numOfVars >= 2:
        datasetPath = f"../resources/mba-dataset-{args.numOfVars}-{args.numOfTerms}-{datetime.now().strftime('%Y%m%d')}.csv"
        if not os.path.exists(datasetPath):
            generator = ExpressionGenerator(nVars=args.numOfVars, nTerms=args.numOfTerms)
            generator.generate_expressions()

            df = pd.DataFrame(generator.result, columns=["Obfuscated", "Original"])
            df.to_csv(datasetPath, mode="a+", header=True, index=False)

            # This part is for SMT validation
            # x = BitVec('x', 32)
            # y = BitVec('y', 32)
            # z = BitVec('z', 32)

            # for i, (origin, obfuscated) in enumerate(generator.result):
            #     print(f"No.{i}:", end=" ")
            #     solve(eval(origin) != eval(obfuscated))

            print("Dataset generation completed.")
        else:
            print(f"{datasetPath} already exist.")
    else:
        print("At least 2 variables are required to generate meaningful MBA expressions.")