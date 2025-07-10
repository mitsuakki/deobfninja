import os
import csv
import random
import argparse
import pandas as pd

from z3 import *
from sympy import Matrix, zeros, lcm
from tqdm import tqdm
from datetime import datetime

class ExpressionGenerator:
    """
    Generate linear MBA expressions.
    Each term in the expression is a simple form of two Boolean variable expressions.

    Attributes:
        nVars: The number of boolean variables (e.g, 1 for x and y, 3 for x, y and z).
        nTerms: The number of terms in the expression.
        exprs: Basic expressions and their truth tables. '-1' represents a constant.
        indexes: All possible combinations of expressions from exprs.
        result: A list to store the generated expressions.
    """

    def __init__(self, nVars: int, nTerms: int):
        self.nVars = nVars
        self.nTerms = nTerms
        self.indexes = []
        self.result = []

        # A dictionnary of boolean expressions and their corresponding truth tables.
        # A bitwise expression En with n variables has 2^(2^n) different reduced Boolean expression
        # ToDo: Generate all of them automatically using a library like sympy or with a custom function.
        self.exprs = {
            "x": [0, 0, 1, 1], "y": [0, 1, 0, 1], "-1": [1, 1, 1, 1], "~x": [1, 1, 0, 0], "~y": [1, 0, 1, 0],
            "x&y": [0, 0, 0, 1], "~x&y": [0, 1, 0, 0], "x&~y": [0, 0, 1, 0], "~(x&y)": [1, 1, 1, 0],
            "x|y": [0, 1, 1, 1], "~x|y": [1, 1, 0, 1], "x|~y": [1, 0, 1, 1], "~(x|y)": [1, 0, 0, 0],
            "x^y": [0, 1, 1, 0], "~x^y": [1, 0, 0, 1], "x^~y": [1, 0, 0, 1], "~(x^y)": [1, 0, 0, 1]
        }

    def index_combine(self, start: int, tmp: list):
        """
        Generate all combinations of non-repeating nTerms expressions from exprs.
        Store all combinations in self.indexes.
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
        Generate an MBA expression using a combination of expression indexes and corresponding coefficients.
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

        # Ouais je sais mais c'est inversé plus parlant pour le dataset
        # Et j'ai la flemme de renommer désolé...
        self.result.append([right, left])

    def first_zero_index(self, v):
        """
        Find the index of the first zero in a vector
        """

        for i in range(len(v)):
            if v[i] == 0:
                return i
        return -1
<<<<<<< Updated upstream

    def compute_v(self, F: Matrix):
=======
 
    def compute_nullspace_vector(self, F: Matrix):
>>>>>>> Stashed changes
        """
        Compute the nullspace vector v with F * v = 0.
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

    def generate(self):
        """
        Generate MBA expressions for all possible combinations of terms in exprs.
        """
        self.index_combine(0, [])

        for index in tqdm(self.indexes, desc="Generating expressions"):
            F = Matrix()

            # Construct the matrix F based on the selected expressions
            for j in index:
                key = list(self.exprs.keys())[j]
                F = F.col_insert(F.shape[1], Matrix(self.exprs[key]))

            v = self.compute_v(F)
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
            generator.generate()

            df = pd.DataFrame(generator.result, columns=["Original", "Obfuscated"])
            df.to_csv(datasetPath, mode="a+", header=True, index=False)

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