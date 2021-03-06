/**
 * This file is a part of channelflow version 2.0.
 * License is GNU GPL version 2 or later: https://channelflow.org/license
 */

#ifndef NSOLVER_BICGSTAB_H
#define NSOLVER_BICGSTAB_H

#include "cfbasics/cfbasics.h"
using namespace cfbasics;

namespace nsolver {

class BiCGStab {
    // usage to solve A*x = b
    //   BiCGStab bicgstab(b);
    //   for (int i=0; i<maxiter; i++) {
    //     VectorXd p = bicgstab.step1();
    //     VectorXd Ap = A*p;
    //     VectorXd s = bicstab.step2(Ap);
    //     VectorXd As = A*s;
    //     bicstab.step3(As);
    //     if (bicgstab.residual() < eps)
    //       break;
    //   }
    //   return bicgstab.solution();

    // The residual decreases monotonically with i since always the
    // best solution is returned by residual() and solution().
    // step3() returns the current progress of the iteration, which may
    // differ from solution() if the last step led to an increase in the
    // residual.

   public:
    BiCGStab(VectorXd b);
    VectorXd step1();
    VectorXd step2(VectorXd& Ap);
    VectorXd step3(VectorXd& As);

    VectorXd solution();
    Real residual();

   private:
    VectorXd r, r0;
    Real r0_sqnorm, rhs_sqnorm;
    Real rho, alpha, w, rho_old, beta;
    VectorXd v, p, kt, ks, s, t, x, solution_;
    Real residual_;
};

}  // namespace nsolver

#endif
