#include <sched.h>

#include "../Helpers/Console/CommandLineParser.h"
#include "../Shell/Shell.h"
#include "Commands/Flow.h"

using namespace Shell;

int main(int argc, char** argv) {
    CommandLineParser clp(argc, argv);
    checkAsserts();
    Shell::Shell shell;
    new LoadMaxFlowInstanceFromDimacs(shell);
    new MakeStaticMaxFlowInstanceParametric(shell);
    new LoadParametricMaxFlowInstanceFromDimacs(shell);
    new RunPushRelabel(shell);
    new TestParametricPushRelabel(shell);
    new RunIBFS(shell);
    new RunExcessesIBFS(shell);
    new TestRestartableIBFS(shell);
    new RunParametricIBFS(shell);
    new TestParametricIBFS(shell);
    new RunChordScheme(shell);
    new TestChordScheme(shell);
    new PrecisionExperiment(shell);
    shell.run();
    return 0;
}