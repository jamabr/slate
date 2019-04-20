//------------------------------------------------------------------------------
// Copyright (c) 2017, University of Tennessee
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the University of Tennessee nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL UNIVERSITY OF TENNESSEE BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//------------------------------------------------------------------------------
// This research was supported by the Exascale Computing Project (17-SC-20-SC),
// a collaborative effort of two U.S. Department of Energy organizations (Office
// of Science and the National Nuclear Security Administration) responsible for
// the planning and preparation of a capable exascale ecosystem, including
// software, applications, hardware, advanced system engineering and early
// testbed platforms, in support of the nation's exascale computing imperative.
//------------------------------------------------------------------------------
// For assistance with SLATE, email <slate-user@icl.utk.edu>.
// You can also join the "SLATE User" Google group by going to
// https://groups.google.com/a/icl.utk.edu/forum/#!forum/slate-user,
// signing in with your Google credentials, and then clicking "Join group".
//------------------------------------------------------------------------------

#include "slate/slate.hh"
#include "aux/Debug.hh"
#include "slate/Matrix.hh"
#include "slate/HermitianMatrix.hh"
#include "slate/Tile_blas.hh"
#include "slate/TriangularMatrix.hh"
#include "internal/internal.hh"

namespace slate {

// specialization namespace differentiates, e.g.,
// internal::potrs from internal::specialization::potrs
namespace internal {
namespace specialization {

//------------------------------------------------------------------------------
/// Distributed parallel Cholesky solve.
/// Generic implementation for any target.
/// @ingroup posv_specialization
///
template <Target target, typename scalar_t>
void potrs(slate::internal::TargetType<target>,
           HermitianMatrix<scalar_t>& A,
           Matrix<scalar_t>& B, int64_t lookahead)
{
    // assert(A.mt() == A.nt());
    assert(B.mt() == A.mt());

    // if upper, change to lower
    if (A.uplo() == Uplo::Upper)
        A = conj_transpose(A);

    auto L = TriangularMatrix<scalar_t>(Diag::NonUnit, A);
    auto LT = conj_transpose(L);

    trsm(Side::Left, scalar_t(1.0), L, B,
         {{Option::Lookahead, lookahead},
          {Option::Target, target}});

    trsm(Side::Left, scalar_t(1.0), LT, B,
         {{Option::Lookahead, lookahead},
          {Option::Target, target}});
}

} // namespace specialization
} // namespace internal

//------------------------------------------------------------------------------
/// Version with target as template parameter.
/// @ingroup posv_specialization
///
template <Target target, typename scalar_t>
void potrs(HermitianMatrix<scalar_t>& A,
           Matrix<scalar_t>& B,
           const std::map<Option, Value>& opts)
{
    int64_t lookahead;
    try {
        lookahead = opts.at(Option::Lookahead).i_;
        assert(lookahead >= 0);
    }
    catch (std::out_of_range) {
        lookahead = 1;
    }

    internal::specialization::potrs(internal::TargetType<target>(),
                                    A, B, lookahead);
}

//------------------------------------------------------------------------------
/// Distributed parallel Cholesky solve.
/// @ingroup posv_computational
///
template <typename scalar_t>
void potrs(HermitianMatrix<scalar_t>& A,
           Matrix<scalar_t>& B,
           const std::map<Option, Value>& opts)
{
    Target target;
    try {
        target = Target(opts.at(Option::Target).i_);
    }
    catch (std::out_of_range) {
        target = Target::HostTask;
    }

    switch (target) {
        case Target::Host:
        case Target::HostTask:
            potrs<Target::HostTask>(A, B, opts);
            break;
        case Target::HostNest:
            potrs<Target::HostNest>(A, B, opts);
            break;
        case Target::HostBatch:
            potrs<Target::HostBatch>(A, B, opts);
            break;
        case Target::Devices:
            potrs<Target::Devices>(A, B, opts);
            break;
    }
    // todo: return value for errors?
}

//------------------------------------------------------------------------------
// Explicit instantiations.
template
void potrs<float>(
    HermitianMatrix<float>& A,
    Matrix<float>& B,
    const std::map<Option, Value>& opts);

template
void potrs<double>(
    HermitianMatrix<double>& A,
    Matrix<double>& B,
    const std::map<Option, Value>& opts);

template
void potrs< std::complex<float> >(
    HermitianMatrix< std::complex<float> >& A,
    Matrix< std::complex<float> >& B,
    const std::map<Option, Value>& opts);

template
void potrs< std::complex<double> >(
    HermitianMatrix< std::complex<double> >& A,
    Matrix< std::complex<double> >& B,
    const std::map<Option, Value>& opts);

} // namespace slate
