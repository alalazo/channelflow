/**
 * This file is a part of channelflow version 2.0.
 * License is GNU GPL version 2 or later: https://channelflow.org/license
 *
 * Based on the benchmark program from examples
 */
#include <sys/time.h>
#include <iostream>
#include <limits>
#include "channelflow/dns.h"
#include "channelflow/flowfield.h"
#include "channelflow/utilfuncs.h"
#ifdef HAVE_MPI
#include <mpi.h>
#endif
#include "channelflow/cfmpi.h"

using namespace std;
using namespace channelflow;

int main(int argc, char* argv[]) {
    int failure = 0;
    bool verbose = true;
    cfMPI_Init(&argc, &argv);
    {
        int taskid = mpirank();
        if (taskid == 0) {
            cerr << "time_integrationTest: " << flush;
            if (verbose) {
                cout << "\n====================================================" << endl;
                cout << "time_integrationTest\n\n";
            }
        }

        string purpose(
            "This program loads the field uinit from the harddisk and integrates\nit for 10 time units. The resulting "
            "field is compared to the field\nufinal. Wall-clock time elapsed for each timeunit as well as an\naverage "
            "are calculated.");
        ArgList args(argc, argv, purpose);
        int nproc0 = args.getint("-np0", "-nproc0", 0, "number of processes for transpose/parallel ffts");
        int nproc1 = args.getint("-np1", "-nproc1", 0, "number of processes for slice fft");
        bool fftwmeasure = args.getflag("-fftwmeasure", "--fftwmeasure", "use fftw_measure instead of fftw_patient");
        Real tol = args.getreal("-t", "--tolerance", 1.0e-13, "max distance allowed for the test to pass");
        string dir = "data";
        args.check();

        if (taskid == 0 && verbose)
            cout << "Creating CfMPI object..." << flush;
        CfMPI* cfmpi = &CfMPI::getInstance(nproc0, nproc1);
        if (taskid == 0 && verbose)
            cout << "done" << endl;

        if (taskid == 0 && verbose)
            cout << "Loading FlowField..." << endl;
        FlowField u(dir + "/uinit", cfmpi);
        if (taskid == 0 && verbose)
            cout << "done" << endl;

        if (u.taskid() == 0 && verbose) {
            cout << "================================================================\n";
            cout << purpose << endl << endl;
            cout << "Distribution of processes is " << u.nproc0() << "x" << u.nproc1() << endl;
        }
        FlowField utmp(u);
        if (fftwmeasure)
            utmp.optimizeFFTW(FFTW_MEASURE);
        else
            utmp.optimizeFFTW(FFTW_PATIENT);
        fftw_savewisdom();

        // Define integration parameters
        const int n = 40;         // take n steps between printouts
        const Real dt = 1.0 / n;  // integration timestep

        // Define DNS parameters
        DNSFlags flags;
        //         flags.baseflow     = LaminarBase;
        flags.baseflow = SuctionBase;
        flags.timestepping = SBDF3;
        flags.initstepping = SMRK2;
        flags.nonlinearity = Rotational;
        flags.dealiasing = DealiasXZ;
        flags.taucorrection = true;
        flags.constraint = PressureGradient;  // enforce constant pressure gradient
        flags.dPdx = 0;
        flags.dt = dt;
        flags.nu = 1. / 400;
        flags.Vsuck = 1. / 400;
        flags.verbosity = Silent;

        if (u.taskid() == 0 && verbose)
            cout << "Building FlowField q..." << flush;
        vector<FlowField> fields = {
            u, FlowField(u.Nx(), u.Ny(), u.Nz(), 1, u.Lx(), u.Lz(), u.a(), u.b(), u.cfmpi(), Spectral, Spectral)};
        if (u.taskid() == 0 && verbose)
            cout << "done" << endl;
        if (u.taskid() == 0 && verbose)
            cout << "Building dns..." << flush;
        DNS dns(fields, flags);
        if (u.taskid() == 0 && verbose)
            cout << "done" << endl;

        Real avtime = 0;
        int i = 0;
        Real T = 10;
        for (Real t = 0; t <= T; t += 1) {
            timeval start, end;
            gettimeofday(&start, 0);
            Real cfl = dns.CFL(fields[0]);
            if (fields[0].taskid() == 0 && verbose)
                cout << "         t == " << t << endl;
            if (fields[0].taskid() == 0 && verbose)
                cout << "       CFL == " << cfl << endl;
            Real l2n = L2Norm(fields[0]);
            if (fields[0].taskid() == 0 && verbose)
                cout << " L2Norm(u) == " << l2n << endl;

            // Take n steps of length dt
            dns.advance(fields, n);
            if (verbose) {
                gettimeofday(&end, 0);
                Real sec = (Real)(end.tv_sec - start.tv_sec);
                Real ms = (((Real)end.tv_usec) - ((Real)start.tv_usec));
                Real timeused = sec + ms / 1000000.;
                if (fields[0].taskid() == 0)
                    cout << "duration for this timeunit: " << timeused << endl;
                if (t != 0) {
                    avtime += timeused;
                    i++;
                }
                if (fields[0].taskid() == 0)
                    cout << endl;
            }
        }

        FlowField v(dir + "/ufinal", cfmpi);
        Real l2d = L2Dist(v, fields[0]);
        if (l2d > tol) {
            if (fields[0].taskid() == 0) {
                if (verbose)
                    cout << endl << "Final L2Dist: " << l2d << endl;
                cerr << "\t** FAIL **" << endl;
                cout << "\t** FAIL **" << endl;
            }
            failure = 1;
        } else {
            if (fields[0].taskid() == 0) {
                if (verbose)
                    cout << endl << "Final L2Dist: " << l2d << endl;
                cerr << "\t   pass   " << endl;
                cout << "\t   pass   " << endl;
            }
        }
        if (fields[0].taskid() == 0 && verbose) {
            cout << "Average time/timeunit: " << avtime / i << "s" << endl;
        }
    }
    cfMPI_Finalize();
    return failure;
}
