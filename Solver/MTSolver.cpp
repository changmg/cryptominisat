#include "MTSolver.h"

#include <set>
#include <omp.h>

void MTSolver::printNumThreads() const
{
    if (conf.verbosity >= 1) {
        std::cout << "c Using " << numThreads << " threads" << std::endl;
    }
}

MTSolver::MTSolver(const int _numThreads, const SolverConf& _conf, const GaussConf& _gaussConfig) :
    conf(_conf)
    , gaussConfig(_gaussConfig)
    , numThreads(_numThreads)
{
    omp_set_dynamic(0);
    solvers.resize(numThreads, NULL);

    #pragma omp parallel num_threads(numThreads)
    {
        setupOneSolver();
    }
    finishedThread = 0;
}

MTSolver::~MTSolver()
{
    for (uint32_t i = 0; i < solvers.size(); i++) {
        delete solvers[i];
    }
}

void MTSolver::setupOneSolver()
{
    SolverConf myConf = conf;
    int num = omp_get_thread_num();
    myConf.origSeed = num;
    if (num > 0) {
        //if (num % 2) myConf.fixRestartType = dynamic_restart;
        //else myConf.fixRestartType = static_restart;
        myConf.simpBurstSConf *= 1.0 + num;
        myConf.simpStartMult *= 1.0 + 0.2*num;
        myConf.simpStartMMult *= 1.0 + 0.2*num;
        if (num == numThreads-1) {
            //myConf.doVarElim = false;
            myConf.doPerformPreSimp = false;
            myConf.polarity_mode = polarity_false;
        }
    }
    if (num != 0) myConf.verbosity = 0;

    Solver* solver = new Solver(myConf, gaussConfig, &sharedData);
    solvers[num] = solver;
}



const lbool MTSolver::solve()
{
    std::set<uint32_t> finished;
    lbool retVal;

    #pragma omp parallel num_threads(numThreads)
    {
        int threadNum = omp_get_thread_num();
        lbool ret = solvers[threadNum]->solve();

        #pragma omp critical (finished)
        {
            finished.insert(threadNum);
        }

        #pragma omp single
        {
            int numNeededInterrupt = 0;
            for(int i = 0; i < numThreads; i++) {
                if (i != threadNum && solvers[i]->getNeedToInterrupt() == false) {
                    solvers[i]->setNeedToInterrupt();
                    numNeededInterrupt++;
                }
            }
            assert(numNeededInterrupt == numThreads-1);

            bool mustWait = true;
            while (mustWait) {
                #pragma omp critical (finished)
                if (finished.size() == (unsigned)numThreads) mustWait = false;
            }

            finishedThread = threadNum;
            retVal = ret;
            setUpFinish(threadNum);
        }
    }

    return retVal;
}

void MTSolver::setUpFinish(const int threadNum)
{
    Solver& solver = *solvers[threadNum];
    model.clear();
    model.growTo(solver.model.size());
    std::copy(solver.model.getData(), solver.model.getDataEnd(), model.getData());

    conflict.clear();
    conflict.growTo(solver.conflict.size());
    std::copy(solver.conflict.getData(), solver.conflict.getDataEnd(), conflict.getData());
}

Var MTSolver::newVar(bool dvar)
{
    uint32_t ret = 0;
    #pragma omp parallel for firstprivate(ret) num_threads(numThreads)
    for (uint32_t i = 0; i < solvers.size(); i++) {
        ret = solvers[i]->newVar();
    }

    return ret;
}

template<class T> bool MTSolver::addClause (T& ps, const uint32_t group, const char* group_name)
{
    bool globalRet = true;
    #pragma omp parallel for num_threads(numThreads)
    for (uint32_t i = 0; i < solvers.size(); i++) {
        vec<Lit> copyPS(ps.size());
        std::copy(ps.getData(), ps.getDataEnd(), copyPS.getData());
        bool ret = solvers[i]->addClause(copyPS, group, group_name);
        if (ret == false) {
            #pragma omp critical
            globalRet = false;
        }
    }

    return globalRet;
}
template bool MTSolver::addClause(vec<Lit>& ps, const uint32_t group, const char* group_name);
template bool MTSolver::addClause(Clause& ps, const uint32_t group, const char* group_name);

template<class T> bool MTSolver::addLearntClause(T& ps, const uint32_t group, const char* group_name, const uint32_t glue, const float miniSatActivity)
{
    bool globalRet = true;
    #pragma omp parallel for num_threads(numThreads)
    for (uint32_t i = 0; i < solvers.size(); i++) {
        vec<Lit> copyPS(ps.size());
        std::copy(ps.getData(), ps.getDataEnd(), copyPS.getData());
        bool ret = solvers[i]->addLearntClause(copyPS, group, group_name, glue, miniSatActivity);
        if (ret == false) {
            #pragma omp critical
            globalRet = false;
        }
    }

    return globalRet;
}
template bool MTSolver::addLearntClause(vec<Lit>& ps, const uint32_t group, const char* group_name, const uint32_t glue, const float miniSatActivity);
template bool MTSolver::addLearntClause(Clause& ps, const uint32_t group, const char* group_name, const uint32_t glue, const float miniSatActivity);

template<class T> bool MTSolver::addXorClause (T& ps, bool xorEqualFalse, const uint32_t group, const char* group_name)
{
    bool globalRet = true;
    #pragma omp parallel for num_threads(numThreads)
    for (uint32_t i = 0; i < solvers.size(); i++) {
        vec<Lit> copyPS(ps.size());
        std::copy(ps.getData(), ps.getDataEnd(), copyPS.getData());
        bool ret = solvers[i]->addXorClause(copyPS, xorEqualFalse, group, group_name);
        if (ret == false) {
            #pragma omp critical
            globalRet = false;
        }
    }

    return globalRet;
}
template bool MTSolver::addXorClause(vec<Lit>& ps, bool xorEqualFalse, const uint32_t group, const char* group_name);
template bool MTSolver::addXorClause(XorClause& ps, bool xorEqualFalse, const uint32_t group, const char* group_name);


void MTSolver::printStats()
{
    Solver& solver = *solvers[finishedThread];
    solver.printStats(numThreads);
}

void MTSolver::setNeedToInterrupt()
{
    for (uint32_t i = 0; i < solvers.size(); i++) {
        solvers[i]->setNeedToInterrupt();
    }
}

void MTSolver::dumpSortedLearnts(const std::string& fileName, const uint32_t maxSize)
{
    solvers[finishedThread]->dumpSortedLearnts(fileName, maxSize);
}

void MTSolver::dumpOrigClauses(const std::string& fileName, const bool alsoLearntBin) const
{
    solvers[finishedThread]->dumpSortedLearnts(fileName, alsoLearntBin);
}

void MTSolver::setVariableName(const Var var, const std::string& name)
{
    for (uint32_t i = 0; i < solvers.size(); i++) {
        solvers[i]->setVariableName(var, name);
    }
}

//void needLibraryCNFFile(const std::string& fileName); //creates file in current directory with the filename indicated, and puts all calls from the library into the file.

