#ifndef MOMENTS_H
#define MOMENTS_H

typedef float momFloat;

//----------------------------------------------------------------------------
// Moment tensor components for reduced multipoles.
// IMPORTANT: The order of the fields MUST match with VMOMR
//----------------------------------------------------------------------------
typedef struct
{
    momFloat m;
    momFloat xx,yy,xy,xz,yz;
    momFloat xxx,xyy,xxy,yyy,xxz,yyz,xyz;
    momFloat xxxx,xyyy,xxxy,yyyy,xxxz,yyyz,xxyy,xxyz,xyyz;
} MOMR;

//----------------------------------------------------------------------------
// Moment tensor components for complete multipoles.
//----------------------------------------------------------------------------
typedef struct 
{
    momFloat m;
    momFloat xx,yy,xy,xz,yz;
    momFloat xxx,xyy,xxy,yyy,xxz,yyz,xyz;
    momFloat xxxx,xyyy,xxxy,yyyy,xxxz,yyyz,xxyy,xxyz,xyyz;
    momFloat zz;
    momFloat xzz,yzz,zzz;
    momFloat xxzz,xyzz,xzzz,yyzz,yzzz,zzzz;
} MOMC;


//----------------------------------------------------------------------------
// Moment tensor components for reduced local expansion.
// Note that we have the 5th-order terms here now!
//----------------------------------------------------------------------------
typedef struct 
{
    momFloat m;
    momFloat x,y,z;
    momFloat xx,xy,yy,xz,yz;
    momFloat xxx,xxy,xyy,yyy,xxz,xyz,yyz;
    momFloat xxxx,xxxy,xxyy,xyyy,yyyy,xxxz,xxyz,xyyz,yyyz;
    momFloat xxxxx,xxxxy,xxxyy,xxyyy,xyyyy,yyyyy,xxxxz,xxxyz,xxyyz,xyyyz,yyyyz;
} LOCR;

//----------------------------------------------------------------------------
// Moment tensor components for complete local expansion.
// Note that we have the 5th-order terms here now!
//----------------------------------------------------------------------------
typedef struct
{
    momFloat m;
    momFloat x,y,z;
    momFloat xx,yy,xy,xz,yz;
    momFloat xxx,xyy,xxy,yyy,xxz,yyz,xyz;
    momFloat xxxx,xyyy,xxxy,yyyy,xxxz,yyyz,xxyy,xxyz,xyyz;
    momFloat xxxxx,xxxxy,xxxyy,xxyyy,xyyyy,yyyyy,xxxxz,xxxyz,xxyyz,xyyyz,yyyyz;
    momFloat zz;
    momFloat xzz,yzz,zzz;
    momFloat xxzz,xyzz,xzzz,yyzz,yzzz,zzzz;
    momFloat xxxzz,xxyzz,xyyzz,yyyzz,xxzzz,xyzzz,yyzzz,xzzzz,yzzzz,zzzzz;
} LOCC;


void momAddLocr(LOCR *lr,LOCR *la);
void momAddMomc(MOMC *,MOMC *);
void momAddMomr(MOMR *,MOMR *);
void momMulAddMomc(MOMC *,momFloat,MOMC *);
void momMulAddMomr(MOMR *,momFloat,MOMR *);
void momSubMomc(MOMC *,MOMC *);
void momSubMomr(MOMR *,MOMR *);
void momMakeMomc(MOMC *,momFloat,momFloat,momFloat,momFloat);
momFloat momMakeMomr(MOMR *,momFloat,momFloat,momFloat,momFloat);
void momOldMakeMomr(MOMR *,momFloat,momFloat,momFloat,momFloat);
void momShiftMomc(MOMC *,momFloat,momFloat,momFloat);
void momShiftMomr(MOMR *,momFloat,momFloat,momFloat);
double momShiftLocr(LOCR *,momFloat,momFloat,momFloat);
void momReduceMomc(MOMC *,MOMR *);
/*
** All the variants of EvalMomr...
*/
void momEvalMomr(MOMR *m,momFloat dir,momFloat x,momFloat y,momFloat z,
		 momFloat *fPot,momFloat *ax,momFloat *ay,momFloat *az,
		 momFloat *magai);
void momGenEvalMomr(MOMR *m,momFloat g0,momFloat g1,momFloat g2,momFloat g3,momFloat g4,momFloat g5,
		    momFloat x,momFloat y,momFloat z,
		    momFloat *fPot,momFloat *ax,momFloat *ay,momFloat *az,momFloat *magai);
void momClearLocc(LOCC *);
void momClearLocr(LOCR *);
void momClearMomr(MOMR *);
void momMomr2Momc(MOMR *,MOMC *);
/*
** All the variants of LocrAddMomr...
*/
void momSymLocrAddMomr(LOCR *l1,LOCR *l2,MOMR *q1,MOMR *q2,momFloat dir,momFloat x,momFloat y,momFloat z);
double momLocrAddMomr(LOCR *,MOMR *,momFloat,momFloat,momFloat,momFloat);
void momGenLocrAddMomr(LOCR *l,MOMR *q,momFloat dir,
		       momFloat g0,momFloat t1,momFloat t2,momFloat t3r,momFloat t4r,
		       momFloat x,momFloat y,momFloat z);
void momEwaldLocrAddMomr(LOCR *l,MOMR *m,momFloat r2,int bInHole,momFloat x,momFloat y,momFloat z);
void momNooptLocrAddMomr(LOCR *l,MOMR *m,momFloat dir,momFloat x,momFloat y,momFloat z);
void momLoccAddMomrAccurate(LOCC *l,MOMC *m,momFloat g0,momFloat x,momFloat y,momFloat z);
void momLocrAddMomrAccurate(LOCR *l,MOMR *m,momFloat g0,momFloat x,momFloat y,momFloat z);
/*
** These are the prefered versions that should be used in pkdgrav2.
*/
double momLocrAddMomr5(LOCR *,MOMR *,momFloat,momFloat,momFloat,momFloat,double *,double *,double *);
double momLocrAddMomr5Noopt(LOCR *,MOMR *,momFloat,momFloat,momFloat,momFloat);
void momEvalLocr(LOCR *,momFloat,momFloat,momFloat,
		 momFloat *,momFloat *,momFloat *,momFloat *);
void momPrintMomc(MOMC *);
void momPrintMomr(MOMR *);
void momPrintLocc(LOCC *);
void momPrintLocr(LOCR *);

#endif
