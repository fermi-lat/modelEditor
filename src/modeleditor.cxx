/**
 * @file modeleditor.cxx
 * @brief Driver for running ModelEditor.py
 *
 * @author J. Asercion <Joseph.A.Asercion@nasa.gov>
 *
 */

#include <cstdlib>
#include <sstream>

int main(int iargc, char * argv[]) {
   std::ostringstream command;
   command << "python $CONDA_PREFIX/lib/python2.7/site-packages/fermitools/ModelEditor.py";
   std::system(command.str().c_str());
}
