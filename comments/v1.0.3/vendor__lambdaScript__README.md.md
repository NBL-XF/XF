# Comments extracted from `vendor/lambdaScript/README.md`

Version: `v1.0.3`

Source: `vendor/lambdaScript/README.md`

## Comment 1

LambdaScript

## Comment 2

# Table of Contents

## Comment 3

# Why LambdaScript

## Comment 4

-

## Comment 5

# Current Architecture

## Comment 6

-

## Comment 7

# Getting Started

## Comment 8

# CLI Usage

## Comment 9

no-prelude   disable built-in I/K/S/TRUE/FALSE
            stop option parsing; remaining values become ARGS

## Comment 10

## Script arguments

## Comment 11

# A First LambdaScript Program

## Comment 12

-

## Comment 13

# Language Basics

## Comment 14

## Lambda abstraction

## Comment 15

## Function application

## Comment 16

## Comments

## Comment 17

double-dash comment

## Comment 18

## Literals and identifiers

## Comment 19

## Program shape

## Comment 20

# Definitions

## Comment 21

## Top-level compound definitions

## Comment 22

# Booleans and Logic Operators

## Comment 23

## Logic functions

## Comment 24

## Logic and equivalence operators

## Comment 25

# Branching

## Comment 26

-

## Comment 27

# Pairs

## Comment 28

-

## Comment 29

# Church Numerals

## Comment 30

-

## Comment 31

# Numeric and Symbolic Math

## Comment 32

## Numeric literals and arithmetic

## Comment 33

## Symbolic math forms

## Comment 34

## Quantifiers and membership syntax

## Comment 35

## Actionable subscripts

## Comment 36

# Loops and Iteration

## Comment 37

## 1. Bounded loops with Church numerals

## Comment 38

## 2. Recursive loops with fixed-point combinators

## Comment 39

-

## Comment 40

# Common Algorithms

## Comment 41

## Identity

## Comment 42

## Constant selection

## Comment 43

## Composition

## Comment 44

## Boolean negation

## Comment 45

## Numeric arithmetic

## Comment 46

## Symbolic sigma

## Comment 47

## Quantified logic

## Comment 48

## Pair projection

## Comment 49

## Bounded iteration

## Comment 50

## Numeric equality

## Comment 51

# Worked Examples

## Comment 52

## Example 1A: Quantified logic over a finite domain

## Comment 53

## Example 1: Identity smoke test

## Comment 54

-

## Comment 55

## Example 2: Constant selection

## Comment 56

-

## Comment 57

## Example 3: Boolean display with `SHOW`

## Comment 58

-

## Comment 59

## Example 4: Logic operator smoke test

## Comment 60

-

## Comment 61

## Example 5: Branching with Church booleans

## Comment 62

-

## Comment 63

## Example 6: Logic gate bundle

## Comment 64

-

## Comment 65

## Example 7: Pairs as tiny records

## Comment 66

-

## Comment 67

## Example 8: Symbolic function composition

## Comment 68

-

## Comment 69

## Example 9: Church numerals as bounded loops

## Comment 70

-

## Comment 71

## Example 10: Increment-like symbolic loop

## Comment 72

-

## Comment 73

## Example 11: Church addition

## Comment 74

-

## Comment 75

## Example 12: Church multiplication

## Comment 76

-

## Comment 77

## Example 13: Zero check

## Comment 78

-

## Comment 79

## Example 14: Predecessor

## Comment 80

-

## Comment 81

## Example 15: Numeric equality

## Comment 82

-

## Comment 83

## Example 16: Pair-encoded loop state

## Comment 84

-

## Comment 85

## Example 17: Symbolic transformation pipeline

## Comment 86

-

## Comment 87

## Example 18: Symbolic model expression

## Comment 88

-

## Comment 89

## Example 19: Factorial with a fixed-point combinator

## Comment 90

-

## Comment 91

## Example 20: Full operator and algorithm smoke program

## Comment 92

# Testing

## Comment 93

## `tests/math_ascii_aliases.lambda`

## Comment 94

## `tests/math_ops.lambda`

## Comment 95

## `tests/math_forms.lambda`

## Comment 96

## `tests/subscripts.lambda`

## Comment 97

## `tests/torture.lambda`

## Comment 98

# XF Integration

## Comment 99

-

## Comment 100

# Symbolic Models

## Comment 101

-

## Comment 102

# Tips for Writing Good LambdaScript

## Comment 103

## 1. Keep definitions small

## Comment 104

## 2. Put definitions before use

## Comment 105

## 3. Use `SHOW` for booleans

## Comment 106

## 4. Display numerals with symbolic step names

## Comment 107

## 5. Treat hard recursion as a stress test

## Comment 108

## 6. Keep runnable examples single-line per definition

## Comment 109

-

## Comment 110

# Troubleshooting

## Comment 111

## `unexpected trailing input`

## Comment 112

## `expected ')' at byte ...`

## Comment 113

## Raw output like `\x0 x1.x0`

## Comment 114

## Symbolic output like `sigma ...`, `integral ...`, `limit ...`, `sub ...`

## Comment 115

## Very long reduction or step-limit warning
