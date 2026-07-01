HEAD = \xs.xs (\h t.h) NONE
TAIL = \xs.xs (\h t.t) NIL
SECOND = \xs.HEAD (TAIL xs)
THIRD = \xs.HEAD (TAIL (TAIL xs))

TRIPLE = \a b c.triple a b c

TRIPLE (HEAD ARGS) (SECOND ARGS) (THIRD ARGS)
