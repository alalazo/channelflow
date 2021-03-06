/**
 * This file is a part of channelflow version 2.0.
 * License is GNU GPL version 2 or later: https://channelflow.org/license
 *
 * Original author: Tobias Kreilos
 */
#include <iostream>
#include <memory>
#include <set>
#include "cfbasics/cfbasics.h"
#include "cfbasics/mathdefs.h"
#include "channelflow/cfdsi.h"
#include "channelflow/dns.h"
#include "channelflow/flowfield.h"
#include "channelflow/poissonsolver.h"
#include "channelflow/utilfuncs.h"
#include "nsolver/nsolver.h"

namespace channelflow {

/** \file laurettedsi.h
 * This is an implementation of an algorithm to search for fixed points
 * brought and explained to me by Laurette Tuckerman.
 *
 * Consider an equation of the form du/dt = Lu + N(u), with L linear and N nonlinear.
 * We want so solve du/dt = 0 with a Newton method:
 * J Du_n = -(L+N)u_n,
 * with J the Jacobian of du/dt at u_n: J = N_u + L
 * and Du_n the update step u_{n+1} - u_n
 *
 * Multiply by (I-L)^{-1} dt to obtain
 * [(I-dt L)^-1 (I + dt N_u) Du_n = -[(I-dtL)^-1 (I+dtN) - I] u_n
 *
 * This is a linear system of equations which can be solved iteratively if we
 * know the rhs and the action of the left operator on a test vector.
 * The advantage of the system compared to the original one is that it is way
 * better conditioned (due to the $L^{-1}$ which acts as a preconditioner) and
 * that the action of the operators can be easiliy computed since:
 * $u(t+\Delta t) - u(t) = u(t) + \Delta t[N(u(t)) + L u(t+\Delta t)] - u(t) = \Delta t N u(t) + \Delta t L u(t+\delta
 * t)$
 * $\Rightarrow u(t+\Delta t) - \Delta t L u(t+\Delta t) = (\Delta t N + *i) u(t)$
 * $\Rightarrow u(t+\Delta t) = (I-\Delta t L)^{-1}(I+\Delta t N) u(t)$
 *  i.e. the rhs is just the difference between two timesteps and the operator
 * on the lhs just the difference between two linearized timesteps.
 * The $\Delta t$ should be large since it is not the accuracy of the
 * timesteps that matters but the preconditioning action of the $L^{-1}$ term.
 */

/** \brief nonlinear part of Navier Stokes operator, linearized about U, applied to u.
 * (u dot grad) U + (U dot grad) u
 *
 * \param[in] u operator acts on this field
 * \param[in] U operator is linearized about this field
 * \param[in] Ubase base flow in x direction
 * \param[in] Wbase base flow in y direction
 * \param[out] f result: (u dot grad) U + (U dot grad) u
 * \param tmp field used for intermediate computations
 * \param[in] flags specify flow paramaeters and boundary conditions
 */
void navierstokesNL_linearU(const FlowField& u, const FlowField& U, const ChebyCoeff Ubase, const ChebyCoeff Wbase,
                            FlowField& f, FlowField& tmp, DNSFlags& flags, const bool quad = false);

/** \brief Implements first order forward-euler backward-euler
 * timestepping algorithm. Don't use this in time-integrations.
 * This DNS Algorithm is used in the computations in Laurettes method.
 *
 * Basically a clone of SBDF1, but it contains an additional advance function that allows
 * to use a nonlinear operator that is linearized about another field.
 *
 * FS: Instead of copying the MultistepDNS class it is now derived and overloads the advance and clone method.
 * The timestepping flag FEBE was removed because of its reduncance with SBDF1.
 * Works as before also with generalized time stepper classes (16.10.2016)
 */
class EulerDNS : public MultistepDNS {
   public:
    EulerDNS();
    EulerDNS(const EulerDNS& dns);
    EulerDNS(const vector<FlowField>& fields, const shared_ptr<NSE>& nse, const DNSFlags& flags);

    ~EulerDNS();
    EulerDNS& operator=(const EulerDNS& dns);

    // TODO: Check if overloading a virtual function is something we want or just an error
    using MultistepDNS::advance;
    virtual void advance(vector<FlowField>& fieldsn, int Nsteps, FlowField& linearU, bool linearize, Real Cx, Real cx,
                         bool quad);

    DNSAlgorithm* clone(const shared_ptr<NSE>& nse) const override;  // new copy of *this
};

/** \brief Dynamical systems interface for Laurettes fixed point method */
class LauretteDSI : public cfDSI {
   public:
    LauretteDSI(FlowField& u, DNSFlags& flags, Real dt, bool xrel, bool zrel, FieldSymmetry sigma);
    virtual ~LauretteDSI();
    VectorXd eval(const VectorXd& x);
    virtual VectorXd Jacobian(const VectorXd& x, const VectorXd& dx, const VectorXd& Gx, const cfbasics::Real& epsDx,
                              bool centdiff, int& fcount);
    virtual void updateMu(cfbasics::Real mu);

    VectorXd Q(const VectorXd& w);
    //   virtual VectorXd makeVector(const FlowField& u, FieldSymmetry& sigma, Real T);

    Real shift2speed(Real ax);

   private:
    //   FlowField ut_, udt_, U_;
    vector<FlowField> fieldst_, fieldsdt_;
    FlowField U_;
    //   FlowField p;
    Real dt_;

    shared_ptr<NSE> nse;
    unique_ptr<EulerDNS> alg;
};

}  // namespace channelflow