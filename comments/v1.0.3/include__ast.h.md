# Comments extracted from `include/ast.h`

Version: `v1.0.3`

Source: `include/ast.h`

## Comment 1

============================================================
xf AST

Every node carries a Loc for error reporting.
Nodes are heap-allocated and owned by the tree.

Ownership rules:
  - AST constructors take ownership of child Expr, Stmt, and TopLevel
    pointers and of heap arrays passed to them.
  - Constructors taking xf_Str* retain those strings unless the
    implementation explicitly documents transfer-of-ownership.
  - ast_*_free() recursively frees owned children.

Program source:
  - Program.source is borrowed unless ast_program_new() chooses
    to duplicate it internally. Keep that behavior explicit in
    ast.c and consistent everywhere.
============================================================

## Comment 2

------------------------------------------------------------
Forward declarations
------------------------------------------------------------

## Comment 3

============================================================
Expr — expression nodes
============================================================

## Comment 4

literals

## Comment 5

variables / references

## Comment 6

operations

## Comment 7

assignment

## Comment 8

call / access

## Comment 9

introspection

## Comment 10

cast

## Comment 11

pipelines

## Comment 12

spread / variadic

## Comment 13

function literal / spawn expression

## Comment 14

------------------------------------------------------------
Operators
------------------------------------------------------------

## Comment 15

shell pipe semantics

## Comment 16

expr | "shell cmd"

## Comment 17

"shell cmd" | expr

## Comment 18

------------------------------------------------------------
Parameter
------------------------------------------------------------

## Comment 19

retained

## Comment 20

XF_TYPE_*

## Comment 21

owned, NULL if absent

## Comment 22

------------------------------------------------------------
Loop binding
------------------------------------------------------------

## Comment 23

retained

## Comment 24

owned

## Comment 25

------------------------------------------------------------
Expr node
------------------------------------------------------------

## Comment 26

optional annotation / inferred hint

## Comment 27

EXPR_NUM

## Comment 28

EXPR_STR

## Comment 29

retained

## Comment 30

EXPR_REGEX

## Comment 31

retained

## Comment 32

EXPR_VALUE

## Comment 33

EXPR_ARR_LIT / EXPR_SET_LIT / EXPR_TUPLE_LIT

## Comment 34

owned

## Comment 35

EXPR_MAP_LIT

## Comment 36

owned

## Comment 37

owned

## Comment 38

EXPR_IDENT

## Comment 39

retained

## Comment 40

EXPR_FIELD

## Comment 41

EXPR_IVAR / EXPR_SVAR

## Comment 42

EXPR_UNARY

## Comment 43

owned

## Comment 44

EXPR_BINARY

## Comment 45

owned

## Comment 46

owned

## Comment 47

EXPR_TERNARY

## Comment 48

owned

## Comment 49

owned

## Comment 50

owned

## Comment 51

EXPR_COALESCE

## Comment 52

owned

## Comment 53

owned

## Comment 54

EXPR_ASSIGN

## Comment 55

owned

## Comment 56

owned

## Comment 57

EXPR_WALRUS

## Comment 58

retained

## Comment 59

XF_TYPE_VOID = inferred

## Comment 60

owned

## Comment 61

EXPR_CALL

## Comment 62

owned

## Comment 63

owned

## Comment 64

EXPR_SUBSCRIPT

## Comment 65

owned

## Comment 66

owned

## Comment 67

EXPR_MEMBER

## Comment 68

owned

## Comment 69

retained

## Comment 70

EXPR_STATE / EXPR_TYPE / EXPR_LEN

## Comment 71

owned

## Comment 72

EXPR_CAST

## Comment 73

XF_TYPE_*

## Comment 74

owned

## Comment 75

EXPR_PIPE_FN

## Comment 76

owned

## Comment 77

owned

## Comment 78

EXPR_SPREAD

## Comment 79

owned

## Comment 80

EXPR_FN

## Comment 81

owned

## Comment 82

owned, usually STMT_BLOCK

## Comment 83

EXPR_SPAWN

## Comment 84

owned; expected to be EXPR_CALL

## Comment 85

EXPR_STATE_LIT

## Comment 86

XF_STATE_*

## Comment 87

============================================================
Stmt — statement nodes
============================================================

## Comment 88

------------------------------------------------------------
Branch
------------------------------------------------------------

## Comment 89

owned, NULL for else-like branch

## Comment 90

owned

## Comment 91

------------------------------------------------------------
Stmt node
------------------------------------------------------------

## Comment 92

STMT_BLOCK

## Comment 93

owned

## Comment 94

STMT_EXPR

## Comment 95

owned

## Comment 96

STMT_VAR_DECL

## Comment 97

XF_TYPE_*

## Comment 98

retained

## Comment 99

owned, NULL = unresolved

## Comment 100

STMT_FN_DECL

## Comment 101

retained

## Comment 102

owned

## Comment 103

owned

## Comment 104

STMT_IF

## Comment 105

owned

## Comment 106

owned, NULL if absent

## Comment 107

STMT_WHILE

## Comment 108

owned

## Comment 109

owned

## Comment 110

STMT_FOR

## Comment 111

owned, optional

## Comment 112

owned, required

## Comment 113

owned

## Comment 114

owned

## Comment 115

STMT_WHILE_SHORT

## Comment 116

owned

## Comment 117

owned

## Comment 118

STMT_FOR_SHORT

## Comment 119

owned

## Comment 120

owned, optional

## Comment 121

owned, required

## Comment 122

owned

## Comment 123

STMT_RETURN

## Comment 124

owned, NULL = void return

## Comment 125

STMT_PRINT / STMT_PRINTF

## Comment 126

owned

## Comment 127

owned, NULL = stdout

## Comment 128

0 none, 1 >file, 2 >>append, 3 |pipe

## Comment 129

STMT_OUTFMT

## Comment 130

XF_OUTFMT_*

## Comment 131

STMT_IMPORT

## Comment 132

retained

## Comment 133

STMT_RIP

## Comment 134

retained

## Comment 135

STMT_DELETE

## Comment 136

owned

## Comment 137

STMT_SPAWN

## Comment 138

owned; expected EXPR_CALL

## Comment 139

STMT_JOIN

## Comment 140

owned

## Comment 141

STMT_SUBST

## Comment 142

retained

## Comment 143

retained

## Comment 144

owned, NULL = $0

## Comment 145

STMT_TRANS

## Comment 146

retained

## Comment 147

retained

## Comment 148

owned, NULL = $0

## Comment 149

============================================================
Top-level items
============================================================

## Comment 150

TOP_BEGIN / TOP_END

## Comment 151

owned

## Comment 152

TOP_RULE

## Comment 153

owned, NULL = match all

## Comment 154

owned

## Comment 155

TOP_FN

## Comment 156

retained

## Comment 157

owned

## Comment 158

owned

## Comment 159

TOP_STMT

## Comment 160

owned

## Comment 161

============================================================
Program
============================================================

## Comment 162

owned

## Comment 163

borrowed

## Comment 164

============================================================
Output format modes
============================================================

## Comment 165

============================================================
Constructors
============================================================

## Comment 166

expression constructors

## Comment 167

loop binding

## Comment 168

statement constructors

## Comment 169

top-level constructors

## Comment 170

program

## Comment 171

recursive free

## Comment 172

debug print

## Comment 173

XF_AST_H

## Comment 174

ifndef XF_AST_H
define XF_AST_H

## Comment 175

include <stddef.h>
include <stdint.h>

## Comment 176

include "lexer.h"
include "value.h"

## Comment 177

define XF_OUTFMT_TEXT  0
define XF_OUTFMT_CSV   1
define XF_OUTFMT_TSV   2
define XF_OUTFMT_JSON  3

## Comment 178

endif /* XF_AST_H */
