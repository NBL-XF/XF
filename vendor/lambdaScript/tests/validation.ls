; npdf_complete_impl.ls
; Complete LambdaScript implementation of normalized propagation and target-state update.
;
; This is intentionally algebra-parametric. LambdaScript currently has no native
; numeric tower, no real-valued exp, no numeric integral, and no graph iterator.
; So the model is complete at the LambdaScript layer and delegates math/runtime
; behavior to an algebra record. A symbolic algebra is provided at the bottom.
;
; Run:
;   ./lambdascript -q npdf_complete_impl.ls
;
; Expected: a symbolic expression rooted in Phi.
;
; To make it numeric later, replace SYMBOLIC_ALG with a native/runtime algebra
; whose add, sub, mul, div, exp, integral, and sum_neighbors names are bound by
; the host implementation.

ALG = \add sub mul div exp neg integral sum phi f.f add sub mul div exp neg integral sum phi
AADD = \alg.alg (\add sub mul div exp neg integral sum phi.add)
ASUB = \alg.alg (\add sub mul div exp neg integral sum phi.sub)
AMUL = \alg.alg (\add sub mul div exp neg integral sum phi.mul)
ADIV = \alg.alg (\add sub mul div exp neg integral sum phi.div)
AEXP = \alg.alg (\add sub mul div exp neg integral sum phi.exp)
ANEG = \alg.alg (\add sub mul div exp neg integral sum phi.neg)
AINTEGRAL = \alg.alg (\add sub mul div exp neg integral sum phi.integral)
ASUM_NEIGHBORS = \alg.alg (\add sub mul div exp neg integral sum phi.sum)
APHI = \alg.alg (\add sub mul div exp neg integral sum phi.phi)

ADD = \alg x y.(AADD alg) x y
SUB = \alg x y.(ASUB alg) x y
MUL = \alg x y.(AMUL alg) x y
DIV = \alg x y.(ADIV alg) x y
EXP = \alg x.(AEXP alg) x
NEG = \alg x.(ANEG alg) x
INTEGRAL = \alg lo hi body.(AINTEGRAL alg) lo hi body
SUM_NEIGHBORS = \alg beta body.(ASUM_NEIGHBORS alg) beta body
PHI = \alg beta S A I K V.(APHI alg) beta S A I K V

ONE_MINUS = \alg x.SUB alg ONE x

G = \alg alpha beta.MUL alg (MUL alg (MUL alg (V_EDGE alpha beta) (C_EDGE alpha beta)) (A_EDGE alpha beta)) (U_EDGE alpha beta)
DECAY = \alg alpha.EXP alg (NEG alg (MUL alg (LAMBDA_RATE alpha) (ONE_MINUS alg (TAU_STAR alpha))))
FAST_MEMORY = \alg alpha beta.MUL alg (MUL alg (H_MEM alpha) (RHO alpha beta)) (DECAY alg alpha)
SLOW_DIFFUSION = \alg alpha beta.MUL alg (MUL alg (L_MEM alpha) (KAPPA alpha beta)) (ONE_MINUS alg (DECAY alg alpha))
DIFFUSED_MEMORY = \alg alpha beta.ADD alg (FAST_MEMORY alg alpha beta) (SLOW_DIFFUSION alg alpha beta)
PROPAGATION_KERNEL = \alg alpha beta.MUL alg (G alg alpha beta) (DIFFUSED_MEMORY alg alpha beta)
ACTIVATION_DERIVATIVE = \alpha r t.D_SIGMA alpha r t
NUMERATOR_TERM = \alg alpha beta r t.MUL alg (PROPAGATION_KERNEL alg alpha beta) (ACTIVATION_DERIVATIVE alpha r t)
NUMERATOR_INTEGRAND = \alg beta r t.SUM_NEIGHBORS alg beta (\alpha.NUMERATOR_TERM alg alpha beta r t)
NUMERATOR = \alg beta r T.INTEGRAL alg ZERO T (\t.NUMERATOR_INTEGRAND alg beta r t)
NORMALIZATION_COMPONENT = \alg alpha beta.ADD alg (MUL alg (H_MEM alpha) (RHO alpha beta)) (MUL alg (L_MEM alpha) (KAPPA alpha beta))
NORMALIZATION_TERM = \alg alpha beta.MUL alg (G alg alpha beta) (NORMALIZATION_COMPONENT alg alpha beta)
DENOMINATOR_INTEGRAND = \alg beta t.SUM_NEIGHBORS alg beta (\alpha.NORMALIZATION_TERM alg alpha beta)
DENOMINATOR = \alg beta T epsilon.ADD alg epsilon (INTEGRAL alg ZERO T (\t.DENOMINATOR_INTEGRAND alg beta t))
I_HAT_BETA_NEXT = \alg beta r T epsilon.DIV alg (NUMERATOR alg beta r T) (DENOMINATOR alg beta T epsilon)
TARGET_STATE_NEXT = \alg beta r T epsilon.PHI alg beta (S_STATE beta r) (A_STATE beta r) (I_HAT_BETA_NEXT alg beta r T epsilon) (K_CONTEXT beta) (V_CONTEXT beta)

SYMBOLIC_ALG = ALG add sub mul div exp neg integral sum_neighbors Phi

TARGET_STATE_NEXT SYMBOLIC_ALG BETA R T EPSILON