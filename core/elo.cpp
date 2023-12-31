#include "../core/elo.h"

#include <cmath>

#include "../core/test.h"
#include "../core/os.h"

using namespace std;

static const double ELO_PER_LOG_GAMMA = 173.717792761;

// 计算 log(1 + exp(x))
static double logOnePlusExpX(double x)
{
  if (x >= 50)
    return 50;
  return log(1 + exp(x));
}

// 计算 log(1 + exp(x)) 的二阶导数
static double logOnePlusExpXSecondDerivative(double x)
{
  double halfX = 0.5 * x;
  double denom = exp(halfX) + exp(-halfX);
  return 1 / (denom * denom);
}

// 根据elo差值计算第一个获胜的概率
double ComputeElos::probWin(double eloDiff)
{
  double logGammaDiff = eloDiff / ELO_PER_LOG_GAMMA;
  return 1.0 / (1.0 + exp(-logGammaDiff));
}

// 计算给定胜负记录的对数似然
static double logLikelihoodOfWL(
    double eloFirstMinusSecond,
    ComputeElos::WLRecord winRecord)
{
  double logGammaFirstMinusSecond = eloFirstMinusSecond / ELO_PER_LOG_GAMMA;
  double logProbFirstWin = -logOnePlusExpX(-logGammaFirstMinusSecond);
  double logProbSecondWin = -logOnePlusExpX(logGammaFirstMinusSecond);
  return winRecord.firstWins * logProbFirstWin + winRecord.secondWins * logProbSecondWin;
}

// 计算给定胜负记录对数似然的二阶导数
static double logLikelihoodOfWLSecondDerivative(
    double eloFirstMinusSecond,
    ComputeElos::WLRecord winRecord)
{
  double logGammaFirstMinusSecond = eloFirstMinusSecond / ELO_PER_LOG_GAMMA;
  double logProbFirstWinSecondDerivative = -logOnePlusExpXSecondDerivative(-logGammaFirstMinusSecond);
  double logProbSecondWinSecondDerivative = -logOnePlusExpXSecondDerivative(logGammaFirstMinusSecond);
  return (winRecord.firstWins * logProbFirstWinSecondDerivative + winRecord.secondWins * logProbSecondWinSecondDerivative) / (ELO_PER_LOG_GAMMA * ELO_PER_LOG_GAMMA);
}

// 计算给定球员的局部对数似然
static double computeLocalLogLikelihood(
    int player,
    const vector<double> &elos,
    const ComputeElos::WLRecord *winMatrix,
    int numPlayers,
    double priorWL)
{
  double logLikelihood = 0.0;
  for (int y = 0; y < numPlayers; y++)
  {
    if (y == player)
      continue;
    logLikelihood += logLikelihoodOfWL(elos[player] - elos[y], winMatrix[player * numPlayers + y]);
    logLikelihood += logLikelihoodOfWL(elos[y] - elos[player], winMatrix[y * numPlayers + player]);
  }
  logLikelihood += logLikelihoodOfWL(elos[player] - 0.0, ComputeElos::WLRecord(priorWL, priorWL));

  return logLikelihood;
}

// 计算给定球员局部对数似然的二阶导数
static double computeLocalLogLikelihoodSecondDerivative(
    int player,
    const vector<double> &elos,
    const ComputeElos::WLRecord *winMatrix,
    int numPlayers,
    double priorWL)
{
  double logLikelihoodSecondDerivative = 0.0;
  for (int y = 0; y < numPlayers; y++)
  {
    if (y == player)
      continue;
    logLikelihoodSecondDerivative += logLikelihoodOfWLSecondDerivative(elos[player] - elos[y], winMatrix[player * numPlayers + y]);
    logLikelihoodSecondDerivative += logLikelihoodOfWLSecondDerivative(elos[y] - elos[player], winMatrix[y * numPlayers + player]);
  }
  logLikelihoodSecondDerivative += logLikelihoodOfWLSecondDerivative(elos[player] - 0.0, ComputeElos::WLRecord(priorWL, priorWL));

  return logLikelihoodSecondDerivative;
}

// 近似计算所有球员Elo的标准差
vector<double> ComputeElos::computeApproxEloStdevs(
    const vector<double> &elos,
    const ComputeElos::WLRecord *winMatrix,
    int numPlayers,
    double priorWL)
{
  // 粗略离散建模分布并查看标准差
  vector<double> eloStdevs(numPlayers, 0.0);

  const int radius = 1500;
  vector<double> relProbs(radius*2+1,0.0);
  const double step = 1.0; //one-elo increments

  for(int player = 0; player < numPlayers; player++) {
    double logLikelihood = computeLocalLogLikelihood(player,elos,winMatrix,numPlayers,priorWL);
    double sumRelProbs = 0.0;
    vector<double> tempElos = elos;
    for(int i = 0; i < radius*2+1; i++) {
      double elo = elos[player] + (i - radius) * step;
      tempElos[player] = elo;
      double newLogLikelihood = computeLocalLogLikelihood(player,tempElos,winMatrix,numPlayers,priorWL);
      relProbs[i] = exp(newLogLikelihood-logLikelihood);
      sumRelProbs += relProbs[i];
    }

    double secondMomentAroundElo = 0.0;
    for(int i = 0; i < radius*2+1; i++) {
      double elo = elos[player] + (i - radius) * step;
      secondMomentAroundElo += relProbs[i] / sumRelProbs * (elo - elos[player]) * (elo - elos[player]);
    }
    eloStdevs[player] = sqrt(secondMomentAroundElo);
  }
  return eloStdevs;
}

// 计算Elo评分
vector<double> ComputeElos::computeElos(
  const ComputeElos::WLRecord* winMatrix, // 胜负记录
  int numPlayers, // 玩家数量
  double priorWL, // 先验水平
  int maxIters, // 最大迭代次数 
  double tolerance, // 容差
  ostream* out // 输出流
) {

  vector<double> elos(numPlayers,0.0); // Elo分数组

  // 常规无梯度算法
  vector<double> nextDelta(numPlayers,100.0); 

  auto iterate = [&]() {
    
    double maxEloDiff = 0;
    for(int x = 0; x<numPlayers; x++) {
    
      double oldElo = elos[x]; // 原Elo分
      double hiElo = oldElo + nextDelta[x]; // 高分
      double loElo = oldElo - nextDelta[x]; // 低分

      // 计算对数似然
      double likelihood = computeLocalLogLikelihood(x, elos, winMatrix, numPlayers, priorWL);
      
      // 尝试高低分判断
      elos[x] = hiElo; 
      double likelihoodHi = computeLocalLogLikelihood(x, elos, winMatrix, numPlayers, priorWL);

      elos[x] = loElo;
      double likelihoodLo = computeLocalLogLikelihood(x, elos, winMatrix, numPlayers, priorWL);

      // 调整Elo和delta
      if (likelihoodHi > likelihood) {
        elos[x] = hiElo;
        nextDelta[x] *= 1.1;
      } else if (likelihoodLo > likelihood) {
        elos[x] = loElo;
        nextDelta[x] *= 1.1;
      } else {
        elos[x] = oldElo;
        nextDelta[x] *= 0.8;
      }

      // 记录最大变化
      double eloDiff = nextDelta[x];
      maxEloDiff = max(eloDiff, maxEloDiff);
    }
    
    return maxEloDiff;
  };

  // 迭代计算
  for (int i = 0; i < maxIters; i++) {
    double maxEloDiff = iterate();

    // 打印输出
    if (out != NULL && i % 50 == 0) {
      (*out) << "Iteration " << i << " maxEloDiff = " << maxEloDiff << endl; 
    }
    if (maxEloDiff < tolerance)
      break;
  }

  return elos;
}

// 判断两个double是否足够接近
static bool approxEqual(double x, double y, double tolerance)
{
  return std::fabs(x - y) < tolerance;
}

void ComputeElos::runTests()
{
  ostringstream out;
  // Avoid complaint on windows about not calling a function with arguments - the point is to ignore the function
#ifdef OS_IS_WINDOWS
#pragma warning(suppress : 4551)
  (void)computeLocalLogLikelihoodSecondDerivative;
#else
  (void)computeLocalLogLikelihoodSecondDerivative;
#endif

  double optTolerance = 0.00001;
  double testTolerance = 0.01;

  {
    // const char* name = "Elo test 0";

    int numPlayers = 1;
    ComputeElos::WLRecord *winMatrix = new ComputeElos::WLRecord[numPlayers * numPlayers];
    double priorWL = 0.1;
    int maxIters = 1000;

    vector<double> elos = ComputeElos::computeElos(winMatrix, numPlayers, priorWL, maxIters, optTolerance, &out);
    testAssert(elos.size() == 1);
    testAssert(approxEqual(elos[0], 0.0, testTolerance));

    //     string expected = R"%%(
    // Iteration 0 maxEloDiff = 80
    // Iteration 50 maxEloDiff = 0.0011418
    // Elo 0 = 0 stdev 780.104 2nd der -1.65684e-06 approx -1.65684e-06
    // )%%";

    //     printEloStuff(elos,winMatrix,numPlayers,priorWL);

    //     TestCommon::expect(name,out,expected);
    delete[] winMatrix;
  }

  {
    // const char* name = "Elo test 1";

    int numPlayers = 3;
    ComputeElos::WLRecord *winMatrix = new ComputeElos::WLRecord[numPlayers * numPlayers];
    double priorWL = 1.0;
    int maxIters = 1000;

    winMatrix[0 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 2] = ComputeElos::WLRecord(200.0, 0.0);
    winMatrix[2 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[2 * numPlayers + 1] = ComputeElos::WLRecord(100.0, 0.0);
    winMatrix[2 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);

    vector<double> elos = ComputeElos::computeElos(winMatrix, numPlayers, priorWL, maxIters, optTolerance, &out);
    testAssert(elos.size() == 3);
    testAssert(approxEqual(elos[0], 0.0, testTolerance));
    testAssert(approxEqual(elos[1], 59.9833, testTolerance));
    testAssert(approxEqual(elos[2], -59.9834, testTolerance));

    //     string expected = R"%%(
    // Iteration 0 maxEloDiff = 110
    // Iteration 50 maxEloDiff = 0.135559
    // Iteration 100 maxEloDiff = 0.0708954
    // Iteration 150 maxEloDiff = 0.0509811
    // Iteration 200 maxEloDiff = 0.0266623
    // Iteration 250 maxEloDiff = 0.0191729
    // Iteration 300 maxEloDiff = 0.0137873
    // Iteration 350 maxEloDiff = 0.00721055
    // Iteration 400 maxEloDiff = 0.00518513
    // Iteration 450 maxEloDiff = 0.000291829
    // Elo 0 = 2.37142e-07 stdev 313.547 2nd der -1.65684e-05 approx -1.65684e-05
    // Elo 1 = 59.9831 stdev 21.2381 2nd der -0.00222709 approx -0.00222709
    // Elo 2 = -59.9836 stdev 21.2381 2nd der -0.00222709 approx -0.00222709
    // )%%";

    //     printEloStuff(elos,winMatrix,numPlayers,priorWL);

    //     TestCommon::expect(name,out,expected);
    delete[] winMatrix;
  }

  {
    // const char* name = "Elo test 2";

    int numPlayers = 3;
    ComputeElos::WLRecord *winMatrix = new ComputeElos::WLRecord[numPlayers * numPlayers];
    double priorWL = 1.0;
    int maxIters = 1000;

    winMatrix[0 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 1.0);
    winMatrix[1 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 2] = ComputeElos::WLRecord(5.0, 0.0);
    winMatrix[2 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 5.0);
    winMatrix[2 * numPlayers + 1] = ComputeElos::WLRecord(1.0, 0.0);
    winMatrix[2 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);

    vector<double> elos = ComputeElos::computeElos(winMatrix, numPlayers, priorWL, maxIters, optTolerance, &out);
    testAssert(elos.size() == 3);
    testAssert(approxEqual(elos[0], 76.5228, testTolerance));
    testAssert(approxEqual(elos[1], 76.5228, testTolerance));
    testAssert(approxEqual(elos[2], -161.285, testTolerance));

    //     string expected = R"%%(
    // Iteration 0 maxEloDiff = 110
    // Iteration 50 maxEloDiff = 0.0985887
    // Iteration 100 maxEloDiff = 8.83612e-05
    // Elo 0 = 76.5227 stdev 162.965 2nd der -4.7933e-05 approx -4.7933e-05
    // Elo 1 = 76.5227 stdev 162.965 2nd der -4.7933e-05 approx -4.7933e-05
    // Elo 2 = -161.285 stdev 123.894 2nd der -7.77407e-05 approx -7.77407e-05
    // )%%";

    //     printEloStuff(elos,winMatrix,numPlayers,priorWL);

    //     TestCommon::expect(name,out,expected);
    delete[] winMatrix;
  }

  {
    // const char* name = "Elo test 3";

    int numPlayers = 3;
    ComputeElos::WLRecord *winMatrix = new ComputeElos::WLRecord[numPlayers * numPlayers];
    double priorWL = 1.0;
    int maxIters = 1000;

    winMatrix[0 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 1.0);
    winMatrix[2 * numPlayers + 0] = ComputeElos::WLRecord(5.0, 1.0);
    winMatrix[2 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 5.0);
    winMatrix[2 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);

    vector<double> elos = ComputeElos::computeElos(winMatrix, numPlayers, priorWL, maxIters, optTolerance, &out);
    testAssert(elos.size() == 3);
    testAssert(approxEqual(elos[0], -190.849, testTolerance));
    testAssert(approxEqual(elos[1], 190.849, testTolerance));
    testAssert(approxEqual(elos[2], 0, testTolerance));

    //     string expected = R"%%(
    // Iteration 0 maxEloDiff = 110
    // Iteration 50 maxEloDiff = 0.916107
    // Iteration 100 maxEloDiff = 0.0272716
    // Iteration 150 maxEloDiff = 0.000590434
    // Elo 0 = -190.849 stdev 161.134 2nd der -4.97053e-05 approx -4.97053e-05
    // Elo 1 = 190.849 stdev 161.134 2nd der -4.97053e-05 approx -4.97053e-05
    // Elo 2 = 6.54304e-07 stdev 106.849 2nd der -9.11264e-05 approx -9.11263e-05
    // )%%";

    //     printEloStuff(elos,winMatrix,numPlayers,priorWL);

    //     TestCommon::expect(name,out,expected);
    delete[] winMatrix;
  }

  {
    // const char* name = "Elo test 3";

    int numPlayers = 3;
    ComputeElos::WLRecord *winMatrix = new ComputeElos::WLRecord[numPlayers * numPlayers];
    double priorWL = 0.1;
    int maxIters = 10000;

    winMatrix[0 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 1.0);
    winMatrix[2 * numPlayers + 0] = ComputeElos::WLRecord(5.0, 1.0);
    winMatrix[2 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 5.0);
    winMatrix[2 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);

    vector<double> elos = ComputeElos::computeElos(winMatrix, numPlayers, priorWL, maxIters, optTolerance, &out);
    testAssert(elos.size() == 3);
    testAssert(approxEqual(elos[0], -266.471, testTolerance));
    testAssert(approxEqual(elos[1], 266.471, testTolerance));
    testAssert(approxEqual(elos[2], 0, testTolerance));

    //     string expected = R"%%(
    // Iteration 0 maxEloDiff = 110
    // Iteration 50 maxEloDiff = 1.25965
    // Iteration 100 maxEloDiff = 0.0198339
    // Iteration 150 maxEloDiff = 0.000429407
    // Elo 0 = -266.471 stdev 234.178 2nd der -2.99835e-05 approx -2.99835e-05
    // Elo 1 = 266.471 stdev 234.178 2nd der -2.99835e-05 approx -2.99835e-05
    // Elo 2 = 7.36479e-07 stdev 128.942 2nd der -5.96894e-05 approx -5.96895e-05
    // )%%";

    //     printEloStuff(elos,winMatrix,numPlayers,priorWL);

    //     TestCommon::expect(name,out,expected);
    delete[] winMatrix;
  }

  {
    // const char* name = "Elo test 4";

    int numPlayers = 3;
    ComputeElos::WLRecord *winMatrix = new ComputeElos::WLRecord[numPlayers * numPlayers];
    double priorWL = 0.01;
    int maxIters = 10000;

    winMatrix[0 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[0 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 0] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 0.0);
    winMatrix[1 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 1.0);
    winMatrix[2 * numPlayers + 0] = ComputeElos::WLRecord(7.0, 1.0);
    winMatrix[2 * numPlayers + 1] = ComputeElos::WLRecord(0.0, 5.0);
    winMatrix[2 * numPlayers + 2] = ComputeElos::WLRecord(0.0, 0.0);

    vector<double> elos = ComputeElos::computeElos(winMatrix, numPlayers, priorWL, maxIters, optTolerance, &out);
    testAssert(elos.size() == 3);
    testAssert(approxEqual(elos[0], -322.013, testTolerance));
    testAssert(approxEqual(elos[1], 292.742, testTolerance));
    testAssert(approxEqual(elos[2], 14.5828, testTolerance));

    //     string expected = R"%%(
    // Iteration 0 maxEloDiff = 110
    // Iteration 50 maxEloDiff = 1.25965
    // Iteration 100 maxEloDiff = 0.479109
    // Iteration 150 maxEloDiff = 0.344529
    // Iteration 200 maxEloDiff = 0.340659
    // Iteration 250 maxEloDiff = 0.178159
    // Iteration 300 maxEloDiff = 0.176158
    // Iteration 350 maxEloDiff = 0.0921276
    // Iteration 400 maxEloDiff = 0.0662492
    // Iteration 450 maxEloDiff = 0.04764
    // Iteration 500 maxEloDiff = 0.0342581
    // Iteration 550 maxEloDiff = 0.0246351
    // Iteration 600 maxEloDiff = 0.0177152
    // Iteration 650 maxEloDiff = 0.012739
    // Iteration 700 maxEloDiff = 0.00916067
    // Iteration 750 maxEloDiff = 0.00658747
    // Iteration 800 maxEloDiff = 0.00473707
    // Iteration 850 maxEloDiff = 0.00247741
    // Iteration 900 maxEloDiff = 0.00244958
    // Iteration 950 maxEloDiff = 0.00128109
    // Iteration 1000 maxEloDiff = 0.000921237
    // Iteration 1050 maxEloDiff = 0.000662464
    // Iteration 1100 maxEloDiff = 0.000346458
    // Iteration 1150 maxEloDiff = 0.000249139
    // Iteration 1200 maxEloDiff = 0.000179157
    // Elo 0 = -322.005 stdev 246.125 2nd der -2.92534e-05 approx -2.92534e-05
    // Elo 1 = 292.751 stdev 248.472 2nd der -2.7853e-05 approx -2.78531e-05
    // Elo 2 = 14.5915 stdev 129.788 2nd der -5.71067e-05 approx -5.71068e-05
    // )%%";

    //     printEloStuff(elos,winMatrix,numPlayers,priorWL);

    //     TestCommon::expect(name,out,expected);
    delete[] winMatrix;
  }
}
