; Run: ./lambdascript -q tests/args_head.ls alpha beta
; Expected: alpha
HEAD = \xs.xs (\h t.h) NONE
HEAD ARGS
