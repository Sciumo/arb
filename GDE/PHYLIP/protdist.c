
#include "phylip.h"
#include "seq.h"

/* version 3.6. (c) Copyright 1993-2001 by the University of Washington.
   Written by Joseph Felsenstein, Akiko Fuseki, Sean Lamont, and Andrew Keeffe.
   Permission is granted to copy and use this program provided no fee is
   charged for it and provided that this copyright notice is not removed. */

#define nmlngth         10    /* number of characters in species name     */

typedef long *steparray;
typedef enum {
  universal, ciliate, mito, vertmito, flymito, yeastmito
} codetype;
typedef enum {
  chemical, hall, george
} cattype;

typedef double matrix[20][20];

#ifndef OLDC
/* function prototypes */
void   protdist_uppercase(Char *);
void   protdist_inputnumbers(void);
void   getoptions(void);
void   transition(void);
void   doinit(void);
void   printcategories(void);
void   inputoptions(void);
void   protdist_inputdata(void);
void   doinput(void);
void   code(void);
void   protdist_cats(void);
void   maketrans(void);
void   givens(matrix, long, long, long, double, double, boolean);
void   coeffs(double, double, double *, double *, double);
void   tridiag(matrix, long, double);
void   shiftqr(matrix, long, double);
void   qreigen(matrix, long);
void   pameigen(void);
void   jtteigen(void);
void   predict(long, long, long);
void   makedists(void);
/* function prototypes */
#endif

long chars, datasets, ith, ctgry, categs;
/* spp = number of species
   chars = number of positions in actual sequences */
double freqa, freqc, freqg, freqt, cvi, invarfrac, ttratio, xi, xv,
  ease, fracchange;
boolean weights, justwts, progress, mulsets, gama, invar, basesequal,
  usepam, usejtt, kimura, similarity, firstset, printdata, interleaved;
codetype whichcode;
cattype whichcat;
steptr oldweight;
double rate[maxcategs];
aas **gnode;
aas trans[4][4][4];
double pie[20];
long cat[(long)ser - (long)ala + 1], numaa[(long)ser - (long)ala + 1];
double eig[20];
matrix prob, eigvecs;
double **d;
char infilename[100], outfilename[100], catfilename[100], weightfilename[100];

/* Local variables for makedists, propagated globally for c version: */
  double tt, p, dp, d2p, q, elambdat;


static double pameigs[] = {0.0, -0.002350753691875762, -0.002701991863800379,
         -0.002931612442853115, -0.004262492032364507, -0.005395980482561625, 
         -0.007141172690079523, -0.007392844756151318, -0.007781761342200766, 
         -0.00810032066366362, -0.00875299712761124, -0.01048227332164386, 
         -0.01109594097332267, -0.01298616073142234, -0.01342036228188581, 
         -0.01552599145527578, -0.01658762802054814, -0.0174893445623765, 
         -0.01933280832903272, -0.02206353522613025};

static double pamprobs[20][20] =
 {{0.087683339901135, 0.04051291829598762, 0.04087846315185977, 
   0.04771603459744777, 0.03247095396561266, 0.03784612688594957, 
   0.0504933695604875, 0.0898249006830755, 0.03285885059543713, 
   0.0357514442352119, 0.0852464099207521, 0.07910313444070642, 
   0.01488243946396588, 0.04100101908956829, 0.05158026947089499, 
   0.06975497205982451, 0.05832757042475474, 0.00931264523877807, 
   0.03171540880870517, 0.06303972920984541}, 
  {0.01943453646811026, -0.004492574160484092, 0.007694891061220776, 
   0.01278399096887701, 0.0106157418450234, 0.007542140341575122, 
   0.01326994069032819, 0.02615565199894889, 0.003123125764490066, 
   0.002204507682495444, -0.004782898215768979, 0.01204241965177619, 
   0.0007847400096924341, -0.03043626073172116, 0.01221202591902536, 
   0.01100527004684405, 0.01116495631339549, -0.0925364931988571, 
   -0.02622065387931562, 0.00843494142432107}, 
  {0.01855357100209072, 0.01493642835763868, 0.0127983090766285, 
   0.0200533250704364, -0.1681898360107787, 0.01551657969909255, 
   0.02128060163107209, 0.03100633591848964, 0.00845480845269879, 
   0.000927149370785571, 0.00937207565817036, 0.03490557769673472, 
   0.00300443019551563, -0.02590837220264415, 0.01329376859943192, 
   0.006854110889741407, 0.01102593860528263, 0.003360844186685888, 
   -0.03459712356647764, 0.003351477369404443}, 
  {0.02690642688200102, 0.02131745801890152, 0.0143626616005213, 
   0.02405101425725929, 0.05041008641436849, 0.01430925051050233, 
   0.02362114036816964, 0.04688381789373886, 0.005250115453626377, 
   -0.02040112168595516, -0.0942720776915669, 0.03773004996758644, 
   -0.00822831940782616, -0.1164872809439224, 0.02286281877257392, 
   0.02849551240669926, 0.01468856796295663, 0.02377110964207936, 
   -0.094380545436577, -0.02089068498518036}, 
  {0.00930172577225213, 0.01493463068441099, 0.020186920775608, 
   0.02892154953912524, -0.01224593358361567, 0.01404228329986624, 
   0.02671186617119041, 0.04537535161795231, 0.02229995804098249, 
   -0.04635704133961575, -0.1966910360247138, 0.02796648065439046, 
   -0.02263484732621436, 0.0440490503242072, 0.01148782948302166, 
   0.01989170531824069, 0.001306805142981245, -0.005676690969116321, 
   0.07680476281625202, -0.07967537039721849}, 
  {0.06602274245435476, -0.0966661981471856, -0.005241648783844579, 
   0.00859135188171146, -0.007762129660943368, -0.02888965572526196, 
   0.003592291525888222, 0.1668410669287673, -0.04082039290551406, 
   0.005233775047553415, -0.01758244726137135, -0.1493955762326898, 
   -0.00855819137835548, 0.004211419253492328, 0.01929306335052688, 
   0.03008056746359405, 0.0190444422412472, 0.005577189741419315, 
   0.0000874156155112068, 0.02634091459108298}, 
  {0.01933897472880726, 0.05874583569377844, -0.02293534606228405, 
   -0.07206314017962175, -0.004580681581546643, -0.0628814337610561, 
   -0.0850783812795136, 0.07988417636610614, -0.0852798990133397, 
   0.01649047166155952, -0.05416647263757423, 0.1089834536254064, 
   0.005093403979413865, 0.02520300254161142, 0.0005951431406455604, 
   0.02441251821224675, 0.02796099482240553, -0.002574933994926502, 
   -0.007172237553012804, 0.03002455129086954}, 
  {0.04041118479094272, -0.002476225672095412, -0.01494505811263243, 
   -0.03759443758599911, -0.00892246902492875, -0.003634714029239211, 
   -0.03085671837973749, -0.126176309029931, 0.005814031139083794, 
   0.01313561962646063, -0.04760487162503322, -0.0490563712725484, 
   -0.005082243450421558, -0.01213634309383557, 0.1806666927079249, 
   0.02111663336185495, 0.02963486860587087, -0.0000175020101657785, 
   0.01197155383597686, 0.0357526792184636}, 
  {-0.01184769557720525, 0.01582776076338872, -0.006570708266564639, 
   -0.01471915653734024, 0.00894343616503608, 0.00562664968033149, 
   -0.01465878888356943, 0.05365282692645818, 0.00893509735776116, 
   -0.05879312944436473, 0.0806048683392995, -0.007722897986905326, 
   -0.001819943882718859, 0.0942535573077267, 0.07483883782251654, 
   0.004354639673913651, -0.02828804845740341, -0.001318222184691827, 
   -0.07613149604246563, -0.1251675867732172}, 
  {0.00834167031558193, -0.01509357596974962, 0.007098172811092488, 
   0.03127677418040319, 0.001992448468465455, 0.00915441566808454, 
   0.03430175973499201, -0.0730648147535803, -0.001402707145575659, 
   0.04780949194330815, -0.1115035603461273, -0.01292297197609604, 
   -0.005056270550868528, 0.1112053349612027, -0.03801929822379964, 
   -0.001191241001736563, 0.01872874622910247, 0.0005314214903865993, 
   -0.0882576318311789, 0.07607183599610171}, 
  {-0.01539460099727769, 0.04988596184297883, -0.01187240760647617, 
   -0.06987843637091853, -0.002490472846497859, 0.01009857892494956, 
   -0.07473588067847209, 0.0906009925879084, 0.1243612446505172, 
   0.02152806401345371, -0.03504879644860233, -0.06680752427613573, 
   -0.005574485153629651, 0.001518282948127752, -0.01999168507510701, 
   -0.01478606199529457, -0.02203749419458996, -0.00132680708294333, 
   -0.01137505997867614, 0.05332658773667142}, 
  {-0.06104378736432388, 0.0869446603393548, -0.03298331234537257, 
   0.03128515657456024, 0.003906358569208259, 0.03578694104193928, 
   0.06241936133189683, 0.06182827284921748, -0.05566564263245907, 
   0.02640868588189002, -0.01349751243059039, -0.05507866642582638, 
   -0.006671347738489326, -0.001470096466016046, 0.05185743641479938, 
   -0.07494697511168257, -0.1175185439057584, -0.001188074094105709, 
   0.00937934805737347, 0.05024773745437657}, 
  {-0.07252555582124737, -0.116554459356382, 0.003605361887406413, 
   -0.00836518656029184, 0.004615715410745561, 0.005105376617651312, 
   -0.00944938657024391, 0.05602449420950007, 0.02722719610561933, 
   0.01959357494748446, -0.0258655103753962, 0.1440733975689835, 
   0.01446782819722976, 0.003718896062070054, 0.05825843045655135, 
   -0.06230154142733073, -0.07833704962300169, 0.003160836143568724, 
   -0.001169873777936648, 0.03471745590503304}, 
  {-0.03204352258752698, 0.01019272923862322, 0.04509668708733181, 
   0.05756522429120813, -0.0004601149081726732, -0.0984718150777423, 
   -0.01107826100664925, -0.005680277810520585, 0.01962359392320817, 
   0.01550006899131986, 0.05143956925922197, 0.02462476682588468, 
   -0.0888843861002653, -0.00171553583659411, 0.01606331750661664, 
   0.001176847743518958, -0.02070972978912828, -0.000341523293579971, 
   -0.002654732745607882, 0.02075709428885848}, 
  {0.03595199666430258, -0.02800219615234468, -0.04341570015493925, 
   -0.0748275906176658, 0.0001051403676377422, 0.1137431321746627, 
   0.005852087565974318, 0.003443037513847801, -0.02481931657706633, 
   -0.003651181839831423, 0.03195794176786321, 0.04135411406392523, 
   -0.07562030263210619, 0.001769332364699, -0.01984381173403915, 
   -0.005029750745010152, 0.02649253902476472, 0.000518085571702734, 
   0.001062936684474851, 0.01295950668914449}, 
  {-0.16164552322896, -0.0006050035060464324, 0.0258380054414968, 
   0.003188424740960557, -0.0002058911341821877, 0.03157555987384681, 
   -0.01678913462596107, 0.03096216145389774, -0.0133791110666919, 
   0.1125249625204277, -0.00769017706442472, -0.02653938062180483, 
   -0.002555329863523985, -0.00861833362947954, 0.01775148884754278, 
   0.02529310679774722, 0.0826243417011238, -0.0001036728183032624, 
   0.001963562313294209, -0.0935900561309786}, 
  {0.1652394174588469, -0.002814245280784351, -0.0328982001821263, 
   -0.02000104712964131, 0.0002208121995725443, -0.02733462178511839, 
   0.02648078162927627, -0.01788316626401427, 0.01630747623755998, 
   0.1053849023838147, -0.005447706553811218, 0.01810876922536839, 
   -0.001808914710282444, -0.007687912115607397, -0.01332593672114388, 
   -0.02110750894891371, -0.07456116592983384, 0.000219072589592394, 
   0.001270886972191055, -0.1083616930749109}, 
  {0.02453279389716254, -0.005820072356487439, 0.100260287284095, 
   0.01277522280305745, -0.003184943445296999, 0.05814689527984152, 
   -0.0934012278200201, -0.03017986487349484, -0.03136625380994165, 
   0.00988668352785117, -0.00358900410973142, -0.02017443675004764, 
   0.000915384582922184, -0.001460963415183106, -0.01370112443251124, 
   0.1130040979284457, -0.1196161771323699, -0.0005800211204222045, 
   -0.0006153403201024954, 0.00416806428223025}, 
  {-0.0778089244252535, -0.007055161182430869, -0.0349307504860869, 
   -0.0811915584276571, -0.004689825871599125, -0.03726108871471753, 
   0.1072225647141469, -0.00917015113070944, 0.01381628985996913, 
   -0.00123227881492089, 0.001815954515275675, 0.005708744099349901, 
   -0.0001448985044877925, -0.001306578795561384, -0.006992743514185243, 
   0.1744720240732789, -0.05353628497814023, -0.0007613684227234787, 
   -0.0003550282315997644, 0.01340106423804634}, 
  {-0.0159527329868513, -0.007622151568160798, -0.1389875105184963, 
   0.1165051999914764, -0.002217810389087748, 0.01550003226513692, 
   -0.07427664222230566, -0.003371438498619264, 0.01385754771325365, 
   0.004759020167383304, 0.001624078805220564, 0.02011638303109029, 
   -0.001717827082842178, -0.0007424036708598594, -0.003978884451898934, 
   0.0866418927301209, -0.01280817739158123, -0.00023039242454603, 
   0.002309205802479111, 0.0005926106991001195}};

/* this jtt matrix decomposition due to Elisabeth  Tillier */
static double jtteigs[] =
{0.0,        -0.007031123, -0.006484345, -0.006086499, -0.005514432,
-0.00772664, -0.008643413, -0.010620756, -0.009965552, -0.011671808,
-0.012222418,-0.004589201, -0.013103714, -0.014048038, -0.003170582,
-0.00347935, -0.015311677, -0.016021194, -0.017991454, -0.018911888};

static double jttprobs[20][20] =
{{0.076999996, 0.051000003, 0.043000004, 0.051999998, 0.019999996, 0.041,
  0.061999994, 0.073999997, 0.022999999, 0.052000004, 0.090999997, 0.058999988,
  0.024000007, 0.04, 0.050999992, 0.069, 0.059000006, 0.014000008, 0.032000004,
  0.066000005},
 {0.015604455, -0.068062363, 0.020106264, 0.070723273, 0.011702977, 0.009674053,
  0.074000798, -0.169750458, 0.005560808, -0.008208636, -0.012305869,
 -0.063730179, -0.005674643, -0.02116828, 0.104586169, 0.016480839, 0.016765139,
  0.005936994, 0.006046367, -0.0082877},
 {-0.049778281, -0.007118197, 0.003801272, 0.070749616, 0.047506147,
   0.006447017, 0.090522425, -0.053620432, -0.008508175, 0.037170603,
   0.051805545, 0.015413608, 0.019939916, -0.008431976, -0.143511376,
  -0.052486072, -0.032116542, -0.000860626, -0.02535993, 0.03843545},
 {-0.028906423, 0.092952047, -0.009615343, -0.067870117, 0.031970392,
   0.048338335, -0.054396304, -0.135916654, 0.017780083, 0.000129242,
   0.031267424, 0.116333586, 0.007499746, -0.032153596, 0.033517051,
  -0.013719269, -0.00347293, -0.003291821, -0.02158326, -0.008862168},
 {0.037181176, -0.023106564, -0.004482225, -0.029899635, 0.118139633,
 -0.032298569, -0.04683198, 0.05566988, -0.012622847, 0.002023096,
 -0.043921088, -0.04792557, -0.003452711, -0.037744513, 0.020822974,
  0.036580187, 0.02331425, -0.004807711, -0.017504496, 0.01086673},
 {0.044754061, -0.002503471, 0.019452517, -0.015611487, -0.02152807,
 -0.013131425, -0.03465365, -0.047928912, 0.020608851, 0.067843095,
 -0.122130014, 0.002521499, 0.013021646, -0.082891087, -0.061590119,
  0.016270856, 0.051468938, 0.002079063, 0.081019713, 0.082927944},
 {0.058917882, 0.007320741, 0.025278141, 0.000357541, -0.002831285,
 -0.032453034, -0.010177288, -0.069447924, -0.034467324, 0.011422358,
 -0.128478324, 0.04309667, -0.015319944, 0.113302422, -0.035052393,
  0.046885372, 0.06185183, 0.00175743, -0.06224497, 0.020282093},
 {-0.014562092, 0.022522921, -0.007094389, 0.03480089, -0.000326144,
 -0.124039037, 0.020577906, -0.005056454, -0.081841576, -0.004381786,
  0.030826152, 0.091261631, 0.008878828, -0.02829487, 0.042718836,
 -0.011180886, -0.012719227, -0.000753926, 0.048062375, -0.009399129},
 {0.033789571, -0.013512235, 0.088010984, 0.017580292, -0.006608005,
 -0.037836971, -0.061344686, -0.034268357, 0.018190209, -0.068484614,
  0.120024744, -0.00319321, -0.001349477, -0.03000546, -0.073063759,
  0.081912399, 0.0635245, 0.000197, -0.002481798, -0.09108114},
 {-0.113947615, 0.019230545, 0.088819683, 0.064832765, 0.001801467,
 -0.063829682, -0.072001633, 0.018429333, 0.057465965, 0.043901014,
 -0.048050874, -0.001705918, 0.022637173, 0.017404665, 0.043877902,
 -0.017089594, -0.058489485, 0.000127498, -0.029357194, 0.025943972},
 {0.01512923, 0.023603725, 0.006681954, 0.012360216, -0.000181447,
 -0.023011838, -0.008960024, -0.008533239, 0.012569835, 0.03216118,
  0.061986403, -0.001919083, -0.1400832, -0.010669741, -0.003919454,
 -0.003707024, -0.026806029, -0.000611603, -0.001402648, 0.065312824},
 {-0.036405351, 0.020816769, 0.011408213, 0.019787053, 0.038897829,
   0.017641789, 0.020858533, -0.006067252, 0.028617353, -0.064259496,
  -0.081676567, 0.024421823, -0.028751676, 0.07095096, -0.024199434,
  -0.007513119, -0.028108766, -0.01198095, 0.111761119, -0.076198809},
 {0.060831772, 0.144097327, -0.069151377, 0.023754576, -0.003322955,
 -0.071618574, 0.03353154, -0.02795295, 0.039519769, -0.023453968,
 -0.000630308, -0.098024591, 0.017672997, 0.003813378, -0.009266499,
 -0.011192111, 0.016013873, -0.002072968, -0.010022044, -0.012526904},
 {-0.050776604, 0.092833081, 0.044069596, 0.050523021, -0.002628417,
   0.076542572, -0.06388631, -0.00854892, -0.084725311, 0.017401063,
  -0.006262541, -0.094457679, -0.002818678, -0.0044122, -0.002883973,
   0.028729685, -0.004961596, -0.001498627, 0.017994575, -0.000232779},
 {-0.01894566, -0.007760205, -0.015160993, -0.027254587, 0.009800903,
  -0.013443561, -0.032896517, -0.022734138, -0.001983861, 0.00256111,
   0.024823166, -0.021256768, 0.001980052, 0.028136263, -0.012364384,
  -0.013782446, -0.013061091, 0.111173981, 0.021702122, 0.00046654},
 {-0.009444193, -0.042106824, -0.02535015, -0.055125574, 0.006369612,
  -0.02945416, -0.069922064, -0.067221068, -0.003004999, 0.053624311,
   0.128862984, -0.057245803, 0.025550508, 0.087741073, -0.001119043,
  -0.012036202, -0.000913488, -0.034864475, 0.050124813, 0.055534723},
 {0.145782464, -0.024348311, -0.031216873, 0.106174443, 0.00202862,
  0.02653866, -0.113657267, -0.00755018, 0.000307232, -0.051241158,
  0.001310685, 0.035275877, 0.013308898, 0.002957626, -0.002925034,
 -0.065362319, -0.071844582, 0.000475894, -0.000112419, 0.034097762},
 {0.079840455, 0.018769331, 0.078685899, -0.084329807, -0.00277264,
 -0.010099754, 0.059700608, -0.019209715, -0.010442992, -0.042100476,
 -0.006020556, -0.023061786, 0.017246106, -0.001572858, -0.006703785,
  0.056301316, -0.156787357, -0.000303638, 0.001498195, 0.051363455},
 {0.049628261, 0.016475144, 0.094141653, -0.04444633, 0.005206131,
 -0.001827555, 0.02195624, 0.013066683, -0.010415582, -0.022338403,
  0.007837197, -0.023397671, -0.002507095, 0.005177694, 0.017109561,
 -0.202340113, 0.069681441, 0.000120736, 0.002201146, 0.004670849},
 {0.089153689, 0.000233354, 0.010826822, -0.004273519, 0.001440618,
  0.000436077, 0.001182351, -0.002255508, -0.000700465, 0.150589876,
 -0.003911914, -0.00050154, -0.004564983, 0.00012701, -0.001486973,
 -0.018902754, -0.054748555, 0.000217377, -0.000319302, -0.162541651}};


void protdist_uppercase(Char *ch)
{
 (*ch) = (isupper(*ch) ? (*ch) : toupper(*ch));
}  /* protdist_uppercase */


void protdist_inputnumbers()
{
  /* input the numbers of species and of characters */
  long i;

  fscanf(infile, "%ld%ld", &spp, &chars);

  if (printdata)
    fprintf(outfile, "%2ld species, %3ld  positions\n\n", spp, chars);
  gnode = (aas **)Malloc(spp * sizeof(aas *));
  if (firstset) {
    for (i = 0; i < spp; i++)
      gnode[i] = (aas *)Malloc(chars * sizeof(aas ));
  }
  weight = (steparray)Malloc(chars*sizeof(long));
  oldweight = (steparray)Malloc(chars*sizeof(long));
  category = (steparray)Malloc(chars*sizeof(long));
  d      = (double **)Malloc(spp*sizeof(double *));
  nayme  = (naym *)Malloc(spp*sizeof(naym));

  for (i = 0; i < spp; ++i)
    d[i] = (double *)Malloc(spp*sizeof(double));
}  /* protdist_inputnumbers */


void getoptions()
{
  /* interactively set options */
  long loopcount, loopcount2;
  Char ch, ch2;
  Char in[100];
  boolean done;

  if (printdata)
    fprintf(outfile, "\nProtein distance algorithm, version %s\n\n",VERSION);
  putchar('\n');
  weights = false;
  printdata = false;
  progress = true;
  interleaved = true;
  similarity = false;
  ttratio = 2.0;
  whichcode = universal;
  whichcat = george;
  basesequal = true;
  freqa = 0.25;
  freqc = 0.25;
  freqg = 0.25;
  freqt = 0.25;
  usejtt = true;
  usepam = false;
  kimura = false;
  gama = false;
  invar = false;
  invarfrac = 0.0;
  ease = 0.457;
  loopcount = 0;
  do {
    cleerhome();
    printf("\nProtein distance algorithm, version %s\n\n",VERSION);
    printf("Settings for this run:\n");
    printf("  P     Use JTT, PAM, Kimura or categories model?  %s\n",
           usejtt ? "Jones-Taylor-Thornton matrix" :
           usepam ? "Dayhoff PAM matrix" :
           kimura ? "Kimura formula" :
           similarity ? "Similarity table" : "Categories model");
    if (!kimura && !similarity) {
      printf("  G  Gamma distribution of rates among positions?");
      if (gama)
        printf("  Yes\n");
      else {
        if (invar)
          printf("  Gamma+Invariant\n");
        else
          printf("  No\n");
      }
    }
    printf("  C           One category of substitution rates?");
    if (!ctgry || categs == 1)
      printf("  Yes\n");
    else
      printf("  %ld categories\n", categs);
    printf("  W                    Use weights for positions?");
    if (weights)
      printf("  Yes\n");
    else
      printf("  No\n");
    if (!(usejtt || usepam || kimura || similarity)) {
      printf("  U                       Use which genetic code?  %s\n",
             (whichcode == universal) ? "Universal"                  :
             (whichcode == ciliate)   ? "Ciliate"                    :
             (whichcode == mito)      ? "Universal mitochondrial"    :
             (whichcode == vertmito)  ? "Vertebrate mitochondrial"   :
             (whichcode == flymito)   ? "Fly mitochondrial\n"        :
             (whichcode == yeastmito) ? "Yeast mitochondrial"        : "");
      printf("  A          Which categorization of amino acids?  %s\n",
             (whichcat == chemical) ? "Chemical"              :
             (whichcat == george)   ? "George/Hunt/Barker"    : "Hall");
        
      printf("  E              Prob change category (1.0=easy):%8.4f\n",ease);
      printf("  T                Transition/transversion ratio:%7.3f\n",ttratio);
      printf("  F                             Base Frequencies:");
      if (basesequal)
        printf("  Equal\n");
      else
        printf("%7.3f%6.3f%6.3f%6.3f\n", freqa, freqc, freqg, freqt);
    }
    printf("  M                   Analyze multiple data sets?");
    if (mulsets)
      printf("  Yes, %2ld %s\n", datasets,
               (justwts ? "sets of weights" : "data sets"));
    else
      printf("  No\n");
    printf("  I                  Input sequences interleaved?  %s\n",
           (interleaved ? "Yes" : "No, sequential"));
    printf("  0                 Terminal type (IBM PC, ANSI)?  %s\n",
           ibmpc ? "IBM PC" :
           ansi  ? "ANSI"   : "(none)");
    printf("  1            Print out the data at start of run  %s\n",
           (printdata ? "Yes" : "No"));
    printf("  2          Print indications of progress of run  %s\n",
           progress ? "Yes" : "No");
    printf("\nAre these settings correct? (type Y or the letter for one to change)\n");
    in[0] = '\0';
    getstryng(in);
    ch=in[0];
    if (ch == '\n')
      ch = ' ';
    protdist_uppercase(&ch);
    done = (ch == 'Y');
    if (!done) {
      if (((strchr("CPGMWI120",ch) != NULL) && (usejtt || usepam)) ||
          ((strchr("CPMWI120",ch) != NULL) && (kimura || similarity)) ||
          ((strchr("CUAPGETFMWI120",ch) != NULL) && 
            (! (usejtt || usepam || kimura || similarity)))) {
        switch (ch) {

        case 'U':
          printf("Which genetic code?\n");
          printf(" type         for\n\n");
          printf("   U           Universal\n");
          printf("   M           Mitochondrial\n");
          printf("   V           Vertebrate mitochondrial\n");
          printf("   F           Fly mitochondrial\n");
          printf("   Y           Yeast mitochondrial\n\n");
          loopcount2 = 0;
          do {
            printf("type U, M, V, F, or Y\n");
            scanf("%c%*[^\n]", &ch);
            getchar();
            if (ch == '\n')
              ch = ' ';
            protdist_uppercase(&ch);
            countup(&loopcount2, 10);
          } while (ch != 'U' && ch != 'M' && ch != 'V' && ch != 'F' && ch != 'Y');
          switch (ch) {

          case 'U':
            whichcode = universal;
            break;

          case 'M':
            whichcode = mito;
            break;

          case 'V':
            whichcode = vertmito;
            break;

          case 'F':
            whichcode = flymito;
            break;

          case 'Y':
            whichcode = yeastmito;
            break;
          }
          break;

        case 'A':
          printf(
            "Which of these categorizations of amino acids do you want to use:\n\n");
          printf(
            " all have groups: (Glu Gln Asp Asn), (Lys Arg His), (Phe Tyr Trp)\n");
          printf(" plus:\n");
          printf("George/Hunt/Barker:");
          printf(" (Cys), (Met   Val  Leu  Ileu), (Gly  Ala  Ser  Thr    Pro)\n");
          printf("Chemical:          ");
          printf(" (Cys   Met), (Val  Leu  Ileu    Gly  Ala  Ser  Thr), (Pro)\n");
          printf("Hall:              ");
          printf(" (Cys), (Met   Val  Leu  Ileu), (Gly  Ala  Ser  Thr), (Pro)\n\n");
          printf("Which do you want to use (type C, H, or G)\n");
          loopcount2 = 0;
          do {
            scanf("%c%*[^\n]", &ch);
            getchar();
            if (ch == '\n')
              ch = ' ';
            protdist_uppercase(&ch);
            countup(&loopcount2, 10);
          } while (ch != 'C' && ch != 'H' && ch != 'G');
          switch (ch) {

          case 'C':
            whichcat = chemical;
            break;

          case 'H':
            whichcat = hall;
            break;

          case 'G':
            whichcat = george;
            break;
          }
          break;

        case 'C':
          ctgry = !ctgry;
          if (ctgry) {
            initcatn(&categs);
            initcategs(categs, rate);
          }
          break;

        case 'W':
          weights = !weights;
          break;

        case 'P':
          if (usejtt) {
            usejtt = false;
            usepam = true;
          } else {
            if (usepam) {
              usepam = false;
              kimura = true;
            } else {
              if (kimura) {
                kimura = false;
                similarity = true;
              } else {
                if (similarity)
                  similarity = false;
                else
                  usejtt = true;
              }
            }
          }
          break;

        case 'G':
          if (!(gama || invar))
            gama = true;
          else {
            if (gama) {
              gama = false;
              invar = true;
            } else {
              if (invar)
                invar = false;
            }
          }
          break;


        case 'E':
          printf("Ease of changing category of amino acid?\n");
          loopcount2 = 0;
          do {
            printf(" (1.0 if no difficulty of changing,\n");
            printf(" less if less easy. Can't be negative\n");
            scanf("%lf%*[^\n]", &ease);
            getchar();
            countup(&loopcount2, 10);
          } while (ease > 1.0 || ease < 0.0);
          break;

        case 'T':
          loopcount2 = 0;
          do {
            printf("Transition/transversion ratio?\n");
            scanf("%lf%*[^\n]", &ttratio);
            getchar();
            countup(&loopcount2, 10);
          } while (ttratio < 0.0);
          break;

        case 'F':
          loopcount2 = 0;
          do {
            basesequal = false;
            printf("Frequencies of bases A,C,G,T ?\n");
            scanf("%lf%lf%lf%lf%*[^\n]", &freqa, &freqc, &freqg, &freqt);
            getchar();
            if (fabs(freqa + freqc + freqg + freqt - 1.0) >= 1.0e-3)
              printf("FREQUENCIES MUST SUM TO 1\n");
            countup(&loopcount2, 10);
          } while (fabs(freqa + freqc + freqg + freqt - 1.0) >= 1.0e-3);
          break;

        case 'M':
          mulsets = !mulsets;
          if (mulsets) {
            printf("Multiple data sets or multiple weights?");
            loopcount2 = 0;
            do {
              printf(" (type D or W)\n");
              scanf("%c%*[^\n]", &ch2);
              getchar();
              if (ch2 == '\n')
                  ch2 = ' ';
              uppercase(&ch2);
              countup(&loopcount2, 10);
            } while ((ch2 != 'W') && (ch2 != 'D'));
            justwts = (ch2 == 'W');
            if (justwts)
              justweights(&datasets);
            else
              initdatasets(&datasets);
          }
          break;

        case 'I':
          interleaved = !interleaved;
          break;

        case '0':
          if (ibmpc) {
            ibmpc = false;
            ansi = true;
            } else if (ansi)
              ansi = false;
            else
              ibmpc = true;
          break;

        case '1':
          printdata = !printdata;
          break;

        case '2':
          progress = !progress;
          break;
        }
      } else {
        if (strchr("CUAPGETFMWI120",ch) == NULL)
          printf("Not a possible option!\n");
        else
          printf("That option not allowed with these settings\n");
        printf("\nPress Enter or Return key to continue\n");
        getchar();
      }
    }
    countup(&loopcount, 100);
  } while (!done);
  if (gama || invar) {
    loopcount = 0;
    do {
      printf(
"\nCoefficient of variation of substitution rate among positions (must be positive)\n");
      printf(
  " In gamma distribution parameters, this is 1/(square root of alpha)\n");
      scanf("%lf%*[^\n]", &cvi);
      getchar();
      countup(&loopcount, 10);
    } while (cvi <= 0.0);
    cvi = 1.0 / (cvi * cvi);
  }
  if (invar) {
    loopcount = 0;
    do {
      printf("Fraction of invariant positions?\n");
      scanf("%lf%*[^\n]", &invarfrac);
      getchar();
      countup (&loopcount, 10);
    } while ((invarfrac <= 0.0) || (invarfrac >= 1.0));
  }
}  /* getoptions */


void transition()
{
  /* calculations related to transition-transversion ratio */
  double aa, bb, freqr, freqy, freqgr, freqty;

  freqr = freqa + freqg;
  freqy = freqc + freqt;
  freqgr = freqg / freqr;
  freqty = freqt / freqy;
  aa = ttratio * freqr * freqy - freqa * freqg - freqc * freqt;
  bb = freqa * freqgr + freqc * freqty;
  xi = aa / (aa + bb);
  xv = 1.0 - xi;
  if (xi <= 0.0 && xi >= -epsilon)
    xi = 0.0;
  if (xi < 0.0){
    printf("THIS TRANSITION-TRANSVERSION RATIO IS IMPOSSIBLE WITH");
    printf(" THESE BASE FREQUENCIES\n");
    exxit(-1);}
}  /* transition */


void doinit()
{
  /* initializes variables */
  protdist_inputnumbers();
  getoptions();
  transition();
}  /* doinit*/


void printcategories()
{ /* print out list of categories of positions */
  long i, j;

  fprintf(outfile, "Rate categories\n\n");
  for (i = 1; i <= nmlngth + 3; i++)
    putc(' ', outfile);
  for (i = 1; i <= chars; i++) {
    fprintf(outfile, "%ld", category[i - 1]);
    if (i % 60 == 0) {
      putc('\n', outfile);
      for (j = 1; j <= nmlngth + 3; j++)
        putc(' ', outfile);
    } else if (i % 10 == 0)
      putc(' ', outfile);
  }
  fprintf(outfile, "\n\n");
}  /* printcategories */


void inputoptions()
{ /* input the information on the options */
  long i;

  if (!firstset)
    samenumsp(&chars, ith);
  if (firstset) {
    for (i = 0; i < chars; i++) {
      category[i] = 1;
      oldweight[i] = 1;
      weight[i] = 1;
    }
  }
  if (!justwts && weights)
    inputweights(chars, oldweight, &weights);
  if (printdata)
    putc('\n', outfile);
  if (usejtt && printdata)
    fprintf(outfile, "  Jones-Taylor-Thornton model distance\n");
  if (usepam && printdata)
    fprintf(outfile, "  Dayhoff PAM model distance\n");
  if (kimura && printdata)
    fprintf(outfile, "  Kimura protein distance\n");
  if (!(usejtt || usepam || kimura || similarity) && printdata)
    fprintf(outfile, "  Categories model distance\n");
  if (similarity)
    fprintf(outfile, "  \n  Table of similarity between sequences\n");
  if ((ctgry && categs > 1) && (firstset || !justwts)) {
    inputcategs(0, chars, category, categs, "ProtDist");
    if (printdata)
      printcategs(outfile, chars, category, "Position categories");
  } else if (printdata && (categs > 1)) {
    fprintf(outfile, "\nPosition category   Rate of change\n\n");
    for (i = 1; i <= categs; i++)
      fprintf(outfile, "%15ld%13.3f\n", i, rate[i - 1]);
    putc('\n', outfile);
    printcategories();
  }
  if (weights && printdata)
    printweights(outfile, 0, chars, oldweight, "Positions");
}  /* inputoptions */


void protdist_inputdata()
{
  /* input the names and sequences for each species */
  long i, j, k, l, aasread=0, aasnew=0;
  Char charstate;
  boolean allread, done;
  aas aa=0;   /* temporary amino acid for input */

  if (progress)
    putchar('\n');
  j = nmlngth + (chars + (chars - 1) / 10) / 2 - 5;
  if (j < nmlngth - 1)
    j = nmlngth - 1;
  if (j > 37)
    j = 37;
  if (printdata) {
    fprintf(outfile, "\nName");
    for (i = 1; i <= j; i++)
      putc(' ', outfile);
    fprintf(outfile, "Sequences\n");
    fprintf(outfile, "----");
    for (i = 1; i <= j; i++)
      putc(' ', outfile);
    fprintf(outfile, "---------\n\n");
  }
  aasread = 0;
  allread = false;
  while (!(allread)) {
    allread = true;
    if (eoln(infile)) 
      scan_eoln(infile);
    i = 1;
    while (i <= spp) {
      if ((interleaved && aasread == 0) || !interleaved)
        initname(i-1);
      if (interleaved)
        j = aasread;
      else
        j = 0;
      done = false;
      while (((!done) && (!(eoln(infile) | eoff(infile))))) {
        if (interleaved)
          done = true;
        while (((j < chars) & (!(eoln(infile) | eoff(infile))))) {
          charstate = gettc(infile);
          if (charstate == '\n')
            charstate = ' ';
          if (charstate == ' ' || (charstate >= '0' && charstate <= '9'))
            continue;
          protdist_uppercase(&charstate);
          if ((!isalpha(charstate) && charstate != '.' && charstate != '?' &&
               charstate != '-' && charstate != '*') || charstate == 'J' ||
              charstate == 'O' || charstate == 'U' || charstate == '.') {
        printf("ERROR -- bad amino acid: %c at position %ld of species %3ld\n",
                   charstate, j, i);
            if (charstate == '.') {
          printf("         Periods (.) may not be used as gap characters.\n");
          printf("         The correct gap character is (-)\n");
            }
            exxit(-1);
          }
          j++;

          switch (charstate) {

          case 'A':
            aa = ala;
            break;

          case 'B':
            aa = asx;
            break;

          case 'C':
            aa = cys;
            break;

          case 'D':
            aa = asp;
            break;

          case 'E':
            aa = glu;
            break;

          case 'F':
            aa = phe;
            break;

          case 'G':
            aa = gly;
            break;

          case 'H':
            aa = his;
            break;

          case 'I':
            aa = ileu;
            break;

          case 'K':
            aa = lys;
            break;

          case 'L':
            aa = leu;
            break;

          case 'M':
            aa = met;
            break;

          case 'N':
            aa = asn;
            break;

          case 'P':
            aa = pro;
            break;

          case 'Q':
            aa = gln;
            break;

          case 'R':
            aa = arg;
            break;

          case 'S':
            aa = ser;
            break;

          case 'T':
            aa = thr;
            break;

          case 'V':
            aa = val;
            break;

          case 'W':
            aa = trp;
            break;

          case 'X':
            aa = unk;
            break;

          case 'Y':
            aa = tyr;
            break;

          case 'Z':
            aa = glx;
            break;

          case '*':
            aa = stop;
            break;

          case '?':
            aa = quest;
            break;

          case '-':
            aa = del;
            break;
          }
          gnode[i - 1][j - 1] = aa;
        }
        if (interleaved)
          continue;
        if (j < chars) 
          scan_eoln(infile);
        else if (j == chars)
          done = true;
      }
      if (interleaved && i == 1)
        aasnew = j;
      scan_eoln(infile);
      if ((interleaved && j != aasnew) || ((!interleaved) && j != chars)){
        printf("ERROR: SEQUENCES OUT OF ALIGNMENT\n");
        exxit(-1);}
      i++;
    }
    if (interleaved) {
      aasread = aasnew;
      allread = (aasread == chars);
    } else
      allread = (i > spp);
  }
  if ( printdata) {
    for (i = 1; i <= ((chars - 1) / 60 + 1); i++) {
      for (j = 1; j <= spp; j++) {
        for (k = 0; k < nmlngth; k++)
          putc(nayme[j - 1][k], outfile);
        fprintf(outfile, "   ");
        l = i * 60;
        if (l > chars)
          l = chars;
        for (k = (i - 1) * 60 + 1; k <= l; k++) {
          if (j > 1 && gnode[j - 1][k - 1] == gnode[0][k - 1])
            charstate = '.';
          else {
            switch (gnode[j - 1][k - 1]) {

            case ala:
              charstate = 'A';
              break;

            case asx:
              charstate = 'B';
              break;

            case cys:
              charstate = 'C';
              break;

            case asp:
              charstate = 'D';
              break;

            case glu:
              charstate = 'E';
              break;

            case phe:
              charstate = 'F';
              break;

            case gly:
              charstate = 'G';
              break;

            case his:
              charstate = 'H';
              break;

            case ileu:
              charstate = 'I';
              break;

            case lys:
              charstate = 'K';
              break;

            case leu:
              charstate = 'L';
              break;

            case met:
              charstate = 'M';
              break;

            case asn:
              charstate = 'N';
              break;

            case pro:
              charstate = 'P';
              break;

            case gln:
              charstate = 'Q';
              break;

            case arg:
              charstate = 'R';
              break;

            case ser:
              charstate = 'S';
              break;

            case thr:
              charstate = 'T';
              break;

            case val:
              charstate = 'V';
              break;

            case trp:
              charstate = 'W';
              break;

            case tyr:
              charstate = 'Y';
              break;

            case glx:
              charstate = 'Z';
              break;

            case del:
              charstate = '-';
              break;

            case stop:
              charstate = '*';
              break;

            case unk:
              charstate = 'X';
              break;

            case quest:
              charstate = '?';
              break;
            
            default:        /*cases ser1 and ser2 cannot occur*/
              break;
            }
          }
          putc(charstate, outfile);
          if (k % 10 == 0 && k % 60 != 0)
            putc(' ', outfile);
        }
        putc('\n', outfile);
      }
      putc('\n', outfile);
    }
    putc('\n', outfile);
  }
  if (printdata)
    putc('\n', outfile);
}  /* protdist_inputdata */


void doinput()
{ /* reads the input data */
  long i;
  double sumrates, weightsum;

  inputoptions();
  protdist_inputdata();
  if (!ctgry) {
    categs = 1;
    rate[0] = 1.0;
  }
  weightsum = 0;
  for (i = 0; i < chars; i++)
    weightsum += oldweight[i];
  sumrates = 0.0;
  for (i = 0; i < chars; i++)
    sumrates += oldweight[i] * rate[category[i] - 1];
  for (i = 0; i < categs; i++)
    rate[i] *= weightsum / sumrates;
}  /* doinput */


void code()
{
  /* make up table of the code 1 = u, 2 = c, 3 = a, 4 = g */
  long n;
  aas b;

  trans[0][0][0] = phe;
  trans[0][0][1] = phe;
  trans[0][0][2] = leu;
  trans[0][0][3] = leu;
  trans[0][1][0] = ser;
  trans[0][1][1] = ser;
  trans[0][1][2] = ser;
  trans[0][1][3] = ser;
  trans[0][2][0] = tyr;
  trans[0][2][1] = tyr;
  trans[0][2][2] = stop;
  trans[0][2][3] = stop;
  trans[0][3][0] = cys;
  trans[0][3][1] = cys;
  trans[0][3][2] = stop;
  trans[0][3][3] = trp;
  trans[1][0][0] = leu;
  trans[1][0][1] = leu;
  trans[1][0][2] = leu;
  trans[1][0][3] = leu;
  trans[1][1][0] = pro;
  trans[1][1][1] = pro;
  trans[1][1][2] = pro;
  trans[1][1][3] = pro;
  trans[1][2][0] = his;
  trans[1][2][1] = his;
  trans[1][2][2] = gln;
  trans[1][2][3] = gln;
  trans[1][3][0] = arg;
  trans[1][3][1] = arg;
  trans[1][3][2] = arg;
  trans[1][3][3] = arg;
  trans[2][0][0] = ileu;
  trans[2][0][1] = ileu;
  trans[2][0][2] = ileu;
  trans[2][0][3] = met;
  trans[2][1][0] = thr;
  trans[2][1][1] = thr;
  trans[2][1][2] = thr;
  trans[2][1][3] = thr;
  trans[2][2][0] = asn;
  trans[2][2][1] = asn;
  trans[2][2][2] = lys;
  trans[2][2][3] = lys;
  trans[2][3][0] = ser;
  trans[2][3][1] = ser;
  trans[2][3][2] = arg;
  trans[2][3][3] = arg;
  trans[3][0][0] = val;
  trans[3][0][1] = val;
  trans[3][0][2] = val;
  trans[3][0][3] = val;
  trans[3][1][0] = ala;
  trans[3][1][1] = ala;
  trans[3][1][2] = ala;
  trans[3][1][3] = ala;
  trans[3][2][0] = asp;
  trans[3][2][1] = asp;
  trans[3][2][2] = glu;
  trans[3][2][3] = glu;
  trans[3][3][0] = gly;
  trans[3][3][1] = gly;
  trans[3][3][2] = gly;
  trans[3][3][3] = gly;
  if (whichcode == mito)
    trans[0][3][2] = trp;
  if (whichcode == vertmito) {
    trans[0][3][2] = trp;
    trans[2][3][2] = stop;
    trans[2][3][3] = stop;
    trans[2][0][2] = met;
  }
  if (whichcode == flymito) {
    trans[0][3][2] = trp;
    trans[2][0][2] = met;
    trans[2][3][2] = ser;
  }
  if (whichcode == yeastmito) {
    trans[0][3][2] = trp;
    trans[1][0][2] = thr;
    trans[2][0][2] = met;
  }
  n = 0;
  for (b = ala; (long)b <= (long)val; b = (aas)((long)b + 1)) {
    if (b != ser2) {
      n++;
      numaa[(long)b - (long)ala] = n;
    }
  }
  numaa[(long)ser - (long)ala] = (long)ser1 - (long)(ala);
}  /* code */


void protdist_cats()
{
  /* define categories of amino acids */
  aas b;

  /* fundamental subgroups */
  cat[0] = 1;                        /* for alanine */
  cat[(long)cys - (long)ala] = 1;
  cat[(long)met - (long)ala] = 2;
  cat[(long)val - (long)ala] = 3;
  cat[(long)leu - (long)ala] = 3;
  cat[(long)ileu - (long)ala] = 3;
  cat[(long)gly - (long)ala] = 4;
  cat[0] = 4;
  cat[(long)ser - (long)ala] = 4;
  cat[(long)thr - (long)ala] = 4;
  cat[(long)pro - (long)ala] = 5;
  cat[(long)phe - (long)ala] = 6;
  cat[(long)tyr - (long)ala] = 6;
  cat[(long)trp - (long)ala] = 6;
  cat[(long)glu - (long)ala] = 7;
  cat[(long)gln - (long)ala] = 7;
  cat[(long)asp - (long)ala] = 7;
  cat[(long)asn - (long)ala] = 7;
  cat[(long)lys - (long)ala] = 8;
  cat[(long)arg - (long)ala] = 8;
  cat[(long)his - (long)ala] = 8;
  if (whichcat == george) {
  /* George, Hunt and Barker: sulfhydryl, small hydrophobic, small hydrophilic,
                              aromatic, acid/acid-amide/hydrophilic, basic */
    for (b = ala; (long)b <= (long)val; b = (aas)((long)b + 1)) {
      if (cat[(long)b - (long)ala] == 3)
        cat[(long)b - (long)ala] = 2;
      if (cat[(long)b - (long)ala] == 5)
        cat[(long)b - (long)ala] = 4;
    }
  }
  if (whichcat == chemical) {
    /* Conn and Stumpf:  monoamino, aliphatic, heterocyclic,
                         aromatic, dicarboxylic, basic */
    for (b = ala; (long)b <= (long)val; b = (aas)((long)b + 1)) {
      if (cat[(long)b - (long)ala] == 2)
        cat[(long)b - (long)ala] = 1;
      if (cat[(long)b - (long)ala] == 4)
        cat[(long)b - (long)ala] = 3;
    }
  }
  /* Ben Hall's personal opinion */
  if (whichcat != hall)
    return;
  for (b = ala; (long)b <= (long)val; b = (aas)((long)b + 1)) {
    if (cat[(long)b - (long)ala] == 3)
      cat[(long)b - (long)ala] = 2;
  }
}  /* protdist_cats */


void maketrans()
{
  /* Make up transition probability matrix from code and category tables */
  long i, j, k, m, n, s, nb1, nb2;
  double x, sum;
  long sub[3], newsub[3];
  double f[4], g[4];
  aas b1, b2;
  double TEMP, TEMP1, TEMP2, TEMP3;

  for (i = 0; i <= 19; i++) {
    pie[i] = 0.0;
    for (j = 0; j <= 19; j++)
      prob[i][j] = 0.0;
  }
  f[0] = freqt;
  f[1] = freqc;
  f[2] = freqa;
  f[3] = freqg;
  g[0] = freqc + freqt;
  g[1] = freqc + freqt;
  g[2] = freqa + freqg;
  g[3] = freqa + freqg;
  TEMP = f[0];
  TEMP1 = f[1];
  TEMP2 = f[2];
  TEMP3 = f[3];
  fracchange = xi * (2 * f[0] * f[1] / g[0] + 2 * f[2] * f[3] / g[2]) +
      xv * (1 - TEMP * TEMP - TEMP1 * TEMP1 - TEMP2 * TEMP2 - TEMP3 * TEMP3);
  sum = 0.0;
  for (i = 0; i <= 3; i++) {
    for (j = 0; j <= 3; j++) {
      for (k = 0; k <= 3; k++) {
        if (trans[i][j][k] != stop)
          sum += f[i] * f[j] * f[k];
      }
    }
  }
  for (i = 0; i <= 3; i++) {
    sub[0] = i + 1;
    for (j = 0; j <= 3; j++) {
      sub[1] = j + 1;
      for (k = 0; k <= 3; k++) {
        sub[2] = k + 1;
        b1 = trans[i][j][k];
        for (m = 0; m <= 2; m++) {
          s = sub[m];
          for (n = 1; n <= 4; n++) {
            memcpy(newsub, sub, sizeof(long) * 3L);
            newsub[m] = n;
            x = f[i] * f[j] * f[k] / (3.0 * sum);
            if (((s == 1 || s == 2) && (n == 3 || n == 4)) ||
                ((n == 1 || n == 2) && (s == 3 || s == 4)))
              x *= xv * f[n - 1];
            else
              x *= xi * f[n - 1] / g[n - 1] + xv * f[n - 1];
            b2 = trans[newsub[0] - 1][newsub[1] - 1][newsub[2] - 1];
            if (b1 != stop) {
              nb1 = numaa[(long)b1 - (long)ala];
              pie[nb1 - 1] += x;
              if (b2 != stop) {
                nb2 = numaa[(long)b2 - (long)ala];
                if (cat[(long)b1 - (long)ala] != cat[(long)b2 - (long)ala]) {
                  prob[nb1 - 1][nb2 - 1] += x * ease;
                  prob[nb1 - 1][nb1 - 1] += x * (1.0 - ease);
                } else
                  prob[nb1 - 1][nb2 - 1] += x;
              } else
                prob[nb1 - 1][nb1 - 1] += x;
            }
          }
        }
      }
    }
  }
  for (i = 0; i <= 19; i++)
    prob[i][i] -= pie[i];
  for (i = 0; i <= 19; i++) {
    for (j = 0; j <= 19; j++)
      prob[i][j] /= sqrt(pie[i] * pie[j]);
  }
  /* computes pi^(1/2)*B*pi^(-1/2)  */
}  /* maketrans */


void givens(double (*a)[20], long i, long j, long n, double ctheta,
                        double stheta, boolean left)
{ /* Givens transform at i,j for 1..n with angle theta */
  long k;
  double d;

  for (k = 0; k < n; k++) {
    if (left) {
      d = ctheta * a[i - 1][k] + stheta * a[j - 1][k];
      a[j - 1][k] = ctheta * a[j - 1][k] - stheta * a[i - 1][k];
      a[i - 1][k] = d;
    } else {
      d = ctheta * a[k][i - 1] + stheta * a[k][j - 1];
      a[k][j - 1] = ctheta * a[k][j - 1] - stheta * a[k][i - 1];
      a[k][i - 1] = d;
    }
  }
}  /* givens */


void coeffs(double x, double y, double *c, double *s, double accuracy)
{ /* compute cosine and sine of theta */
  double root;

  root = sqrt(x * x + y * y);
  if (root < accuracy) {
    *c = 1.0;
    *s = 0.0;
  } else {
    *c = x / root;
    *s = y / root;
  }
}  /* coeffs */


void tridiag(double (*a)[20], long n, double accuracy)
{ /* Givens tridiagonalization */
  long i, j;
  double s, c;

  for (i = 2; i < n; i++) {
    for (j = i + 1; j <= n; j++) {
      coeffs(a[i - 2][i - 1], a[i - 2][j - 1], &c, &s,accuracy);
      givens(a, i, j, n, c, s, true);
      givens(a, i, j, n, c, s, false);
      givens(eigvecs, i, j, n, c, s, true);
    }
  }
}  /* tridiag */


void shiftqr(double (*a)[20], long n, double accuracy)
{ /* QR eigenvalue-finder */
  long i, j;
  double approx, s, c, d, TEMP, TEMP1;

  for (i = n; i >= 2; i--) {
    do {
      TEMP = a[i - 2][i - 2] - a[i - 1][i - 1];
      TEMP1 = a[i - 1][i - 2];
      d = sqrt(TEMP * TEMP + TEMP1 * TEMP1);
      approx = a[i - 2][i - 2] + a[i - 1][i - 1];
      if (a[i - 1][i - 1] < a[i - 2][i - 2])
        approx = (approx - d) / 2.0;
      else
        approx = (approx + d) / 2.0;
      for (j = 0; j < i; j++)
        a[j][j] -= approx;
      for (j = 1; j < i; j++) {
        coeffs(a[j - 1][j - 1], a[j][j - 1], &c, &s, accuracy);
        givens(a, j, j + 1, i, c, s, true);
        givens(a, j, j + 1, i, c, s, false);
        givens(eigvecs, j, j + 1, n, c, s, true);
      }
      for (j = 0; j < i; j++)
        a[j][j] += approx;
    } while (fabs(a[i - 1][i - 2]) > accuracy);
  }
}  /* shiftqr */


void qreigen(double (*prob)[20], long n)
{ /* QR eigenvector/eigenvalue method for symmetric matrix */
  double accuracy;
  long i, j;

  accuracy = 1.0e-6;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++)
      eigvecs[i][j] = 0.0;
    eigvecs[i][i] = 1.0;
  }
  tridiag(prob, n, accuracy);
  shiftqr(prob, n, accuracy);
  for (i = 0; i < n; i++)
    eig[i] = prob[i][i];
  for (i = 0; i <= 19; i++) {
    for (j = 0; j <= 19; j++)
      prob[i][j] = sqrt(pie[j]) * eigvecs[i][j];
  }
  /* prob[i][j] is the value of U' times pi^(1/2) */
}  /* qreigen */


void pameigen()
{ /* eigenanalysis for PAM matrix, precomputed */
  memcpy(prob,pamprobs,sizeof(pamprobs));
  memcpy(eig,pameigs,sizeof(pameigs));
  fracchange = 0.01;
}  /* pameigen */


void jtteigen()
{ /* eigenanalysis for JTT matrix, precomputed */
  memcpy(prob,jttprobs,sizeof(jttprobs));
  memcpy(eig,jtteigs,sizeof(jtteigs));
  fracchange = 0.01;
}  /* jtteigen */


void predict(long nb1, long nb2, long cat)
{ /* make contribution to prediction of this aa pair */
  long m;
  double TEMP;

  for (m = 0; m <= 19; m++) {
    if (gama || invar)
      elambdat = exp(-cvi*log(1.0-rate[cat-1]*tt*(eig[m]/(1.0-invarfrac))/cvi));
    else
      elambdat = exp(rate[cat-1]*tt * eig[m]);
    q = prob[m][nb1 - 1] * prob[m][nb2 - 1] * elambdat;
    p += q;
    if (!gama && !invar)
      dp += rate[cat-1]*eig[m] * q;
    else
      dp += (rate[cat-1]*eig[m]/(1.0-rate[cat-1]*tt*(eig[m]/(1.0-invarfrac))/cvi)) * q;
    TEMP = eig[m];
    if (!gama && !invar)
      d2p += TEMP * TEMP * q;
    else
      d2p += (rate[cat-1]*rate[cat-1]*eig[m]*eig[m]*(1.0+1.0/cvi)/
              ((1.0-rate[cat-1]*tt*eig[m]/cvi)
              *(1.0-rate[cat-1]*tt*eig[m]/cvi))) * q;
  }
  if (nb1 == nb2) {
    p *= (1.0 - invarfrac);
    p += invarfrac;
  }
  dp *= (1.0 - invarfrac);
  d2p *= (1.0 - invarfrac);
}  /* predict */


void makedists()
{ /* compute the distances */
  long i, j, k, m, n, itterations, nb1, nb2, cat;
  double delta, lnlike, slope, curv;
  boolean neginfinity, inf;
  aas b1, b2;

  if (!(printdata || similarity))
    fprintf(outfile, "%5ld\n", spp);
  if (progress)
    printf("Computing distances:\n");
  for (i = 1; i <= spp; i++) {
    if (progress)
      printf("  ");
    if (progress) {
      for (j = 0; j < nmlngth; j++)
        putchar(nayme[i - 1][j]);
    }
    if (progress) {
      printf("   ");
      fflush(stdout);
    }
    if (similarity)
      d[i-1][i-1] = 1.0;
    else
      d[i-1][i-1] = 0.0;
    for (j = 0; j <= i - 2; j++) {
      if (!(kimura || similarity)) {
        if (usejtt || usepam)
          tt = 10.0;
        else
          tt = 1.0;
        delta = tt / 2.0;
        itterations = 0;
        inf = false;
        do {
          lnlike = 0.0;
          slope = 0.0;
          curv = 0.0;
          neginfinity = false;
          for (k = 0; k < chars; k++) {
            if (oldweight[k] > 0) {
              cat = category[k];
              b1 = gnode[i - 1][k];
              b2 = gnode[j][k];
              if (b1 != stop && b1 != del && b1 != quest && b1 != unk &&
                  b2 != stop && b2 != del && b2 != quest && b2 != unk) {
                p = 0.0;
                dp = 0.0;
                d2p = 0.0;
                nb1 = numaa[(long)b1 - (long)ala];
                nb2 = numaa[(long)b2 - (long)ala];
                if (b1 != asx && b1 != glx && b2 != asx && b2 != glx)
                  predict(nb1, nb2, cat);
                else {
                  if (b1 == asx) {
                    if (b2 == asx) {
                      predict(3L, 3L, cat);
                      predict(3L, 4L, cat);
                      predict(4L, 3L, cat);
                      predict(4L, 4L, cat);
                    } else {
                      if (b2 == glx) {
                        predict(3L, 6L, cat);
                        predict(3L, 7L, cat);
                        predict(4L, 6L, cat);
                        predict(4L, 7L, cat);
                      } else {
                        predict(3L, nb2, cat);
                        predict(4L, nb2, cat);
                      }
                    }
                  } else {
                    if (b1 == glx) {
                      if (b2 == asx) {
                        predict(6L, 3L, cat);
                        predict(6L, 4L, cat);
                        predict(7L, 3L, cat);
                        predict(7L, 4L, cat);
                      } else {
                        if (b2 == glx) {
                          predict(6L, 6L, cat);
                          predict(6L, 7L, cat);
                          predict(7L, 6L, cat);
                          predict(7L, 7L, cat);
                        } else {
                          predict(6L, nb2, cat);
                          predict(7L, nb2, cat);
                        }
                      }
                    } else {
                      if (b2 == asx) {
                        predict(nb1, 3L, cat);
                        predict(nb1, 4L, cat);
                        predict(nb1, 3L, cat);
                        predict(nb1, 4L, cat);
                      } else if (b2 == glx) {
                        predict(nb1, 6L, cat);
                        predict(nb1, 7L, cat);
                        predict(nb1, 6L, cat);
                        predict(nb1, 7L, cat);
                      }
                    }
                  }
                }
                if (p <= 0.0)
                  neginfinity = true;
                else {
                  lnlike += oldweight[k]*log(p);
                  slope += oldweight[k]*dp / p;
                  curv += oldweight[k]*(d2p / p - dp * dp / (p * p));
                }
              }
            }
          }
          itterations++;
          if (!neginfinity) {
            if (curv < 0.0) {
              tt -= slope / curv;
              if (tt > 10000.0) {
                printf("\nWARNING: INFINITE DISTANCE BETWEEN SPECIES %ld AND %ld; -1.0 WAS WRITTEN\n", i, j);
                tt = -1.0/fracchange;
                inf = true;
                itterations = 20;
              }
            }
            else {
              if ((slope > 0.0 && delta < 0.0) || (slope < 0.0 && delta > 0.0))
                delta /= -2;
              tt += delta;
            }
          } else {
            delta /= -2;
            tt += delta;
          }
          if (tt < epsilon && !inf)
            tt = epsilon;
        } while (itterations != 20);
      } else {
        m = 0;
        n = 0;
        for (k = 0; k < chars; k++) {
          b1 = gnode[i - 1][k];
          b2 = gnode[j][k];
          if ((long)b1 <= (long)val && (long)b2 <= (long)val) {
            if (b1 == b2)
              m++;
            n++;
          }
        }
        p = 1 - (double)m / n;
        if (kimura) {
          dp = 1.0 - p - 0.2 * p * p;
          if (dp < 0.0) {
            printf(
"\nDISTANCE BETWEEN SEQUENCES %3ld AND %3ld IS TOO LARGE FOR KIMURA FORMULA\n",
              i, j + 1);
            tt = -1.0;
          } else
            tt = -log(dp);
        } else {              /* if similarity */
            tt = 1.0 - p;
        }
      }
      d[i - 1][j] = fracchange * tt;
      d[j][i - 1] = d[i - 1][j];
      if (progress) {
        putchar('.');
        fflush(stdout);
      }
    }
    if (progress) {
      putchar('\n');
      fflush(stdout);
    }
  }
  if (!similarity) {
    for (i = 0; i < spp; i++) {
      for (j = 0; j < nmlngth; j++)
        putc(nayme[i][j], outfile);
      k = spp;
      for (j = 1; j <= k; j++) {
        fprintf(outfile, "%8.4f", d[i][j - 1]);
        if ((j + 1) % 9 == 0 && j < k)
          putc('\n', outfile);
      }
      putc('\n', outfile);
    }
  } else {
    for (i = 0; i < spp; i += 7) {
      if ((i+7) < spp)
        n = i+7;
      else
        n = spp;
      fprintf(outfile, "            ");
      for (j = i; j < n ; j++) {
        for (k = 0; k < (nmlngth-2); k++)
          putc(nayme[j][k], outfile);
        putc(' ', outfile);
      }
      putc('\n', outfile);
      for (j = 0; j < spp; j++) {
        for (k = 0; k < nmlngth; k++)
          putc(nayme[j][k], outfile);
        if ((i+7) < spp)
          n = i+7;
        else
          n = spp;
        for (k = i; k < n ; k++)
          fprintf(outfile, "%9.5f", d[j][k]);
        putc('\n', outfile);
      }
      putc('\n', outfile);
    }
  }
  if (progress)
    printf("\nOutput written to file \"%s\"\n\n", outfilename);
}  /* makedists */


int main(int argc, Char *argv[])
{  /* ML Protein distances by PAM, JTT or categories model */
#ifdef MAC
   argc = 1;   /* macsetup("Protdist",""); */
   argv[0] = "Protdist";
#endif
  init(argc, argv);
  openfile(&infile,INFILE,"input file","r",argv[0],infilename);
  openfile(&outfile,OUTFILE,"output file","w",argv[0],outfilename);
  ibmpc = IBMCRT;
  ansi = ANSICRT;
  mulsets = false;
  datasets = 1;
  firstset = true;
  doinit();
  if (!(kimura || similarity))
    code();
  if (!(usejtt || usepam || kimura || similarity)) {
    protdist_cats();
    maketrans();
    qreigen(prob, 20L);
  } else {
    if (kimura || similarity)
      fracchange = 1.0;
    else {
      if (usejtt)
        jtteigen();
      else 
        pameigen();
    }
  }
  if (ctgry)
    openfile(&catfile,CATFILE,"categories file","r",argv[0],catfilename);
  if (weights || justwts)
    openfile(&weightfile,WEIGHTFILE,"weights file","r",argv[0],weightfilename);
  for (ith = 1; ith <= datasets; ith++) {
    doinput();
    if (ith == 1)
      firstset = false;
    if ((datasets > 1) && progress)
      printf("\nData set # %ld:\n\n", ith);
    makedists();
  }
  FClose(outfile);
  FClose(infile);
#ifdef MAC
  fixmacfile(outfilename);
#endif
  return 0;
}  /* Protein distances */

